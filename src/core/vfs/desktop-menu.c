/*
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "desktop-menu.h"
#include "fm-xml-file.h"

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

/* ---- rule tree (Include/Exclude content) ---- */

typedef enum {
    RULE_CATEGORY,
    RULE_FILENAME,
    RULE_AND,
    RULE_OR,
    RULE_NOT
} RuleOp;

typedef struct _Rule {
    RuleOp op;
    char *value;         /* Category name or .desktop filename, leaf ops only */
    GSList *children;    /* Rule*, for And/Or/Not */
} Rule;

/* allocates an empty Rule node of the given kind; leaf ops (Category/
 * Filename) still need ->value set, And/Or/Not still need ->children */
static Rule *rule_new(RuleOp op) {
    Rule *r = g_new0(Rule, 1);
    r->op = op;
    return r;
}

/* frees a Rule tree recursively (children first) */
static void rule_free(Rule *r) {
    if(!r) {
        return;
    }
    g_slist_free_full(r->children, (GDestroyNotify)rule_free);
    g_free(r->value);
    g_free(r);
}

/* Evaluates a Rule tree against one app: @categories is a GHashTable used as
 * a set of the app's Categories=, @desktop_id is its desktop-id. Standard
 * short-circuit semantics: an empty And is vacuously TRUE, an empty Or is
 * vacuously FALSE, matching how <Include>/<Exclude> behave in the spec. */
static gboolean rule_eval(const Rule *r, GHashTable *categories, const char *desktop_id) {
    GSList *l;

    if(!r) {
        return FALSE;
    }
    switch(r->op) {
    case RULE_CATEGORY:
        return g_hash_table_contains(categories, r->value);
    case RULE_FILENAME:
        return g_strcmp0(r->value, desktop_id) == 0;
    case RULE_AND:
        for(l = r->children; l; l = l->next) {
            if(!rule_eval(l->data, categories, desktop_id)) {
                return FALSE;
            }
        }
        return TRUE;
    case RULE_OR:
        for(l = r->children; l; l = l->next) {
            if(rule_eval(l->data, categories, desktop_id)) {
                return TRUE;
            }
        }
        return FALSE;
    case RULE_NOT:
        return r->children && !rule_eval(r->children->data, categories, desktop_id);
    }
    return FALSE;
}

/* ---- <Menu> tree, before rule evaluation turns it into DesktopMenuItems ---- */

typedef struct _MenuNode {
    char *name;
    char *directory_file; /* <Directory>, resolved against desktop-directories later */
    gboolean only_unallocated;
    gboolean only_unallocated_set; /* an explicit (Not)OnlyUnallocated was seen */
    gboolean deleted;
    gboolean deleted_set;          /* an explicit <Deleted/>/<NotDeleted/> was seen */
    gboolean has_include;
    Rule *include;
    Rule *exclude;
    GSList *children; /* MenuNode* */
} MenuNode;

static void menu_node_free(MenuNode *n) {
    if(!n) {
        return;
    }
    g_slist_free_full(n->children, (GDestroyNotify)menu_node_free);
    rule_free(n->include);
    rule_free(n->exclude);
    g_free(n->name);
    g_free(n->directory_file);
    g_free(n);
}

/* Appends the rules of @src to *@dst (both are OR containers, which is what
 * several <Include>/<Exclude> elements of one menu amount to) and takes
 * ownership of @src. */
static void rule_or_append(Rule **dst, Rule *src) {
    if(!src) {
        return;
    }
    if(!*dst) {
        *dst = src;
        return;
    }
    (*dst)->children = g_slist_concat((*dst)->children, src->children);
    src->children = NULL;
    rule_free(src);
}

typedef struct {
    FmXmlFileTag Menu, Name, Directory, Include, Exclude, And, Or, Not,
                 Category, Filename, OnlyUnallocated, NotOnlyUnallocated,
                 Deleted, NotDeleted, MergeFile;
} MenuTags;

/* handler that does nothing; we walk the finished tree ourselves */
static gboolean menu_xml_pass(FmXmlFileItem *item, GList *children,
                              char * const *names, char * const *values,
                              guint n, gint line, gint pos, GError **error,
                              gpointer user_data) {
    return TRUE;
}

/* FmXmlFile only hands attributes to the element handler, so this notes which
 * <MergeFile> items have type="parent"; @user_data is the GHashTable (used as
 * a set of items) that menu_doc_open() passes to the parse. */
static gboolean merge_file_xml_handler(FmXmlFileItem *item, GList *children,
                                       char * const *names, char * const *values,
                                       guint n, gint line, gint pos, GError **error,
                                       gpointer user_data) {
    guint i;
    for(i = 0; i < n; i++) {
        if(g_strcmp0(names[i], "type") == 0 && g_strcmp0(values[i], "parent") == 0) {
            g_hash_table_add(user_data, item);
        }
    }
    return TRUE;
}

/* a parsed menu file: the XML plus what its handlers noted along the way */
typedef struct {
    FmXmlFile *xml;
    MenuTags tags;
    FmXmlFileItem *root;       /* the root <Menu>, owned by xml */
    GHashTable *parent_merges; /* <MergeFile type="parent"> items, owned by xml */
} MenuDoc;

static void menu_doc_free(MenuDoc *doc) {
    if(doc) {
        g_hash_table_destroy(doc->parent_merges);
        if(doc->xml) {
            g_object_unref(doc->xml);
        }
        g_free(doc);
    }
}

/* Reads and parses the menu file at @path. Returns NULL (with @error set) if
 * it can't be read, isn't valid XML, or has no root <Menu>. */
static MenuDoc *menu_doc_open(const char *path, GError **error) {
    char *contents = NULL;
    gsize len = 0;
    MenuDoc *doc;
    MenuTags *t;
    GList *top, *l;

    if(!g_file_get_contents(path, &contents, &len, error)) {
        return NULL;
    }
    doc = g_new0(MenuDoc, 1);
    doc->parent_merges = g_hash_table_new(g_direct_hash, g_direct_equal);
    doc->xml = fm_xml_file_new(NULL);
    t = &doc->tags;
    t->Menu = fm_xml_file_set_handler(doc->xml, "Menu", menu_xml_pass, FALSE, NULL);
    t->Name = fm_xml_file_set_handler(doc->xml, "Name", menu_xml_pass, FALSE, NULL);
    t->Directory = fm_xml_file_set_handler(doc->xml, "Directory", menu_xml_pass, FALSE, NULL);
    t->Include = fm_xml_file_set_handler(doc->xml, "Include", menu_xml_pass, FALSE, NULL);
    t->Exclude = fm_xml_file_set_handler(doc->xml, "Exclude", menu_xml_pass, FALSE, NULL);
    t->And = fm_xml_file_set_handler(doc->xml, "And", menu_xml_pass, FALSE, NULL);
    t->Or = fm_xml_file_set_handler(doc->xml, "Or", menu_xml_pass, FALSE, NULL);
    t->Not = fm_xml_file_set_handler(doc->xml, "Not", menu_xml_pass, FALSE, NULL);
    t->Category = fm_xml_file_set_handler(doc->xml, "Category", menu_xml_pass, FALSE, NULL);
    t->Filename = fm_xml_file_set_handler(doc->xml, "Filename", menu_xml_pass, FALSE, NULL);
    t->OnlyUnallocated = fm_xml_file_set_handler(doc->xml, "OnlyUnallocated", menu_xml_pass, FALSE, NULL);
    t->NotOnlyUnallocated = fm_xml_file_set_handler(doc->xml, "NotOnlyUnallocated", menu_xml_pass, FALSE, NULL);
    t->Deleted = fm_xml_file_set_handler(doc->xml, "Deleted", menu_xml_pass, FALSE, NULL);
    t->NotDeleted = fm_xml_file_set_handler(doc->xml, "NotDeleted", menu_xml_pass, FALSE, NULL);
    t->MergeFile = fm_xml_file_set_handler(doc->xml, "MergeFile", merge_file_xml_handler, FALSE, NULL);

    if(!fm_xml_file_parse_data(doc->xml, contents, len, error, doc->parent_merges)) {
        g_free(contents);
        menu_doc_free(doc);
        return NULL;
    }
    g_free(contents);
    top = fm_xml_file_finish_parse(doc->xml, error);
    for(l = top; l; l = l->next) {
        if(fm_xml_file_item_get_tag(l->data) == t->Menu) {
            doc->root = l->data;
            break;
        }
    }
    g_list_free(top);
    if(!doc->root) {
        if(!error || !*error) { /* finish_parse already set one if it failed */
            g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                                "menu file has no root <Menu> element");
        }
        menu_doc_free(doc);
        return NULL;
    }
    return doc;
}

G_LOCK_DEFINE_STATIC(warned_tags);

/* logs each distinct unsupported menu-file tag once (not once per
 * occurrence), so a menu file with many unknown tags doesn't spam the log.
 * Locked because a menu may be (re)loaded from any thread. */
static void warn_unsupported_tag(const char *tag) {
    static GHashTable *warned = NULL;
    gboolean first;

    G_LOCK(warned_tags);
    if(!warned) {
        warned = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    }
    first = g_hash_table_add(warned, g_strdup(tag));
    G_UNLOCK(warned_tags);
    if(first) {
        g_warning("DesktopMenu: unsupported menu tag <%s> ignored", tag);
    }
}

/* returns the text content of a leaf XML element like <Name>text</Name>
 * (NULL if it's empty, e.g. <Name/>) */
static char *item_text(FmXmlFileItem *item) {
    FmXmlFileItem *text = fm_xml_file_item_find_child(item, FM_XML_FILE_TEXT);
    const char *data = text ? fm_xml_file_item_get_data(text, NULL) : NULL;
    return g_strdup(data);
}

/* Parses the content of an <Include> or <Exclude> element into a Rule tree.
 * Per the spec, the direct children of <Include>/<Exclude> are implicitly
 * OR'd together, so @containerOp is RULE_OR for a top-level call; nested
 * <And>/<Or>/<Not> elements recurse with their own op. Any element that
 * isn't Category/Filename/And/Or/Not is reported via warn_unsupported_tag()
 * and otherwise ignored (contributes nothing to the rule). */
static Rule *parse_rule(FmXmlFileItem *container, const MenuTags *tags, RuleOp containerOp) {
    Rule *rule = rule_new(containerOp);
    GList *children = fm_xml_file_item_get_children(container);
    GList *l;
    FmXmlFileTag tag;

    for(l = children; l; l = l->next) {
        FmXmlFileItem *child = l->data;
        tag = fm_xml_file_item_get_tag(child);
        if(tag == tags->Category) {
            Rule *leaf = rule_new(RULE_CATEGORY);
            leaf->value = item_text(child);
            rule->children = g_slist_append(rule->children, leaf);
        }
        else if(tag == tags->Filename) {
            Rule *leaf = rule_new(RULE_FILENAME);
            leaf->value = item_text(child);
            rule->children = g_slist_append(rule->children, leaf);
        }
        else if(tag == tags->And) {
            rule->children = g_slist_append(rule->children, parse_rule(child, tags, RULE_AND));
        }
        else if(tag == tags->Or) {
            rule->children = g_slist_append(rule->children, parse_rule(child, tags, RULE_OR));
        }
        else if(tag == tags->Not) {
            rule->children = g_slist_append(rule->children, parse_rule(child, tags, RULE_NOT));
        }
        else if(tag != FM_XML_FILE_TEXT) {
            warn_unsupported_tag(fm_xml_file_item_get_tag_name(child));
        }
    }
    g_list_free(children);
    return rule;
}

/* ---- <MergeFile> and the rest of menu-file composition ---- */

/* guards against a menu file merging itself, directly or through others */
#define MAX_MERGE_DEPTH 8

/* what the parser needs to know about the file it is currently in */
typedef struct {
    const char *path;  /* the file being parsed, to resolve <MergeFile> against */
    int depth;         /* how many <MergeFile>s deep we are */
    GPtrArray *files;  /* every menu file read so far (owned strings), for monitoring */
} ParseCtx;

/* <MergeFile type="parent"/>: finds the same-named menu file in the next
 * config dir after the one @current_path lives in (user dir first, then
 * $XDG_CONFIG_DIRS in order). This is how a user's ~/.config/menus override
 * pulls in the system menu it customizes. Returns NULL if there is none. */
static char *find_parent_menu_file(const char *current_path) {
    char *base = g_path_get_basename(current_path);
    const char *const *sys_dirs = g_get_system_config_dirs();
    const char *dir = g_get_user_config_dir();
    gboolean past_current = FALSE;
    char *found = NULL;
    int i = 0;

    while(dir && !found) {
        char *candidate = g_build_filename(dir, "menus", base, NULL);
        if(g_strcmp0(candidate, current_path) == 0) {
            past_current = TRUE;
        }
        else if(past_current && g_file_test(candidate, G_FILE_TEST_EXISTS)) {
            found = candidate;
            candidate = NULL;
        }
        g_free(candidate);
        dir = sys_dirs[i++];
    }
    g_free(base);
    return found;
}

static MenuNode *parse_menu(FmXmlFileItem *menuItem, const MenuDoc *doc, const ParseCtx *ctx);

/* Handles one <MergeFile>: loads the referenced file and parses its root
 * <Menu>'s children as if they had been written right here, into @node.
 * Missing or unreadable files are skipped, as the spec asks. */
static void merge_menu_file(MenuNode *node, FmXmlFileItem *item, const MenuDoc *doc,
                            const ParseCtx *ctx);

/* Fills @node from the children of a <Menu> element (Name, Directory,
 * Include, Exclude, nested <Menu>s, flags, <MergeFile>...). @skip_name is set
 * for the root <Menu> of a merged file, whose <Name> must not override the
 * referring menu's. This only builds the raw structure — matching apps
 * against it happens later, in collect_claimed()/build_tree(). */
static void parse_menu_children(MenuNode *node, FmXmlFileItem *menuItem, const MenuDoc *doc,
                                const ParseCtx *ctx, gboolean skip_name) {
    const MenuTags *tags = &doc->tags;
    GList *children = fm_xml_file_item_get_children(menuItem);
    GList *l;

    for(l = children; l; l = l->next) {
        FmXmlFileItem *child = l->data;
        FmXmlFileTag tag = fm_xml_file_item_get_tag(child);
        if(tag == tags->Name) {
            if(!skip_name || !node->name) {
                g_free(node->name);
                node->name = item_text(child);
            }
        }
        else if(tag == tags->Directory) {
            g_free(node->directory_file);
            node->directory_file = item_text(child);
        }
        else if(tag == tags->Include) {
            node->has_include = TRUE;
            rule_or_append(&node->include, parse_rule(child, tags, RULE_OR));
        }
        else if(tag == tags->Exclude) {
            rule_or_append(&node->exclude, parse_rule(child, tags, RULE_OR));
        }
        else if(tag == tags->Menu) {
            node->children = g_slist_append(node->children, parse_menu(child, doc, ctx));
        }
        else if(tag == tags->MergeFile) {
            merge_menu_file(node, child, doc, ctx);
        }
        else if(tag == tags->OnlyUnallocated || tag == tags->NotOnlyUnallocated) {
            node->only_unallocated = (tag == tags->OnlyUnallocated);
            node->only_unallocated_set = TRUE;
        }
        else if(tag == tags->Deleted || tag == tags->NotDeleted) {
            node->deleted = (tag == tags->Deleted);
            node->deleted_set = TRUE;
        }
        else if(tag == FM_XML_FILE_TEXT) {
            /* whitespace between elements, ignore */
        }
        else {
            const char *name = fm_xml_file_item_get_tag_name(child);
            if(g_strcmp0(name, "DefaultAppDirs") == 0 ||
                    g_strcmp0(name, "DefaultDirectoryDirs") == 0 ||
                    g_strcmp0(name, "Layout") == 0 || g_strcmp0(name, "DefaultLayout") == 0) {
                /* handled implicitly / display-order only, no membership effect */
            }
            else if(g_strcmp0(name, "DefaultMergeDirs") == 0) {
                /* fragments under the applications-merged dir aren't merged in yet;
                 * add when a distro is found that actually populates that directory. */
            }
            else {
                warn_unsupported_tag(name);
            }
        }
    }
    g_list_free(children);
}

/* Recursively parses one <Menu>...</Menu> element into a MenuNode tree. */
static MenuNode *parse_menu(FmXmlFileItem *menuItem, const MenuDoc *doc, const ParseCtx *ctx) {
    MenuNode *node = g_new0(MenuNode, 1);
    parse_menu_children(node, menuItem, doc, ctx, FALSE);
    return node;
}

static void merge_menu_file(MenuNode *node, FmXmlFileItem *item, const MenuDoc *doc,
                            const ParseCtx *ctx) {
    char *text = item_text(item);
    char *path = NULL;
    MenuDoc *merged;

    if(ctx->depth >= MAX_MERGE_DEPTH) {
        g_warning("DesktopMenu: <MergeFile> nested too deeply in %s, ignored", ctx->path);
        g_free(text);
        return;
    }
    if(g_hash_table_contains(doc->parent_merges, item)) {
        path = find_parent_menu_file(ctx->path);
    }
    if(!path && text && *text) { /* type="path", or a parent we couldn't find */
        if(g_path_is_absolute(text)) {
            path = g_strdup(text);
        }
        else {
            char *dir = g_path_get_dirname(ctx->path);
            path = g_build_filename(dir, text, NULL);
            g_free(dir);
        }
    }
    if(path && g_strcmp0(path, ctx->path) != 0 && (merged = menu_doc_open(path, NULL)) != NULL) {
        ParseCtx sub;
        sub.path = path;
        sub.depth = ctx->depth + 1;
        sub.files = ctx->files;
        g_ptr_array_add(ctx->files, g_strdup(path));
        parse_menu_children(node, merged->root, merged, &sub, TRUE);
        menu_doc_free(merged);
    }
    g_free(path);
    g_free(text);
}

/* Folds @src into @dst, two <Menu>s of the same name (the spec merges them):
 * Include/Exclude rules add up, and Directory / OnlyUnallocated / Deleted
 * from the later one win. @src's contents are moved out, free it afterwards. */
static void menu_node_merge(MenuNode *dst, MenuNode *src) {
    if(src->directory_file) {
        g_free(dst->directory_file);
        dst->directory_file = src->directory_file;
        src->directory_file = NULL;
    }
    if(src->only_unallocated_set) {
        dst->only_unallocated = src->only_unallocated;
        dst->only_unallocated_set = TRUE;
    }
    if(src->deleted_set) {
        dst->deleted = src->deleted;
        dst->deleted_set = TRUE;
    }
    if(src->has_include) {
        dst->has_include = TRUE;
    }
    rule_or_append(&dst->include, src->include);
    src->include = NULL;
    rule_or_append(&dst->exclude, src->exclude);
    src->exclude = NULL;
    dst->children = g_slist_concat(dst->children, src->children);
    src->children = NULL;
}

/* Merges same-named sibling menus, at every level, into the first of them.
 * A user override that repeats "Accessories" to exclude one app, on top of the
 * system menu it <MergeFile>s, ends up as one Accessories. */
static void merge_duplicate_menus(MenuNode *node) {
    GSList *unique = NULL, *l, *u;

    for(l = node->children; l; l = l->next) {
        MenuNode *child = l->data, *first = NULL;
        for(u = unique; u; u = u->next) {
            if(g_strcmp0(((MenuNode *)u->data)->name, child->name) == 0) {
                first = u->data;
                break;
            }
        }
        if(first) {
            menu_node_merge(first, child);
            menu_node_free(child);
        }
        else {
            unique = g_slist_append(unique, child);
        }
    }
    g_slist_free(node->children);
    node->children = unique;
    for(l = node->children; l; l = l->next) {
        merge_duplicate_menus(l->data);
    }
}

/* ---- locating the menu file and .directory files, XDG-style ---- */

/* Looks for @subdir/@name under @user_dir first, then under each dir in
 * @sys_dirs in order (first match wins) — the standard XDG "user overrides
 * system" search order used for both menus/ and desktop-directories/. */
static char *find_in_dirs(const char *const *sys_dirs, const char *user_dir,
                          const char *subdir, const char *name) {
    char *path = g_build_filename(user_dir, subdir, name, NULL);
    if(g_file_test(path, G_FILE_TEST_EXISTS)) {
        return path;
    }
    g_free(path);
    for(; *sys_dirs; sys_dirs++) {
        path = g_build_filename(*sys_dirs, subdir, name, NULL);
        if(g_file_test(path, G_FILE_TEST_EXISTS)) {
            return path;
        }
        g_free(path);
    }
    return NULL;
}

/* Resolves @effective_name (the menu file's name with $XDG_MENU_PREFIX
 * already applied, e.g. "lxqt-applications.menu") to an actual path under
 * $XDG_CONFIG_HOME/menus or $XDG_CONFIG_DIRS/menus, user dir first, matching
 * how menu-cache resolved the same names. Returns NULL if no such file exists
 * anywhere. */
static char *find_menu_file(const char *effective_name) {
    return find_in_dirs(g_get_system_config_dirs(), g_get_user_config_dir(),
                        "menus", effective_name);
}

/* Resolves a <Directory>name.directory</Directory> value to an actual path
 * under $XDG_DATA_HOME/desktop-directories or $XDG_DATA_DIRS/desktop-directories. */
static char *find_directory_file(const char *name) {
    if(!name || !*name) {
        return NULL;
    }
    return find_in_dirs(g_get_system_data_dirs(), g_get_user_data_dir(),
                        "desktop-directories", name);
}

/* returns the resolved .directory path (transfer full, NULL if not found);
 * name_out and icon_out are replaced in place when the file provides them */
static char *apply_directory_file(char **name_out, char **icon_out, const char *directory_file) {
    char *path = find_directory_file(directory_file);
    GKeyFile *kf;

    if(!path) {
        return NULL;
    }
    kf = g_key_file_new();
    if(g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        char *name = g_key_file_get_locale_string(kf, "Desktop Entry", "Name", NULL, NULL);
        if(name) {
            g_free(*name_out);
            *name_out = name;
        }
        char *icon = g_key_file_get_string(kf, "Desktop Entry", "Icon", NULL);
        if(icon) {
            g_free(*icon_out);
            *icon_out = icon;
        }
    }
    g_key_file_free(kf);
    return path;
}

/* ---- candidate applications, via GIO ---- */

typedef struct {
    GAppInfo *info;
    char *desktop_id;
    char *sort_key;         /* g_utf8_collate_key() of the display name */
    GHashTable *categories; /* set of owned strings */
    gboolean visible;
} CandidateApp;

/* GDestroyNotify for CandidateApp, used as GPtrArray's free func */
static void candidate_app_free(CandidateApp *app) {
    if(!app) {
        return;
    }
    g_object_unref(app->info);
    g_free(app->desktop_id);
    g_free(app->sort_key);
    g_hash_table_destroy(app->categories);
    g_free(app);
}

/* GCompareFunc for the GPtrArray of CandidateApp: alphabetical by display
 * name (locale-aware), desktop-id as tie-break. g_app_info_get_all() returns
 * apps in no particular order, and menu-cache used to hand them out sorted. */
static gint candidate_app_compare(gconstpointer a, gconstpointer b) {
    const CandidateApp *x = *(CandidateApp * const *)a;
    const CandidateApp *y = *(CandidateApp * const *)b;
    gint r = g_strcmp0(x->sort_key, y->sort_key);
    return r ? r : g_strcmp0(x->desktop_id, y->desktop_id);
}

/* Enumerates every installed application via GIO (this is the direct
 * replacement for menu-cache's own app enumeration) and pre-computes what
 * rule_eval()/build_tree() need per app: its desktop-id, its Categories= as
 * a set, and whether it's currently visible. Result is sorted by display
 * name and owned by the caller; free with g_ptr_array_free(result, TRUE). */
static GPtrArray *list_candidate_apps(void) {
    GPtrArray *apps = g_ptr_array_new_with_free_func((GDestroyNotify)candidate_app_free);
    GList *all = g_app_info_get_all();
    GList *l;

    for(l = all; l; l = l->next) {
        GAppInfo *info = G_APP_INFO(l->data);
        GDesktopAppInfo *dinfo = G_IS_DESKTOP_APP_INFO(info) ? G_DESKTOP_APP_INFO(info) : NULL;
        CandidateApp *app;

        /* NoDisplay/Hidden apps should never show up anywhere, unlike a plain
         * desktop-environment mismatch which we still want to keep (as hidden)
         * for callers like the menu:// backend that list hidden items too. */
        if(dinfo && (g_desktop_app_info_get_nodisplay(dinfo) || g_desktop_app_info_get_is_hidden(dinfo))) {
            g_object_unref(info);
            continue;
        }

        app = g_new0(CandidateApp, 1);
        app->info = info; /* takes ownership of this ref */
        app->desktop_id = g_strdup(g_app_info_get_id(info));
        app->sort_key = g_utf8_collate_key(g_app_info_get_display_name(info), -1);
        app->categories = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
        app->visible = g_app_info_should_show(info);
        if(dinfo) {
            const char *cats = g_desktop_app_info_get_categories(dinfo);
            if(cats) {
                char **split = g_strsplit(cats, ";", -1);
                char **c;
                for(c = split; *c; c++) {
                    if(**c) {
                        g_hash_table_add(app->categories, g_strdup(*c));
                    }
                }
                g_strfreev(split);
            }
        }
        g_ptr_array_add(apps, app);
    }
    g_list_free(all);
    g_ptr_array_sort(apps, candidate_app_compare);
    return apps;
}

/* ---- DesktopMenuItem tree ---- */

struct _DesktopMenuItem {
    gint ref_count;
    DesktopMenuItemType type;
    char *id;
    char *name;
    char *icon;
    char *file_path; /* apps only */
    gboolean visible;
    GSList *children; /* DesktopMenuItem*, dirs only, owned refs */
};

/* allocates an empty, ref-count-1 DesktopMenuItem of the given type; all
 * other fields are left NULL/FALSE for the caller to fill in */
static DesktopMenuItem *item_new(DesktopMenuItemType type) {
    DesktopMenuItem *item = g_new0(DesktopMenuItem, 1);
    item->ref_count = 1;
    item->type = type;
    return item;
}

DesktopMenuItem *desktop_menu_item_ref(DesktopMenuItem *item) {
    if(item) {
        g_atomic_int_inc(&item->ref_count);
    }
    return item;
}

void desktop_menu_item_unref(DesktopMenuItem *item) {
    if(item && g_atomic_int_dec_and_test(&item->ref_count)) {
        g_slist_free_full(item->children, (GDestroyNotify)desktop_menu_item_unref);
        g_free(item->id);
        g_free(item->name);
        g_free(item->icon);
        g_free(item->file_path);
        g_free(item);
    }
}

DesktopMenuItemType desktop_menu_item_get_item_type(DesktopMenuItem *item) {
    return item->type;
}

const char *desktop_menu_item_get_id(DesktopMenuItem *item) {
    return item->id;
}

const char *desktop_menu_item_get_name(DesktopMenuItem *item) {
    return item->name;
}

const char *desktop_menu_item_get_icon(DesktopMenuItem *item) {
    return item->icon;
}

char *desktop_menu_item_get_file_path(DesktopMenuItem *item) {
    return g_strdup(item->file_path);
}

gboolean desktop_menu_item_get_is_visible(DesktopMenuItem *item) {
    return item->visible;
}

GSList *desktop_menu_dir_list_children(DesktopMenuItem *dir) {
    GSList *l, *out = NULL;
    for(l = dir->children; l; l = l->next) {
        out = g_slist_append(out, desktop_menu_item_ref(l->data));
    }
    return out;
}

DesktopMenuItem *desktop_menu_find_child_by_name(DesktopMenuItem *dir, const char *name) {
    GSList *l;
    for(l = dir->children; l; l = l->next) {
        DesktopMenuItem *item = l->data;
        if(g_strcmp0(item->id, name) == 0) {
            return desktop_menu_item_ref(item);
        }
    }
    return NULL;
}

/* Recursive worker for desktop_menu_find_item_by_id(): depth-first search
 * of @dir's subtree for an APP item (never a dir) with this desktop-id. */
static DesktopMenuItem *find_item_by_id_recursive(DesktopMenuItem *dir, const char *id) {
    GSList *l;
    for(l = dir->children; l; l = l->next) {
        DesktopMenuItem *item = l->data;
        if(item->type == DESKTOP_MENU_TYPE_APP && g_strcmp0(item->id, id) == 0) {
            return desktop_menu_item_ref(item);
        }
        if(item->type == DESKTOP_MENU_TYPE_DIR) {
            DesktopMenuItem *found = find_item_by_id_recursive(item, id);
            if(found) {
                return found;
            }
        }
    }
    return NULL;
}

/* Builds a leaf DesktopMenuItem (type APP) from a matched CandidateApp,
 * copying over the display name, icon, visibility and .desktop file path. */
static DesktopMenuItem *make_app_item(const CandidateApp *app) {
    DesktopMenuItem *item = item_new(DESKTOP_MENU_TYPE_APP);
    GIcon *icon;

    item->id = g_strdup(app->desktop_id);
    item->name = g_strdup(g_app_info_get_display_name(app->info));
    item->visible = app->visible;
    icon = g_app_info_get_icon(app->info);
    if(icon) {
        item->icon = g_icon_to_string(icon);
    }
    if(G_IS_DESKTOP_APP_INFO(app->info)) {
        const char *path = g_desktop_app_info_get_filename(G_DESKTOP_APP_INFO(app->info));
        if(path) {
            item->file_path = g_strdup(path);
        }
    }
    return item;
}

/* First pass of the two-pass OnlyUnallocated algorithm (see build_tree() for
 * the second pass): walks the WHOLE tree once, and for every non-
 * OnlyUnallocated node, marks in @claimed every app that node's rules match.
 * Must run to completion before build_tree(), since an OnlyUnallocated menu
 * anywhere in the tree needs to know what every OTHER menu claimed, not just
 * its own siblings. @claimed is a GHashTable used purely as a set of
 * desktop-ids (values unused, only key presence matters). */
static void collect_claimed(const MenuNode *node, GPtrArray *apps, GHashTable *claimed) {
    guint i;
    GSList *l;

    if(node->deleted) { /* a <Deleted/> menu and its submenus claim nothing */
        return;
    }
    if(!node->only_unallocated && node->has_include) {
        for(i = 0; i < apps->len; i++) {
            CandidateApp *app = g_ptr_array_index(apps, i);
            if(rule_eval(node->include, app->categories, app->desktop_id) &&
                    !rule_eval(node->exclude, app->categories, app->desktop_id)) {
                g_hash_table_add(claimed, app->desktop_id);
            }
        }
    }
    for(l = node->children; l; l = l->next) {
        collect_claimed(l->data, apps, claimed);
    }
}

/* Second pass: turns a MenuNode (+ the @claimed set from collect_claimed())
 * into the real DesktopMenuItem tree that callers query. For a normal menu,
 * every app matching its Include/Exclude rules is added; for an
 * OnlyUnallocated menu, apps already present in @claimed are skipped even if
 * they'd otherwise match. A dir's own visibility is the OR of all its
 * children's visibility, computed bottom-up as this recurses. */
static DesktopMenuItem *build_tree(const MenuNode *node, GPtrArray *apps, GHashTable *claimed) {
    DesktopMenuItem *dir = item_new(DESKTOP_MENU_TYPE_DIR);
    GSList *l;
    guint i;

    dir->id = g_strdup(node->name ? node->name : "");
    dir->name = g_strdup(node->name);
    dir->file_path = apply_directory_file(&dir->name, &dir->icon, node->directory_file);

    if(node->has_include) {
        for(i = 0; i < apps->len; i++) {
            CandidateApp *app = g_ptr_array_index(apps, i);
            gboolean included = rule_eval(node->include, app->categories, app->desktop_id) &&
                                !rule_eval(node->exclude, app->categories, app->desktop_id);
            if(included && node->only_unallocated && g_hash_table_contains(claimed, app->desktop_id)) {
                included = FALSE;
            }
            if(included) {
                DesktopMenuItem *appItem = make_app_item(app);
                dir->children = g_slist_append(dir->children, appItem);
                dir->visible = dir->visible || appItem->visible;
            }
        }
    }
    for(l = node->children; l; l = l->next) {
        DesktopMenuItem *child;
        if(((const MenuNode *)l->data)->deleted) {
            continue;
        }
        child = build_tree(l->data, apps, claimed);
        dir->children = g_slist_append(dir->children, child);
        dir->visible = dir->visible || child->visible;
    }
    return dir;
}

/* ---- DesktopMenu ---- */

/* Refcounted so desktop_menu_notify() can keep calling a snapshot of the
 * registered callbacks after dropping the lock, even if one callback
 * unregisters another (or itself) meanwhile: unregistering sets @removed and
 * drops the list's ref, the entry is only freed once the snapshot is done. */
typedef struct {
    gint ref_count;
    gint removed;
    DesktopMenuReloadNotify fn;
    gpointer user_data;
} ReloadNotifyEntry;

static void notify_entry_unref(ReloadNotifyEntry *e) {
    if(g_atomic_int_dec_and_test(&e->ref_count)) {
        g_free(e);
    }
}

/* wait this long after the last change event before reloading, so a burst
 * (a package install touching many .desktop files) costs one rebuild */
#define RELOAD_DELAY_MS 300

struct _DesktopMenu {
    gint ref_count;
    char *menu_file_name;
    char *effective_name;       /* $XDG_MENU_PREFIX + menu_file_name, fixed at lookup
                                 * time so a later env change can't redirect reloads */
    GMutex lock;                /* protects root, notify_list and reload_source */
    DesktopMenuItem *root;
    GPtrArray *menu_files;      /* the menu files the last load read (owned strings) */
    GHashTable *file_monitors;  /* path -> GFileMonitor, one per menu file read
                                 * (the root file plus any <MergeFile>d ones) */
    GFileMonitor *dir_monitor;  /* the user's menus dir, to notice an override appearing */
    GAppInfoMonitor *app_monitor;
    guint reload_source;
    GSList *notify_list;        /* ReloadNotifyEntry* */
};

static void desktop_menu_file_changed(GFileMonitor *mon, GFile *file, GFile *other,
                                      GFileMonitorEvent event, gpointer user_data);
static void desktop_menu_dir_changed(GFileMonitor *mon, GFile *file, GFile *other,
                                     GFileMonitorEvent event, gpointer user_data);

/* stops and frees a GFileMonitor created for either handler above */
static void monitor_free(gpointer data) {
    GFileMonitor *mon = data;
    g_signal_handlers_disconnect_matched(mon, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
                                         (gpointer)desktop_menu_file_changed, NULL);
    g_signal_handlers_disconnect_matched(mon, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
                                         (gpointer)desktop_menu_dir_changed, NULL);
    g_file_monitor_cancel(mon);
    g_object_unref(mon);
}

/* Makes dm->file_monitors watch exactly dm->menu_files (what the last load
 * read): starts monitors for new ones, drops those no longer part of the menu.
 * Must run in the main context (see desktop_menu_setup_monitors()). */
static void desktop_menu_update_monitors(DesktopMenu *dm) {
    const GPtrArray *files = dm->menu_files;
    GHashTableIter iter;
    gpointer key;
    guint i;

    g_hash_table_iter_init(&iter, dm->file_monitors);
    while(g_hash_table_iter_next(&iter, &key, NULL)) {
        gboolean used = FALSE;
        for(i = 0; i < files->len && !used; i++) {
            used = g_strcmp0(key, g_ptr_array_index(files, i)) == 0;
        }
        if(!used) {
            g_hash_table_iter_remove(&iter);
        }
    }
    for(i = 0; i < files->len; i++) {
        const char *path = g_ptr_array_index(files, i);
        if(!g_hash_table_contains(dm->file_monitors, path)) {
            GFile *gf = g_file_new_for_path(path);
            GFileMonitor *mon = g_file_monitor_file(gf, G_FILE_MONITOR_NONE, NULL, NULL);
            g_object_unref(gf);
            if(mon) {
                g_signal_connect(mon, "changed", G_CALLBACK(desktop_menu_file_changed), dm);
                g_hash_table_insert(dm->file_monitors, g_strdup(path), mon);
            }
        }
    }
}

/* (Re)builds @dm->root from scratch: resolves the menu file (again every
 * time, so an override created later in the user's config dir is picked up),
 * parses it and everything it <MergeFile>s with FmXmlFile, merges same-named
 * menus, re-enumerates installed apps, and runs the collect_claimed()/
 * build_tree() two-pass match, and records which files it read in
 * dm->menu_files. Called once from desktop_menu_lookup() and again on every
 * change notification. Leaves @dm->root untouched if
 * parsing/loading fails, so a broken edit to the menu file doesn't blow away
 * the last-good tree. */
static gboolean desktop_menu_load(DesktopMenu *dm, GError **error) {
    char *path = find_menu_file(dm->effective_name);
    MenuDoc *doc;
    MenuNode *rootNode;
    ParseCtx ctx;
    GPtrArray *files;
    GPtrArray *apps;
    GHashTable *claimed;
    DesktopMenuItem *newRoot, *oldRoot;

    if(!path) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                    "menu file \"%s\" not found", dm->menu_file_name);
        return FALSE;
    }
    doc = menu_doc_open(path, error);
    if(!doc) {
        g_free(path);
        return FALSE;
    }

    files = g_ptr_array_new_with_free_func(g_free);
    g_ptr_array_add(files, path); /* the array owns @path now */
    ctx.path = path;
    ctx.depth = 0;
    ctx.files = files;
    rootNode = parse_menu(doc->root, doc, &ctx);
    menu_doc_free(doc);
    merge_duplicate_menus(rootNode);

    apps = list_candidate_apps();
    claimed = g_hash_table_new(g_str_hash, g_str_equal);
    collect_claimed(rootNode, apps, claimed);

    newRoot = build_tree(rootNode, apps, claimed);
    /* readers on other threads fetch dm->root under the same lock, and the
     * tree itself is immutable once built, so swapping the pointer is enough */
    g_mutex_lock(&dm->lock);
    oldRoot = dm->root;
    dm->root = newRoot;
    g_mutex_unlock(&dm->lock);
    desktop_menu_item_unref(oldRoot);

    if(dm->menu_files) {
        g_ptr_array_free(dm->menu_files, TRUE);
    }
    dm->menu_files = files;

    menu_node_free(rootNode);
    g_hash_table_destroy(claimed);
    g_ptr_array_free(apps, TRUE);
    return TRUE;
}

/* Fires every callback registered via desktop_menu_add_reload_notify()
 * (vfs-menu.c uses this to diff the old and new trees and emit granular
 * GFileMonitor events of its own). Works on a snapshot taken under the lock,
 * so a callback may register/unregister callbacks (even itself) safely. */
static void desktop_menu_notify(DesktopMenu *dm) {
    GSList *snapshot = NULL, *l;

    g_mutex_lock(&dm->lock);
    for(l = dm->notify_list; l; l = l->next) {
        ReloadNotifyEntry *e = l->data;
        g_atomic_int_inc(&e->ref_count);
        snapshot = g_slist_prepend(snapshot, e);
    }
    g_mutex_unlock(&dm->lock);

    snapshot = g_slist_reverse(snapshot);
    for(l = snapshot; l; l = l->next) {
        ReloadNotifyEntry *e = l->data;
        if(!g_atomic_int_get(&e->removed)) {
            e->fn(dm, e->user_data);
        }
        notify_entry_unref(e);
    }
    g_slist_free(snapshot);
}

/* Timeout callback of desktop_menu_schedule_reload(): rebuilds the tree, and
 * only if that worked (a broken menu edit keeps the last-good tree) tells the
 * registered callbacks. Always runs in the main context, so reloads never
 * overlap with each other. */
static gboolean desktop_menu_reload_cb(gpointer data) {
    DesktopMenu *dm = data;

    g_mutex_lock(&dm->lock);
    dm->reload_source = 0;
    g_mutex_unlock(&dm->lock);
    if(desktop_menu_load(dm, NULL)) {
        desktop_menu_update_monitors(dm); /* the set of menu files may have changed */
        desktop_menu_notify(dm);
    }
    return G_SOURCE_REMOVE;
}

/* Requests a reload after RELOAD_DELAY_MS; requests arriving while one is
 * already pending are folded into it. The pending source holds a ref on @dm. */
static void desktop_menu_schedule_reload(DesktopMenu *dm) {
    g_mutex_lock(&dm->lock);
    if(!dm->reload_source) {
        dm->reload_source = g_timeout_add_full(G_PRIORITY_DEFAULT, RELOAD_DELAY_MS,
                                               desktop_menu_reload_cb, desktop_menu_ref(dm),
                                               (GDestroyNotify)desktop_menu_unref);
    }
    g_mutex_unlock(&dm->lock);
}

/* whether a GFileMonitor event means the file's content may have changed:
 * skips the high-frequency events emitted mid-write (CHANGED itself), so a
 * text editor saving the menu file doesn't trigger several redundant reparses */
static gboolean is_settled_change(GFileMonitorEvent event) {
    return event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
           event == G_FILE_MONITOR_EVENT_CREATED || event == G_FILE_MONITOR_EVENT_DELETED;
}

/* GFileMonitor "changed" handler for one of the menu files being read */
static void desktop_menu_file_changed(GFileMonitor *mon, GFile *file, GFile *other,
                                      GFileMonitorEvent event, gpointer user_data) {
    if(is_settled_change(event)) {
        desktop_menu_schedule_reload(user_data);
    }
}

/* GFileMonitor "changed" handler for the user's menus dir: reloads when the
 * menu file we resolve to appears, disappears or changes there, e.g. the
 * first override written by the menu:// backend, which then takes precedence
 * over the system file we were reading. */
static void desktop_menu_dir_changed(GFileMonitor *mon, GFile *file, GFile *other,
                                     GFileMonitorEvent event, gpointer user_data) {
    DesktopMenu *dm = user_data;
    char *name;

    if(!is_settled_change(event)) {
        return;
    }
    name = g_file_get_basename(file);
    if(g_strcmp0(name, dm->effective_name) == 0) {
        desktop_menu_schedule_reload(dm);
    }
    g_free(name);
}

/* GAppInfoMonitor "changed" handler: the set of installed applications
 * changed (something was installed/removed/edited), so re-match them. */
static void desktop_menu_apps_changed(GAppInfoMonitor *mon, gpointer user_data) {
    desktop_menu_schedule_reload(user_data);
}

/* Starts watching everything a reload depends on: every menu file the last
 * load read, the user's menus dir (so an override created later is noticed)
 * and the set of installed applications.
 *
 * GFileMonitor and GAppInfoMonitor emit their signals in the thread-default
 * main context of whoever creates them, and a menu is usually first looked up
 * on a worker thread: Qt gives those a private context nobody iterates, so
 * monitors made there would never fire. desktop_menu_lookup() therefore runs
 * this in the global default context, which is the one the main thread runs
 * (there is no pushing that context from another thread, GLib requires
 * owning it for that). Consumes the ref taken for it. */
static gboolean desktop_menu_setup_monitors(gpointer data) {
    DesktopMenu *dm = data;
    char *userMenusDir = g_build_filename(g_get_user_config_dir(), "menus", NULL);
    GFile *gf = g_file_new_for_path(userMenusDir);

    desktop_menu_update_monitors(dm);
    dm->dir_monitor = g_file_monitor_directory(gf, G_FILE_MONITOR_NONE, NULL, NULL);
    g_object_unref(gf);
    g_free(userMenusDir);
    if(dm->dir_monitor) {
        g_signal_connect(dm->dir_monitor, "changed", G_CALLBACK(desktop_menu_dir_changed), dm);
    }
    dm->app_monitor = g_app_info_monitor_get();
    g_signal_connect(dm->app_monitor, "changed", G_CALLBACK(desktop_menu_apps_changed), dm);
    desktop_menu_unref(dm);
    return G_SOURCE_REMOVE;
}

DesktopMenu *desktop_menu_lookup(const char *menu_file_name, GError **error) {
    const char *prefix = g_getenv("XDG_MENU_PREFIX");
    DesktopMenu *dm = g_new0(DesktopMenu, 1);

    dm->ref_count = 1;
    g_mutex_init(&dm->lock);
    dm->menu_file_name = g_strdup(menu_file_name);
    dm->effective_name = g_strconcat(prefix ? prefix : "", menu_file_name, NULL);
    dm->file_monitors = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, monitor_free);

    if(!desktop_menu_load(dm, error)) {
        desktop_menu_unref(dm);
        return NULL;
    }
    /* runs right away when called from the main thread, else once its loop gets to it */
    g_main_context_invoke(NULL, desktop_menu_setup_monitors, desktop_menu_ref(dm));
    return dm;
}

DesktopMenu *desktop_menu_ref(DesktopMenu *dm) {
    if(dm) {
        g_atomic_int_inc(&dm->ref_count);
    }
    return dm;
}

void desktop_menu_unref(DesktopMenu *dm) {
    if(dm && g_atomic_int_dec_and_test(&dm->ref_count)) {
        g_slist_free_full(dm->notify_list, (GDestroyNotify)notify_entry_unref);
        g_hash_table_destroy(dm->file_monitors);
        if(dm->menu_files) {
            g_ptr_array_free(dm->menu_files, TRUE);
        }
        if(dm->dir_monitor) {
            monitor_free(dm->dir_monitor);
        }
        if(dm->app_monitor) {
            g_signal_handlers_disconnect_by_data(dm->app_monitor, dm);
            g_object_unref(dm->app_monitor);
        }
        desktop_menu_item_unref(dm->root);
        g_mutex_clear(&dm->lock);
        g_free(dm->menu_file_name);
        g_free(dm->effective_name);
        g_free(dm);
    }
}

DesktopMenuItem *desktop_menu_dup_root_dir(DesktopMenu *dm) {
    DesktopMenuItem *root;

    g_mutex_lock(&dm->lock);
    root = desktop_menu_item_ref(dm->root);
    g_mutex_unlock(&dm->lock);
    return root;
}

DesktopMenuItem *desktop_menu_find_item_by_id(DesktopMenu *dm, const char *id) {
    DesktopMenuItem *root = desktop_menu_dup_root_dir(dm);
    DesktopMenuItem *found = root ? find_item_by_id_recursive(root, id) : NULL;

    desktop_menu_item_unref(root);
    return found;
}

DesktopMenuItem *desktop_menu_item_from_path(DesktopMenu *dm, const char *path) {
    DesktopMenuItem *cur = desktop_menu_dup_root_dir(dm);
    char **segments, **s;

    if(!cur || !path || !*path) {
        return cur;
    }
    segments = g_strsplit(path, "/", -1);
    for(s = segments; *s; s++) {
        DesktopMenuItem *next;
        if((*s)[0] == '\0' || cur->type != DESKTOP_MENU_TYPE_DIR) {
            desktop_menu_item_unref(cur);
            cur = NULL;
            break;
        }
        next = desktop_menu_find_child_by_name(cur, *s);
        desktop_menu_item_unref(cur);
        cur = next;
        if(!cur) {
            break;
        }
    }
    g_strfreev(segments);
    return cur;
}

gpointer desktop_menu_add_reload_notify(DesktopMenu *dm, DesktopMenuReloadNotify fn, gpointer user_data) {
    ReloadNotifyEntry *e = g_new0(ReloadNotifyEntry, 1);
    e->ref_count = 1; /* the list's ref */
    e->fn = fn;
    e->user_data = user_data;
    g_mutex_lock(&dm->lock);
    dm->notify_list = g_slist_append(dm->notify_list, e);
    g_mutex_unlock(&dm->lock);
    return e;
}

void desktop_menu_remove_reload_notify(DesktopMenu *dm, gpointer notify_id) {
    ReloadNotifyEntry *e = notify_id;
    gboolean found;

    g_mutex_lock(&dm->lock);
    found = g_slist_find(dm->notify_list, e) != NULL;
    if(found) {
        dm->notify_list = g_slist_remove(dm->notify_list, e);
    }
    g_mutex_unlock(&dm->lock);
    if(found) {
        g_atomic_int_set(&e->removed, 1);
        notify_entry_unref(e);
    }
}

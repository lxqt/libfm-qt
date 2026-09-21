/* Self-check for core/vfs/desktop-menu.c's parser + rule matching:
 * builds a throwaway menu file and a handful of fake .desktop entries under
 * a scratch XDG_DATA_HOME/XDG_CONFIG_HOME, then asserts the resulting tree
 * matches what the freedesktop.org Desktop Menu Specification says it should
 * (Category/And/Or/Not matching, OnlyUnallocated, NoDisplay exclusion). */

#include "core/vfs/desktop-menu.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

/* writes @contents to @path, aborting the test immediately (g_error) if
 * the scratch fixture can't even be set up */
static void write_file(const char *path, const char *contents) {
    GError *error = NULL;
    if(!g_file_set_contents(path, contents, -1, &error)) {
        g_error("failed to write %s: %s", path, error->message);
    }
}

/* TRUE if @dir has a direct child (app or dir) with this id */
static gboolean has_child(DesktopMenuItem *dir, const char *id) {
    GSList *children = desktop_menu_dir_list_children(dir);
    GSList *l;
    gboolean found = FALSE;
    for(l = children; l; l = l->next) {
        if(g_strcmp0(desktop_menu_item_get_id(l->data), id) == 0) {
            found = TRUE;
        }
    }
    g_slist_free_full(children, (GDestroyNotify)desktop_menu_item_unref);
    return found;
}

/* id of @dir's n-th direct child, or NULL; free with g_free() */
static char *child_id_at(DesktopMenuItem *dir, guint n) {
    GSList *children = desktop_menu_dir_list_children(dir);
    GSList *nth = g_slist_nth(children, n);
    char *id = nth ? g_strdup(desktop_menu_item_get_id(nth->data)) : NULL;
    g_slist_free_full(children, (GDestroyNotify)desktop_menu_item_unref);
    return id;
}

typedef struct {
    const char *name;
    DesktopMenu *dm;
    GError *error;
} LookupArgs;

/* Qt gives every worker thread its own GMainContext and makes it the
 * thread-default one; nothing iterates it. The first menu:// lookup often runs
 * on such a thread (path completion, folder listing), so the menu must still
 * deliver its reloads to the main context. */
static gpointer lookup_in_private_context(gpointer data) {
    LookupArgs *args = data;
    GMainContext *priv = g_main_context_new();

    g_main_context_push_thread_default(priv);
    args->dm = desktop_menu_lookup(args->name, &args->error);
    g_main_context_pop_thread_default(priv);
    g_main_context_unref(priv);
    return NULL;
}

static int reload_count = 0;
static gpointer reload_id = NULL;

/* unregisters itself from inside the callback, which must be safe */
static void on_reload(DesktopMenu *dm, gpointer user_data) {
    reload_count++;
    desktop_menu_remove_reload_notify(dm, reload_id);
}

/* runs the main loop until @dm reloads once (its callback above ran) */
static void wait_for_reload(DesktopMenu *dm) {
    gint64 deadline = g_get_monotonic_time() + 10 * G_USEC_PER_SEC;

    reload_count = 0;
    reload_id = desktop_menu_add_reload_notify(dm, on_reload, NULL);
    while(reload_count == 0 && g_get_monotonic_time() < deadline) {
        g_main_context_iteration(NULL, FALSE);
        g_usleep(20000);
    }
    g_assert_cmpint(reload_count, ==, 1);
}

int main(void) {
    char *tmp = g_dir_make_tmp("desktop-menu-test-XXXXXX", NULL);
    char *data_home = g_build_filename(tmp, "data", NULL);
    char *config_home = g_build_filename(tmp, "config", NULL);
    char *apps_dir = g_build_filename(data_home, "applications", NULL);
    char *menus_dir = g_build_filename(config_home, "menus", NULL);
    char *sys_config = g_build_filename(tmp, "sysconfig", NULL);
    char *sys_menus_dir = g_build_filename(sys_config, "menus", NULL);
    char *empty_dir = g_build_filename(tmp, "empty", NULL);
    GError *error = NULL;
    DesktopMenu *dm;
    DesktopMenuItem *root, *accessories, *dev, *other;

    g_mkdir_with_parents(apps_dir, 0700);
    g_mkdir_with_parents(menus_dir, 0700);
    g_mkdir_with_parents(sys_menus_dir, 0700);
    g_mkdir_with_parents(empty_dir, 0700);

    /* keep the test hermetic: only our fake apps/menus should be visible */
    g_setenv("XDG_DATA_HOME", data_home, TRUE);
    g_setenv("XDG_DATA_DIRS", empty_dir, TRUE);
    g_setenv("XDG_CONFIG_HOME", config_home, TRUE);
    g_setenv("XDG_CONFIG_DIRS", sys_config, TRUE);
    g_unsetenv("XDG_MENU_PREFIX");

    write_file(g_build_filename(apps_dir, "app-utility-only.desktop", NULL),
              "[Desktop Entry]\nType=Application\nName=UtilOnly\nExec=/bin/true\nCategories=Utility;\n");
    write_file(g_build_filename(apps_dir, "app-utility-dev.desktop", NULL),
              "[Desktop Entry]\nType=Application\nName=UtilDev\nExec=/bin/true\nCategories=Utility;Development;\n");
    write_file(g_build_filename(apps_dir, "app-alpha.desktop", NULL),
              "[Desktop Entry]\nType=Application\nName=Alpha\nExec=/bin/true\nCategories=Utility;\n");
    write_file(g_build_filename(apps_dir, "app-game.desktop", NULL),
              "[Desktop Entry]\nType=Application\nName=Game\nExec=/bin/true\nCategories=Game;\nNoDisplay=true\n");
    write_file(g_build_filename(apps_dir, "app-orphan.desktop", NULL),
              "[Desktop Entry]\nType=Application\nName=Orphan\nExec=/bin/true\nCategories=Network;\n");

    write_file(g_build_filename(sys_menus_dir, "test-apps.menu", NULL),
        "<Menu>\n"
        "  <Name>Applications</Name>\n"
        "  <Menu>\n"
        "    <Name>Accessories</Name>\n"
        "    <Include><And><Category>Utility</Category><Not><Category>Development</Category></Not></And></Include>\n"
        "  </Menu>\n"
        "  <Menu>\n"
        "    <Name>Dev</Name>\n"
        "    <Include><Category>Development</Category></Include>\n"
        "  </Menu>\n"
        "  <Menu>\n"
        "    <Name>Empty</Name>\n"
        "    <Include><Category>NoAppHasThis</Category></Include>\n"
        "  </Menu>\n"
        "  <Menu>\n"
        "    <Name>Other</Name>\n"
        "    <OnlyUnallocated/>\n"
        "    <Include><Not><Category>Core</Category></Not></Include>\n"
        "  </Menu>\n"
        "</Menu>\n");

    {
        LookupArgs args = { "test-apps.menu", NULL, NULL };
        g_thread_join(g_thread_new("lookup", lookup_in_private_context, &args));
        g_assert_no_error(args.error);
        dm = args.dm;
        g_assert_nonnull(dm);
    }
    /* the menu sets up its file monitors in the main context, once it runs */
    while(g_main_context_iteration(NULL, FALSE)) {
    }

    root = desktop_menu_dup_root_dir(dm);
    g_assert_nonnull(root);
    g_assert_cmpstr(desktop_menu_item_get_id(root), ==, "Applications");

    accessories = desktop_menu_find_child_by_name(root, "Accessories");
    dev = desktop_menu_find_child_by_name(root, "Dev");
    other = desktop_menu_find_child_by_name(root, "Other");
    g_assert_nonnull(accessories);
    g_assert_nonnull(dev);
    g_assert_nonnull(other);

    /* And/Not: Utility-but-not-Development goes to Accessories only */
    g_assert_true(has_child(accessories, "app-utility-only.desktop"));
    g_assert_false(has_child(accessories, "app-utility-dev.desktop"));

    /* the Development app instead lands in Dev */
    g_assert_true(has_child(dev, "app-utility-dev.desktop"));

    /* NoDisplay=true is dropped from the tree entirely, not even in Other */
    g_assert_false(has_child(other, "app-game.desktop"));

    /* OnlyUnallocated: only the app nobody else claimed shows up here */
    g_assert_true(has_child(other, "app-orphan.desktop"));
    g_assert_false(has_child(other, "app-utility-only.desktop"));
    g_assert_false(has_child(other, "app-utility-dev.desktop"));

    /* apps come out alphabetical by display name (Alpha before UtilOnly),
     * not in g_app_info_get_all()'s arbitrary order */
    {
        char *first = child_id_at(accessories, 0);
        g_assert_cmpstr(first, ==, "app-alpha.desktop");
        g_free(first);
    }

    /* a menu nothing matched stays in the tree but is marked not visible,
     * so views can skip it */
    {
        DesktopMenuItem *empty = desktop_menu_find_child_by_name(root, "Empty");
        g_assert_nonnull(empty);
        g_assert_false(desktop_menu_item_get_is_visible(empty));
        g_assert_true(desktop_menu_item_get_is_visible(accessories));
        desktop_menu_item_unref(empty);
    }

    /* path resolution */
    {
        DesktopMenuItem *byPath = desktop_menu_item_from_path(dm, "Dev");
        g_assert_nonnull(byPath);
        g_assert_true(has_child(byPath, "app-utility-dev.desktop"));
        desktop_menu_item_unref(byPath);

        g_assert_null(desktop_menu_find_child_by_name(root, "NoSuchMenu"));
    }

    /* installing an app makes the menu reload by itself (main loop needed),
     * and a callback unregistering itself mid-notification is fine */
    write_file(g_build_filename(apps_dir, "app-late.desktop", NULL),
              "[Desktop Entry]\nType=Application\nName=Late\nExec=/bin/true\nCategories=Utility;\n");
    wait_for_reload(dm);
    {
        DesktopMenuItem *late = desktop_menu_find_item_by_id(dm, "app-late.desktop");
        g_assert_nonnull(late);
        desktop_menu_item_unref(late);
    }

    /* a user override that merges its parent, deletes Dev, excludes an app
     * from Accessories and adds an Extra menu: the file appears after the
     * lookup, in a dir that had no menu file, and must be picked up and
     * layered over the system menu instead of replacing it */
    write_file(g_build_filename(menus_dir, "test-apps.menu", NULL),
        "<Menu>\n"
        "  <Name>Applications</Name>\n"
        "  <MergeFile type=\"parent\">/does/not/matter.menu</MergeFile>\n"
        "  <Menu><Name>Dev</Name><Deleted/></Menu>\n"
        "  <Menu><Name>Accessories</Name>\n"
        "    <Exclude><Filename>app-utility-only.desktop</Filename></Exclude>\n"
        "  </Menu>\n"
        "  <Menu><Name>Extra</Name>\n"
        "    <Include><Filename>app-orphan.desktop</Filename></Include>\n"
        "  </Menu>\n"
        "</Menu>\n");
    wait_for_reload(dm);
    {
        DesktopMenuItem *root2 = desktop_menu_dup_root_dir(dm);
        DesktopMenuItem *acc2 = desktop_menu_find_child_by_name(root2, "Accessories");
        DesktopMenuItem *extra2 = desktop_menu_find_child_by_name(root2, "Extra");
        DesktopMenuItem *other2 = desktop_menu_find_child_by_name(root2, "Other");

        g_assert_null(desktop_menu_find_child_by_name(root2, "Dev"));        /* Deleted */
        g_assert_nonnull(acc2);                                              /* parent merged in */
        g_assert_true(has_child(acc2, "app-alpha.desktop"));
        g_assert_false(has_child(acc2, "app-utility-only.desktop"));         /* Exclude added */
        g_assert_nonnull(extra2);
        g_assert_true(has_child(extra2, "app-orphan.desktop"));
        g_assert_false(has_child(other2, "app-orphan.desktop"));             /* now allocated */

        desktop_menu_item_unref(other2);
        desktop_menu_item_unref(extra2);
        desktop_menu_item_unref(acc2);
        desktop_menu_item_unref(root2);
    }

    desktop_menu_item_unref(accessories);
    desktop_menu_item_unref(dev);
    desktop_menu_item_unref(other);
    desktop_menu_item_unref(root);
    desktop_menu_unref(dm);

    g_print("test-desktop-menu: all assertions passed\n");
    return 0;
}

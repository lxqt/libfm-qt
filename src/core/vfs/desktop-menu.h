/*
 *      Plain C/GLib replacement for libmenu-cache: parses a freedesktop.org
 *      Desktop Menu file (e.g. lxqt-applications.menu) and matches installed
 *      applications against it using GIO, without an external daemon.
 *
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

#ifndef __DESKTOP_MENU_H__
#define __DESKTOP_MENU_H__

#include <glib.h>
#include "fm-qt6_export.h"

G_BEGIN_DECLS

/**
 * DesktopMenu:
 *
 * A parsed, matched freedesktop.org Desktop Menu (e.g. lxqt-applications.menu)
 * plus the installed applications resolved against it. Opaque, refcounted.
 */
typedef struct _DesktopMenu     DesktopMenu;

/**
 * DesktopMenuItem:
 *
 * One node of a #DesktopMenu tree: either a submenu (%DESKTOP_MENU_TYPE_DIR)
 * or a single application entry (%DESKTOP_MENU_TYPE_APP). Opaque, refcounted.
 */
typedef struct _DesktopMenuItem DesktopMenuItem;

/**
 * DesktopMenuItemType:
 * @DESKTOP_MENU_TYPE_APP: a single application entry (leaf)
 * @DESKTOP_MENU_TYPE_DIR: a submenu, which may have children
 *
 * The kind of a #DesktopMenuItem, as returned by desktop_menu_item_get_item_type().
 */
typedef enum {
    DESKTOP_MENU_TYPE_APP,
    DESKTOP_MENU_TYPE_DIR
} DesktopMenuItemType;

/**
 * desktop_menu_lookup:
 * @menu_file_name: (transfer none): base name of the menu file to look up,
 * e.g. "applications-fm.menu"
 * @error: (allow-none) (out): location to save an error, if the menu file
 * can't be found or fails to parse
 *
 * Looks up and parses a menu file the same way menu-cache did: honors
 * $XDG_MENU_PREFIX, searching $XDG_CONFIG_HOME/menus then each dir in
 * $XDG_CONFIG_DIRS/menus, first match wins. Installed applications are then
 * matched against it via GIO (g_app_info_get_all() + should_show()).
 *
 * Only the tag subset lxqt-applications.menu and the menu:// backend actually
 * use is understood: Menu/Name/Directory, Include/Exclude, And/Or/Not,
 * Category, Filename, (Not)OnlyUnallocated, Deleted/NotDeleted and
 * MergeFile (type="parent" or a path). Same-named sibling menus are merged,
 * so a user file in $XDG_CONFIG_HOME/menus that <MergeFile>s its system
 * counterpart and tweaks a few menus works as the spec describes. Anything
 * else is logged once (via g_warning()) and ignored rather than silently
 * mishandled. <DefaultMergeDirs/> fragments under applications-merged/ are
 * not merged in yet.
 *
 * Unlike menu-cache, hidden apps (failing OnlyShowIn/NotShowIn/TryExec) are
 * kept in the tree with desktop_menu_item_get_is_visible() returning %FALSE,
 * rather than dropped, so callers that want to show them greyed-out (like
 * the menu:// GVFS backend) still can. Apps with NoDisplay=true or
 * Hidden=true are always dropped, since those should never be shown anywhere.
 *
 * The returned #DesktopMenu watches every menu file it read, the user's menus
 * dir (so an override created later is picked up) and the set of installed
 * applications, and rebuilds itself (in the main context, coalescing bursts
 * of changes) when any of them changes; see
 * desktop_menu_add_reload_notify(). A #DesktopMenu is thread-safe: any thread
 * may query it, while reloads and their notifications happen in the main
 * context. Apps sorted alphabetically by display name within each menu.
 *
 * Returns: (transfer full) (nullable): a new #DesktopMenu, or %NULL on
 * failure (see @error). Free with desktop_menu_unref().
 */
LIBFM_QT_API DesktopMenu *desktop_menu_lookup(const char *menu_file_name, GError **error);

/**
 * desktop_menu_ref:
 * @dm: a #DesktopMenu
 *
 * Increments the reference count of @dm.
 *
 * Returns: (transfer full): @dm, for convenience
 */
LIBFM_QT_API DesktopMenu *desktop_menu_ref(DesktopMenu *dm);

/**
 * desktop_menu_unref:
 * @dm: (transfer full): a #DesktopMenu
 *
 * Decrements the reference count of @dm, freeing it (and its whole item
 * tree) once it reaches zero.
 */
LIBFM_QT_API void desktop_menu_unref(DesktopMenu *dm);

/**
 * desktop_menu_dup_root_dir:
 * @dm: a #DesktopMenu
 *
 * Gets the root directory of the parsed menu tree.
 *
 * Returns: (transfer full) (nullable): the root #DesktopMenuItem, or %NULL
 * if @dm failed to load. Free with desktop_menu_item_unref().
 */
LIBFM_QT_API DesktopMenuItem *desktop_menu_dup_root_dir(DesktopMenu *dm);

/**
 * desktop_menu_item_from_path:
 * @dm: a #DesktopMenu
 * @path: (nullable): a "/"-separated path of item ids, relative to the root
 * dir (e.g. "Accessories/firefox.desktop"), matching the shape
 * menu_cache_item_from_path()'s callers already used. %NULL or empty means
 * the root dir itself.
 *
 * Resolves a path within the menu tree to the #DesktopMenuItem it names, by
 * walking one path segment at a time via desktop_menu_find_child_by_name().
 *
 * Returns: (transfer full) (nullable): the matching item, or %NULL if no
 * such path exists. Free with desktop_menu_item_unref().
 */
LIBFM_QT_API DesktopMenuItem *desktop_menu_item_from_path(DesktopMenu *dm, const char *path);

/**
 * desktop_menu_item_ref:
 * @item: a #DesktopMenuItem
 *
 * Increments the reference count of @item.
 *
 * Returns: (transfer full): @item, for convenience
 */
LIBFM_QT_API DesktopMenuItem *desktop_menu_item_ref(DesktopMenuItem *item);

/**
 * desktop_menu_item_unref:
 * @item: (transfer full): a #DesktopMenuItem
 *
 * Decrements the reference count of @item, freeing it (and, for a dir, its
 * children) once it reaches zero.
 */
LIBFM_QT_API void desktop_menu_item_unref(DesktopMenuItem *item);

/**
 * desktop_menu_item_get_item_type:
 * @item: a #DesktopMenuItem
 *
 * Returns: whether @item is an app or a dir
 */
LIBFM_QT_API DesktopMenuItemType desktop_menu_item_get_item_type(DesktopMenuItem *item);

/**
 * desktop_menu_item_get_id:
 * @item: a #DesktopMenuItem
 *
 * For an app, this is its desktop-id (e.g. "firefox.desktop"), globally
 * unique. For a dir, this is just its own <Name> (a single path segment,
 * NOT the full path from the root) — unique only among its siblings, so
 * callers that need a globally-stable key for a dir must build the full
 * path themselves by walking up to the root.
 *
 * Returns: (transfer none): the item's id, owned by @item
 */
LIBFM_QT_API const char *desktop_menu_item_get_id(DesktopMenuItem *item);

/**
 * desktop_menu_item_get_name:
 * @item: a #DesktopMenuItem
 *
 * Returns: (transfer none): the item's display name, owned by @item
 */
LIBFM_QT_API const char *desktop_menu_item_get_name(DesktopMenuItem *item);

/**
 * desktop_menu_item_get_icon:
 * @item: a #DesktopMenuItem
 *
 * Returns: (transfer none) (nullable): the item's icon name/spec, owned by
 * @item, or %NULL if it has none
 */
LIBFM_QT_API const char *desktop_menu_item_get_icon(DesktopMenuItem *item);

/**
 * desktop_menu_item_get_file_path:
 * @item: a #DesktopMenuItem
 *
 * Gets the backing file for @item: the .desktop file for an app, or the
 * .directory file for a dir (if it has a <Directory> tag and the file was
 * actually found).
 *
 * Returns: (transfer full) (nullable): the file path, or %NULL if there is
 * no backing file. Free with g_free().
 */
LIBFM_QT_API char *desktop_menu_item_get_file_path(DesktopMenuItem *item);

/**
 * desktop_menu_item_get_is_visible:
 * @item: a #DesktopMenuItem
 *
 * For an app, this reflects g_app_info_should_show() (OnlyShowIn/NotShowIn/
 * TryExec against the current desktop environment) — items failing this are
 * still kept in the tree (see desktop_menu_lookup()), just marked hidden.
 * For a dir, this is %TRUE if at least one descendant app is visible.
 *
 * Returns: whether @item should currently be shown to the user
 */
LIBFM_QT_API gboolean desktop_menu_item_get_is_visible(DesktopMenuItem *item);

/**
 * desktop_menu_dir_list_children:
 * @dir: a #DesktopMenuItem of type %DESKTOP_MENU_TYPE_DIR
 *
 * Returns: (transfer full) (element-type DesktopMenuItem): the dir's
 * children, each ref'd, in menu order. %NULL if @dir is an app or has no
 * children. Free with g_slist_free_full(list, (GDestroyNotify)desktop_menu_item_unref).
 */
LIBFM_QT_API GSList *desktop_menu_dir_list_children(DesktopMenuItem *dir);

/**
 * desktop_menu_find_child_by_name:
 * @dir: a #DesktopMenuItem of type %DESKTOP_MENU_TYPE_DIR
 * @name: the id (see desktop_menu_item_get_id()) of the child to find
 *
 * Returns: (transfer full) (nullable): the matching direct child, or %NULL
 * if @dir has none with that id. Free with desktop_menu_item_unref().
 */
LIBFM_QT_API DesktopMenuItem *desktop_menu_find_child_by_name(DesktopMenuItem *dir, const char *name);

/**
 * desktop_menu_find_item_by_id:
 * @dm: a #DesktopMenu
 * @id: the desktop-id of the app to find (e.g. "firefox.desktop")
 *
 * Searches the whole tree, at any depth, for an app with this id — useful
 * for checking whether an id is already in use anywhere in the menu before
 * creating a new entry with it.
 *
 * Returns: (transfer full) (nullable): the matching app item, or %NULL if
 * none exists. Free with desktop_menu_item_unref().
 */
LIBFM_QT_API DesktopMenuItem *desktop_menu_find_item_by_id(DesktopMenu *dm, const char *id);

/**
 * DesktopMenuReloadNotify:
 * @dm: the #DesktopMenu that was reloaded
 * @user_data: data passed to desktop_menu_add_reload_notify()
 *
 * Callback invoked after the underlying menu file changes on disk and @dm's
 * tree has already been rebuilt. Any #DesktopMenuItem fetched before this
 * fires still refers to the old tree; fetch the root/children again to see
 * the new one.
 */
typedef void (*DesktopMenuReloadNotify)(DesktopMenu *dm, gpointer user_data);

/**
 * desktop_menu_add_reload_notify:
 * @dm: a #DesktopMenu
 * @fn: (scope notified): callback to invoke on reload
 * @user_data: data to pass to @fn
 *
 * Registers @fn to be called every time @dm reloads (see
 * #DesktopMenuReloadNotify). Multiple callbacks may be registered.
 *
 * Returns: (transfer none): an opaque id for this registration, to be passed
 * to desktop_menu_remove_reload_notify() later
 */
LIBFM_QT_API gpointer desktop_menu_add_reload_notify(DesktopMenu *dm, DesktopMenuReloadNotify fn, gpointer user_data);

/**
 * desktop_menu_remove_reload_notify:
 * @dm: a #DesktopMenu
 * @notify_id: (transfer full): the id returned by desktop_menu_add_reload_notify()
 *
 * Unregisters a previously-added reload callback.
 */
LIBFM_QT_API void desktop_menu_remove_reload_notify(DesktopMenu *dm, gpointer notify_id);

G_END_DECLS

#endif /* __DESKTOP_MENU_H__ */

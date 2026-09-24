/*
 * Copyright (C) 2014 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "appmenuview.h"
#include <QStandardItemModel>
#include "appmenuview_p.h"
#include "core/filepath.h"

#include <gio/gdesktopappinfo.h>

namespace Fm {

// walks up the QStandardItem tree, joining each dir's own (single-segment)
// id, to rebuild the full path relative to the menu root. Dir ids alone are
// only unique among siblings, not globally, so anything that needs a stable
// key for a dir (selection/expand-state tracking across a reload) must use
// this instead of the bare id.
static QByteArray dirRelativePath(QStandardItem* item) {
    QByteArray path;
    while(item) {
        auto* dirItem = static_cast<AppMenuViewItem*>(item);
        QByteArray segment(desktop_menu_item_get_id(dirItem->item()));
        path = path.isEmpty() ? segment : (segment + '/' + path);
        item = item->parent();
    }
    return path;
}

AppMenuView::AppMenuView(QWidget* parent):
    QTreeView(parent),
    model_(new QStandardItemModel()),
    menu_(nullptr),
    reloadNotifyId_(nullptr) {

    setHeaderHidden(true);
    setSelectionMode(SingleSelection);

    // initialize model
    // TODO: share one model among all app menu view widgets
    // ensure that we're using the fm menu of lxqt-menu-data
    QByteArray oldenv = qgetenv("XDG_MENU_PREFIX");
    qputenv("XDG_MENU_PREFIX", "lxqt-");
    menu_ = desktop_menu_lookup("applications-fm.menu", nullptr);
    // if(!oldenv.isEmpty())
    qputenv("XDG_MENU_PREFIX", oldenv); // restore the original value if needed

    if(menu_) {
        reloadNotifyId_ = desktop_menu_add_reload_notify(menu_, &_onMenuReloaded, this);
        if(DesktopMenuItem* dir = desktop_menu_dup_root_dir(menu_)) {
            addMenuItems(nullptr, dir);
            desktop_menu_item_unref(dir);
        }
    }
    setModel(model_);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &AppMenuView::selectionChanged);
    setCurrentIndex(model_->index(0, 0));
}

AppMenuView::~AppMenuView() {
    delete model_;
    if(menu_) {
        if(reloadNotifyId_) {
            desktop_menu_remove_reload_notify(menu_, reloadNotifyId_);
        }
        desktop_menu_unref(menu_);
    }
}

void AppMenuView::addMenuItems(QStandardItem* parentItem, DesktopMenuItem* dir) {
    GSList* children = desktop_menu_dir_list_children(dir);
    for(GSList* l = children; l; l = l->next) {
        DesktopMenuItem* menuItem = static_cast<DesktopMenuItem*>(l->data);
        // the tree keeps hidden apps and empty submenus (the menu:// backend
        // shows them greyed-out), but a chooser must not offer them
        if(!desktop_menu_item_get_is_visible(menuItem)) {
            continue;
        }
        AppMenuViewItem* newItem = new AppMenuViewItem(menuItem);
        if(parentItem) {
            parentItem->insertRow(parentItem->rowCount(), newItem);
        }
        else {
            model_->insertRow(model_->rowCount(), newItem);
        }
        if(desktop_menu_item_get_item_type(menuItem) == DESKTOP_MENU_TYPE_DIR) {
            addMenuItems(newItem, menuItem);
        }
    }
    g_slist_free_full(children, (GDestroyNotify)desktop_menu_item_unref);
}

void AppMenuView::onMenuReloaded() {
    auto expanded = getExpanded();
    QByteArray selectedId;
    bool isDir = false;
    QModelIndexList selected = selectedIndexes();
    if(!selected.isEmpty()) {
        if(AppMenuViewItem* item = static_cast<AppMenuViewItem*>(model_->itemFromIndex(selected.first()))) {
            isDir = item->isDir();
            selectedId = isDir ? dirRelativePath(item) : QByteArray(desktop_menu_item_get_id(item->item()));
        }
    }

    model_->clear();
    if(DesktopMenuItem* dir = desktop_menu_dup_root_dir(menu_)) {
        addMenuItems(nullptr, dir);
        desktop_menu_item_unref(dir);

        // try to restore the expansion state and selection
        restoreExpanded(expanded);
        QModelIndex indx = indexForId(selectedId, isDir);
        if(!indx.isValid()) {
            indx = model_->index(0, 0);
        }
        setCurrentIndex(indx);
    }
}

bool AppMenuView::isAppSelected() const {
    AppMenuViewItem* item = selectedItem();
    return (item && item->isApp());
}

AppMenuViewItem* AppMenuView::selectedItem() const {
    QModelIndexList selected = selectedIndexes();
    if(!selected.isEmpty()) {
        AppMenuViewItem* item = static_cast<AppMenuViewItem*>(model_->itemFromIndex(selected.first()
                                                                                   ));
        return item;
    }
    return nullptr;
}

Fm::GAppInfoPtr AppMenuView::selectedApp() const {
    const char* id = selectedAppDesktopId();
    return Fm::GAppInfoPtr{id ? G_APP_INFO(g_desktop_app_info_new(id)) : nullptr, false};
}

QByteArray AppMenuView::selectedAppDesktopFilePath() const {
    AppMenuViewItem* item = selectedItem();
    if(item && item->isApp()) {
        char* path = desktop_menu_item_get_file_path(item->item());
        QByteArray ret(path);
        g_free(path);
        return ret;
    }
    return QByteArray();
}

const char* AppMenuView::selectedAppDesktopId() const {
    AppMenuViewItem* item = selectedItem();
    if(item && item->isApp()) {
        return desktop_menu_item_get_id(item->item());
    }
    return nullptr;
}

FilePath AppMenuView::selectedAppDesktopPath() const {
    AppMenuViewItem* item = selectedItem();
    FilePath path;
    if(item && item->isApp()) {
        // the app's own id goes last; apps at the top level have no parent dir
        QByteArray relative(desktop_menu_item_get_id(item->item()));
        if(QStandardItem* parentItem = item->parent()) {
            relative = dirRelativePath(parentItem) + '/' + relative;
        }
        path = FilePath::fromUri("menu://applications/").relativePath(relative.constData());
    }
    return path;
}

QModelIndex AppMenuView::indexForId(const QByteArray& id, bool isDir, const QModelIndex& index) const {
    if(id.isEmpty()) {
        return QModelIndex();
    }
    auto child = model_->index(0, 0, index);
    while(child.isValid()) {
        if(isDir == model_->hasChildren(child)) {
            if(AppMenuViewItem* item = static_cast<AppMenuViewItem*>(model_->itemFromIndex(child))) {
                QByteArray itemId = isDir ? dirRelativePath(item) : QByteArray(desktop_menu_item_get_id(item->item()));
                if(id == itemId) {
                    return child;
                }
            }
        }
        auto indx = indexForId(id, isDir, child);
        if(indx.isValid()) {
            return indx;
        }
        child = child.siblingAtRow(child.row() + 1);
    }
    return QModelIndex();
}

QSet<QByteArray> AppMenuView::getExpanded(const QModelIndex& index) const {
    QSet<QByteArray> expanded;
    auto child = model_->index(0, 0, index);
    while(child.isValid()) {
        if(isExpanded(child)) {
            if(AppMenuViewItem* item = static_cast<AppMenuViewItem*>(model_->itemFromIndex(child))) {
                expanded.insert(dirRelativePath(item)); // only dirs can be expanded
            }
            expanded.unite(getExpanded(child)); // only for children of expanded items
        }
        child = child.siblingAtRow(child.row() + 1);
    }
    return expanded;
}

void AppMenuView::restoreExpanded(const QSet<QByteArray>& expanded, const QModelIndex& index) {
    if(expanded.isEmpty()) {
        return;
    }
    auto l = expanded;
    auto child = model_->index(0, 0, index);
    while(child.isValid()) {
        if(model_->hasChildren(child)) {
            if(AppMenuViewItem* item = static_cast<AppMenuViewItem*>(model_->itemFromIndex(child))) {
                auto b = dirRelativePath(item);
                if(l.contains(b)) {
                    setExpanded(child, true);
                    l.remove(b);
                    if(l.isEmpty()) {
                        return;
                    }
                    restoreExpanded(l, child); // only for children of expanded items
                }
            }
        }
        child = child.siblingAtRow(child.row() + 1);
    }
}

} // namespace Fm

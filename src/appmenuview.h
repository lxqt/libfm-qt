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

#ifndef FM_APPMENUVIEW_H
#define FM_APPMENUVIEW_H

#include <QTreeView>
#include "libfmqtglobals.h"
#include "core/vfs/desktop-menu.h"

#include "core/gioptrs.h"
#include "core/filepath.h"

class QStandardItemModel;
class QStandardItem;

namespace Fm {

class AppMenuViewItem;

class LIBFM_QT_API AppMenuView : public QTreeView {
    Q_OBJECT
public:
    explicit AppMenuView(QWidget* parent = nullptr);
    ~AppMenuView() override;

    Fm::GAppInfoPtr selectedApp() const;

    const char* selectedAppDesktopId() const;

    QByteArray selectedAppDesktopFilePath() const;

    FilePath selectedAppDesktopPath() const;

    bool isAppSelected() const;

Q_SIGNALS:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

private:
    // recursively builds QStandardItems for dir's children under parentItem
    // (or at the top level of model_ if parentItem is null)
    void addMenuItems(QStandardItem* parentItem, DesktopMenuItem* dir);
    // rebuilds the whole model from menu_'s (already-reloaded) tree, trying
    // to preserve the current expansion state and selection across the rebuild
    void onMenuReloaded();
    // C-linkage trampoline for desktop_menu_add_reload_notify(), since that
    // API takes a plain function pointer, not a Qt slot
    static void _onMenuReloaded(DesktopMenu* dm, gpointer user_data) {
        Q_UNUSED(dm);
        static_cast<AppMenuView*>(user_data)->onMenuReloaded();
    }

    AppMenuViewItem* selectedItem() const;

    QModelIndex indexForId(const QByteArray& id, bool isDir, const QModelIndex& index = QModelIndex()) const;
    QSet<QByteArray> getExpanded(const QModelIndex& index = QModelIndex()) const;
    void restoreExpanded(const QSet<QByteArray>& expanded, const QModelIndex& index = QModelIndex());

private:
    QStandardItemModel* model_;
    DesktopMenu* menu_;
    gpointer reloadNotifyId_;
};

}

#endif // FM_APPMENUVIEW_H

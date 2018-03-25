/*
 * Copyright (C) 2012 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#include "placesview.h"
#include "placesmodel.h"
#include "placesmodelitem.h"
#include "mountoperation.h"
#include "fileoperation.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QDebug>
#include <QGuiApplication>
#include "folderitemdelegate.h"

namespace Fm {

std::shared_ptr<PlacesProxyModel> PlacesView::proxyModel_;

PlacesProxyModel::PlacesProxyModel(QObject* parent) :
    QSortFilterProxyModel(parent),
    showAll_(false),
    hiddenItemsRestored_(false) {
}

PlacesProxyModel::~PlacesProxyModel() {
}

void PlacesProxyModel::restoreHiddenItems(const QSet<QString>& items) {
    // hidden items should be restored only once
    if(!hiddenItemsRestored_ && !items.isEmpty()) {
        hidden_.clear();
        QSet<QString>::const_iterator i = items.constBegin();
        while (i != items.constEnd()) {
            if(!(*i).isEmpty()) {
                hidden_ << *i;
            }
            ++i;
        }
        hiddenItemsRestored_ = true;
        invalidateFilter();
    }
}

void PlacesProxyModel::setHidden(const QString& str, bool hide) {
    if(hide) {
        if(!str.isEmpty()) {
            hidden_ << str;
        }
    }
    else {
        hidden_.remove(str);
    }
    invalidateFilter();
}

void PlacesProxyModel::showAll(bool show) {
    showAll_ = show;
    invalidateFilter();
}

bool PlacesProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    if(showAll_ || hidden_.isEmpty()) {
        return true;
    }
    if(PlacesModel* srcModel = static_cast<PlacesModel*>(sourceModel())) {
        QModelIndex index = srcModel->index(source_row, 0, source_parent);
        if(PlacesModelItem* item = static_cast<PlacesModelItem*>(srcModel->itemFromIndex(index))) {
            if(item->type() == PlacesModelItem::Places) {
                if(auto path = item->path()) {
                    if(hidden_.contains(path.toString().get())) {
                        return false;
                    }
                }
            }
            else if(item->type() == PlacesModelItem::Volume) {
                CStrPtr uuid{g_volume_get_uuid(static_cast<PlacesModelVolumeItem*>(item)->volume())};
                if(uuid && hidden_.contains(uuid.get())) {
                    return false;
                }
            }
            // show a root items only if, at least, one of its children is shown
            else if((source_row == 0 || source_row == 1) && !source_parent.isValid()) {
                QModelIndex indx = index.child(0, 0);
                while(PlacesModelItem* childItem = static_cast<PlacesModelItem*>(srcModel->itemFromIndex(indx))) {
                    if(childItem->type() == PlacesModelItem::Places) {
                        if(auto path = childItem->path()) {
                            if(!hidden_.contains(path.toString().get())) {
                                return true;
                            }
                        }
                    }
                    else if(childItem->type() == PlacesModelItem::Volume) {
                        CStrPtr uuid{g_volume_get_uuid(static_cast<PlacesModelVolumeItem*>(childItem)->volume())};
                        if(uuid == nullptr || !hidden_.contains(uuid.get())) {
                            return true;
                        }
                    }
                    else {
                        return true;
                    }
                    indx = indx.sibling(indx.row() + 1, 0);
                }
                return false;
            }
        }
    }
    return true;
}

PlacesView::PlacesView(QWidget* parent):
    QTreeView(parent) {
    setRootIsDecorated(false);
    setHeaderHidden(true);
    setIndentation(12);

    /* merge with the surroundings */
    setFrameShape(QFrame::NoFrame);
    QPalette p = palette();
    p.setColor(QPalette::Base, QColor(Qt::transparent));
    setPalette(p);
    viewport()->setAutoFillBackground(false);

    connect(this, &QTreeView::clicked, this, &PlacesView::onClicked);
    connect(this, &QTreeView::pressed, this, &PlacesView::onPressed);

    setIconSize(QSize(24, 24));

    FolderItemDelegate* delegate = new FolderItemDelegate(this, this);
    delegate->setFileInfoRole(PlacesModel::FileInfoRole);
    delegate->setIconInfoRole(PlacesModel::FmIconRole);
    setItemDelegateForColumn(0, delegate);

    model_ = PlacesModel::globalInstance();
    if(!proxyModel_) {
        proxyModel_ = std::make_shared<PlacesProxyModel>();
    }
    if(!proxyModel_->sourceModel()) { // all places-views may have been closed
        proxyModel_->setSourceModel(model_.get());
    }
    setModel(proxyModel_.get());

    // these 2 connections are needed to update filtering
    connect(model_.get(), &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex&, int, int) {
        proxyModel_->setHidden(QString()); // just invalidates filter
        expandAll();
        // for some reason (a Qt bug?), spanning is reset
        setFirstColumnSpanned(0, QModelIndex(), true);
        setFirstColumnSpanned(1, QModelIndex(), true);
        setFirstColumnSpanned(2, QModelIndex(), true);

    });
    connect(model_.get(), &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex&, int, int) {
        proxyModel_->setHidden(QString());
    });

    QHeaderView* headerView = header();
    headerView->setSectionResizeMode(0, QHeaderView::Stretch);
    headerView->setSectionResizeMode(1, QHeaderView::Fixed);
    headerView->setStretchLastSection(false);
    expandAll();

    // FIXME: is there any better way to make the first column span the whole row?
    setFirstColumnSpanned(0, QModelIndex(), true); // places root
    setFirstColumnSpanned(1, QModelIndex(), true); // devices root
    setFirstColumnSpanned(2, QModelIndex(), true); // bookmarks root

    // the 2nd column is for the eject buttons
    setSelectionBehavior(QAbstractItemView::SelectRows); // FIXME: why this does not work?
    setAllColumnsShowFocus(false);

    setAcceptDrops(true);
    setDragEnabled(true);

    // update the umount button's column width based on icon size
    onIconSizeChanged(iconSize());
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0) // this signal requires Qt >= 5.5
    connect(this, &QAbstractItemView::iconSizeChanged, this, &PlacesView::onIconSizeChanged);
#endif
}

PlacesView::~PlacesView() {
    // qDebug("delete PlacesView");
}

void PlacesView::activateRow(int type, const QModelIndex& index) {
    if(!index.parent().isValid()) { // ignore root items
        return;
    }
    PlacesModelItem* item = static_cast<PlacesModelItem*>(model_->itemFromIndex(proxyModel_->mapToSource(index)));
    if(item) {
        auto path = item->path();
        if(!path) {
            // check if mounting volumes is needed
            if(item->type() == PlacesModelItem::Volume) {
                PlacesModelVolumeItem* volumeItem = static_cast<PlacesModelVolumeItem*>(item);
                if(!volumeItem->isMounted()) {
                    // Mount the volume
                    GVolume* volume = volumeItem->volume();
                    MountOperation* op = new MountOperation(true, this);
                    op->mount(volume);
                    // connect(op, SIGNAL(finished(GError*)), SLOT(onMountOperationFinished(GError*)));
                    // blocking here until the mount operation is finished?

                    // FIXME: update status of the volume after mount is finished!!
                    if(!op->wait()) {
                        return;
                    }
                    path = item->path();
                }
            }
        }
        if(path) {
            Q_EMIT chdirRequested(type, path);
        }
    }
}

// mouse button pressed
void PlacesView::onPressed(const QModelIndex& index) {
    // if middle button is pressed
    if(QGuiApplication::mouseButtons() & Qt::MiddleButton) {
        // the real item is at column 0
        activateRow(1, 0 == index.column() ? index : index.sibling(index.row(), 0));
    }
}

void PlacesView::onIconSizeChanged(const QSize& size) {
    setColumnWidth(1, size.width() + 5);
}

void PlacesView::onEjectButtonClicked(PlacesModelItem* item) {
    // The eject button is clicked for a device item (volume or mount)
    if(item->type() == PlacesModelItem::Volume) {
        PlacesModelVolumeItem* volumeItem = static_cast<PlacesModelVolumeItem*>(item);
        MountOperation* op = new MountOperation(true, this);
        if(volumeItem->canEject()) { // do eject if applicable
            op->eject(volumeItem->volume());
        }
        else { // otherwise, do unmount instead
            op->unmount(volumeItem->volume());
        }
    }
    else if(item->type() == PlacesModelItem::Mount) {
        PlacesModelMountItem* mountItem = static_cast<PlacesModelMountItem*>(item);
        MountOperation* op = new MountOperation(true, this);
        op->unmount(mountItem->mount());
    }
    qDebug("PlacesView::onEjectButtonClicked");
}

void PlacesView::onClicked(const QModelIndex& index) {
    if(!index.parent().isValid()) { // ignore root items
        return;
    }

    if(index.column() == 0) {
        activateRow(0, index);
    }
    else if(index.column() == 1) { // column 1 contains eject buttons of the mounted devices
        if(index.parent() == proxyModel_->mapFromSource(model_->devicesRoot->index())) { // this is a mounted device
            // the eject button is clicked
            QModelIndex itemIndex = index.sibling(index.row(), 0); // the real item is at column 0
            PlacesModelItem* item = static_cast<PlacesModelItem*>(model_->itemFromIndex(proxyModel_->mapToSource(itemIndex)));
            if(item) {
                // eject the volume or the mount
                onEjectButtonClicked(item);
            }
        }
        else {
            activateRow(0, index.sibling(index.row(), 0));
        }
    }
}

void PlacesView::setCurrentPath(Fm::FilePath path) {
    currentPath_ = std::move(path);
    if(currentPath_) {
        // TODO: search for item with the path in model_ and select it.
        PlacesModelItem* item = model_->itemFromPath(currentPath_);
        if(item) {
            selectionModel()->select(proxyModel_->mapFromSource(item->index()), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        }
        else {
            clearSelection();
        }
    }
    else {
        clearSelection();
    }
}


void PlacesView::dragMoveEvent(QDragMoveEvent* event) {
    QTreeView::dragMoveEvent(event);
    /*
    QModelIndex index = indexAt(event->pos());
    if(event->isAccepted() && index.isValid() && index.parent() == model_->bookmarksRoot->index()) {
      if(dropIndicatorPosition() != OnItem) {
        event->setDropAction(Qt::LinkAction);
        event->accept();
      }
    }
    */
}

void PlacesView::dropEvent(QDropEvent* event) {
    QTreeView::dropEvent(event);
}

void PlacesView::onEmptyTrash() {
    Fm::FilePathList files;
    files.push_back(Fm::FilePath::fromUri("trash:///"));
    Fm::FileOperation::deleteFiles(std::move(files));
}

void PlacesView::onMoveBookmarkUp() {
    PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
    if(!action->index().isValid()) {
        return;
    }
    PlacesModelBookmarkItem* item = static_cast<PlacesModelBookmarkItem*>(model_->itemFromIndex(action->index()));

    int row = item->row();
    if(row > 0) {
        auto bookmarkItem = item->bookmark();
        Fm::Bookmarks::globalInstance()->reorder(bookmarkItem, row - 1);
    }
}

void PlacesView::onMoveBookmarkDown() {
    PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
    if(!action->index().isValid()) {
        return;
    }
    PlacesModelBookmarkItem* item = static_cast<PlacesModelBookmarkItem*>(model_->itemFromIndex(action->index()));

    int row = item->row();
    if(row < model_->rowCount()) {
        auto bookmarkItem = item->bookmark();
        Fm::Bookmarks::globalInstance()->reorder(bookmarkItem, row + 1);
    }
}

void PlacesView::onDeleteBookmark() {
    PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
    if(!action->index().isValid()) {
        return;
    }
    PlacesModelBookmarkItem* item = static_cast<PlacesModelBookmarkItem*>(model_->itemFromIndex(action->index()));
    auto bookmarkItem = item->bookmark();
    Fm::Bookmarks::globalInstance()->remove(bookmarkItem);
}

// virtual
void PlacesView::commitData(QWidget* editor) {
    QTreeView::commitData(editor);
    PlacesModelBookmarkItem* item = static_cast<PlacesModelBookmarkItem*>(model_->itemFromIndex(proxyModel_->mapToSource(currentIndex())));
    auto bookmarkItem = item->bookmark();
    // rename bookmark
    Fm::Bookmarks::globalInstance()->rename(bookmarkItem, item->text());
}

void PlacesView::onOpenNewTab() {
    PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
    if(!action->index().isValid()) {
        return;
    }
    PlacesModelItem* item = static_cast<PlacesModelItem*>(model_->itemFromIndex(action->index()));
    if(item) {
        Q_EMIT chdirRequested(1, item->path());
    }
}

void PlacesView::onOpenNewWindow() {
    PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
    if(!action->index().isValid()) {
        return;
    }
    PlacesModelItem* item = static_cast<PlacesModelItem*>(model_->itemFromIndex(action->index()));
    if(item) {
        Q_EMIT chdirRequested(2, item->path());
    }
}

void PlacesView::onRenameBookmark() {
    PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
    if(!action->index().isValid()) {
        return;
    }
    PlacesModelBookmarkItem* item = static_cast<PlacesModelBookmarkItem*>(model_->itemFromIndex(action->index()));
    setFocus();
    setCurrentIndex(proxyModel_->mapFromSource(item->index()));
    edit(proxyModel_->mapFromSource(item->index()));
}

void PlacesView::onMountVolume() {
    PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
    if(!action->index().isValid()) {
        return;
    }
    PlacesModelVolumeItem* item = static_cast<PlacesModelVolumeItem*>(model_->itemFromIndex(action->index()));
    MountOperation* op = new MountOperation(true, this);
    op->mount(item->volume());
    op->wait();
}

void PlacesView::onUnmountVolume() {
    PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
    if(!action->index().isValid()) {
        return;
    }
    PlacesModelVolumeItem* item = static_cast<PlacesModelVolumeItem*>(model_->itemFromIndex(action->index()));
    MountOperation* op = new MountOperation(true, this);
    op->unmount(item->volume());
    op->wait();
}

void PlacesView::onUnmountMount() {
    PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
    if(!action->index().isValid()) {
        return;
    }
    PlacesModelMountItem* item = static_cast<PlacesModelMountItem*>(model_->itemFromIndex(action->index()));
    GMount* mount = item->mount();
    MountOperation* op = new MountOperation(true, this);
    op->unmount(mount);
    op->wait();
}

void PlacesView::onEjectVolume() {
    PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
    if(!action->index().isValid()) {
        return;
    }
    PlacesModelVolumeItem* item = static_cast<PlacesModelVolumeItem*>(model_->itemFromIndex(action->index()));
    MountOperation* op = new MountOperation(true, this);
    op->eject(item->volume());
    op->wait();
}

void PlacesView::contextMenuEvent(QContextMenuEvent* event) {
    QModelIndex index = indexAt(event->pos());
    if(index.isValid()) {
        if(index.column() != 0) { // the real item is at column 0
            index = index.sibling(index.row(), 0);
        }

        // Do not take the ownership of the menu since
        // it will be deleted with deleteLater() upon hidden.
        // This is possibly related to #145 - https://github.com/lxqt/pcmanfm-qt/issues/145
        QMenu* menu = new QMenu();
        QAction* action = nullptr;
        PlacesModelItem* item = static_cast<PlacesModelItem*>(model_->itemFromIndex(proxyModel_->mapToSource(index)));

        if(index.parent().isValid()
           && item->type() != PlacesModelItem::Mount
           && (item->type() != PlacesModelItem::Volume
               || static_cast<PlacesModelVolumeItem*>(item)->isMounted())) {
            action = new PlacesModel::ItemAction(item->index(), tr("Open in New Tab"), menu);
            connect(action, &QAction::triggered, this, &PlacesView::onOpenNewTab);
            menu->addAction(action);
            action = new PlacesModel::ItemAction(item->index(), tr("Open in New Window"), menu);
            connect(action, &QAction::triggered, this, &PlacesView::onOpenNewWindow);
            menu->addAction(action);
        }

        switch(item->type()) {
        case PlacesModelItem::Places: {
            auto path = item->path();
            // FIXME: inefficient
            if(path) {
                auto path_str = path.toString();
                if(strcmp(path_str.get(), "trash:///") == 0) {
                    action = new PlacesModel::ItemAction(item->index(), tr("Empty Trash"), menu);
                    auto icn = item->icon();
                    if(icn && icn->qicon().name() == QLatin1String("user-trash")) { // surely an empty trash
                        action->setEnabled(false);
                    }
                    else {
                        connect(action, &QAction::triggered, this, &PlacesView::onEmptyTrash);
                    }
                    // add the "Empty Trash" item on the top
                    QList<QAction*> actions = menu->actions();
                    if(!actions.isEmpty()) {
                        menu->insertAction(actions.at(0), action);
                        menu->insertSeparator(actions.at(0));
                    }
                    else { // impossible
                        menu->addAction(action);
                    }
                }
                // add a "Hide" action to the end
                menu->addSeparator();
                action = new PlacesModel::ItemAction(item->index(), tr("Hide"), menu);
                QString pathStr(path_str.get());
                action->setCheckable(true);
                if(proxyModel_->isShowingAll()) {
                    action->setChecked(proxyModel_->isHidden(pathStr));
                }
                connect(action, &QAction::triggered, [this, pathStr](bool checked) {
                    proxyModel_->setHidden(pathStr, checked);
                    Q_EMIT hiddenItemSet(pathStr, checked);
                });
                menu->addAction(action);
            }
            break;
        }
        case PlacesModelItem::Bookmark: {
            // create context menu for bookmark item
            if(item->index().row() > 0) {
                action = new PlacesModel::ItemAction(item->index(), tr("Move Bookmark Up"), menu);
                connect(action, &QAction::triggered, this, &PlacesView::onMoveBookmarkUp);
                menu->addAction(action);
            }
            if(item->index().row() < model_->rowCount()) {
                action = new PlacesModel::ItemAction(item->index(), tr("Move Bookmark Down"), menu);
                connect(action, &QAction::triggered, this, &PlacesView::onMoveBookmarkDown);
                menu->addAction(action);
            }
            action = new PlacesModel::ItemAction(item->index(), tr("Rename Bookmark"), menu);
            connect(action, &QAction::triggered, this, &PlacesView::onRenameBookmark);
            menu->addAction(action);
            action = new PlacesModel::ItemAction(item->index(), tr("Remove Bookmark"), menu);
            connect(action, &QAction::triggered, this, &PlacesView::onDeleteBookmark);
            menu->addAction(action);
            break;
        }
        case PlacesModelItem::Volume: {
            PlacesModelVolumeItem* volumeItem = static_cast<PlacesModelVolumeItem*>(item);

            if(volumeItem->isMounted()) {
                action = new PlacesModel::ItemAction(item->index(), tr("Unmount"), menu);
                connect(action, &QAction::triggered, this, &PlacesView::onUnmountVolume);
            }
            else {
                action = new PlacesModel::ItemAction(item->index(), tr("Mount"), menu);
                connect(action, &QAction::triggered, this, &PlacesView::onMountVolume);
            }
            menu->addAction(action);

            if(volumeItem->canEject()) {
                action = new PlacesModel::ItemAction(item->index(), tr("Eject"), menu);
                connect(action, &QAction::triggered, this, &PlacesView::onEjectVolume);
                menu->addAction(action);
            }
            // add a "Hide" action to the end
            CStrPtr uuid{g_volume_get_uuid(static_cast<PlacesModelVolumeItem*>(item)->volume())};
            if(uuid) {
                QString str = uuid.get();
                menu->addSeparator();
                action = new PlacesModel::ItemAction(item->index(), tr("Hide"), menu);
                action->setCheckable(true);
                if(proxyModel_->isShowingAll()) {
                    action->setChecked(proxyModel_->isHidden(str));
                }
                connect(action, &QAction::triggered, [this, str](bool checked) {
                    proxyModel_->setHidden(str, checked);
                    Q_EMIT hiddenItemSet(str, checked);
                });
                menu->addAction(action);
            }
            break;
        }
        case PlacesModelItem::Mount: {
            action = new PlacesModel::ItemAction(item->index(), tr("Unmount"), menu);
            connect(action, &QAction::triggered, this, &PlacesView::onUnmountMount);
            menu->addAction(action);
            break;
        }
        }

        // also add an acton for showing all hidden items
        if(proxyModel_->hasHidden()) {
            if(item->type() == PlacesModelItem::Bookmark) {
                menu->addSeparator();
            }
            action = new PlacesModel::ItemAction(item->index(), tr("Show All Entries"), menu);
            action->setCheckable(true);
            action->setChecked(proxyModel_->isShowingAll());
            connect(action, &QAction::triggered, [this](bool checked) {
                showAll(checked);
            });
            menu->addAction(action);
        }

        if(menu->actions().size()) {
            menu->popup(mapToGlobal(event->pos()));
            connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
        }
        else {
            menu->deleteLater();
        }
    }
}

void PlacesView::restoreHiddenItems(const QSet<QString>& items) {
    proxyModel_->restoreHiddenItems(items);
}

void PlacesView::showAll(bool show) {
    proxyModel_->showAll(show);
    if(show) {
        expandAll();
        // for some reason (a Qt bug?), spanning is reset
        setFirstColumnSpanned(0, QModelIndex(), true);
        setFirstColumnSpanned(1, QModelIndex(), true);
        setFirstColumnSpanned(2, QModelIndex(), true);
    }
}

} // namespace Fm

/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#include "placesmodel.h"
#include "icontheme.h"
#include <gio/gio.h>
#include <QDebug>
#include <QMimeData>
#include <QTimer>
#include <QPointer>
#include <QDir>
#include <QStandardPaths>
#include "utilities.h"
#include "placesmodelitem.h"

namespace Fm {

std::weak_ptr<PlacesModel> PlacesModel::globalInstance_;

PlacesModel::PlacesModel(QObject* parent):
    QStandardItemModel(parent),
    showApplications_(true),
    showDesktop_(true),
    ejectIcon_(QIcon::fromTheme("media-eject")) {
    setColumnCount(2);

    placesRoot = new QStandardItem(tr("Places"));
    placesRoot->setSelectable(false);
    placesRoot->setColumnCount(2);
    appendRow(placesRoot);

    homeItem = new PlacesModelItem("user-home", g_get_user_name(),
                                   Fm2::FilePath::fromLocalPath(QDir::homePath().toLocal8Bit().constData()));
    placesRoot->appendRow(homeItem);

    desktopItem = new PlacesModelItem("user-desktop", tr("Desktop"),
                                      Fm2::FilePath::fromLocalPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).toLocal8Bit().constData()));
    placesRoot->appendRow(desktopItem);

    createTrashItem();

    // FIXME: add an option to hide network:///
    if(true) {
        computerItem = new PlacesModelItem("computer", tr("Computer"), Fm2::FilePath::fromUri("computer:///"));
        placesRoot->appendRow(computerItem);
    }
    else {
        computerItem = NULL;
    }

    // FIXME: add an option to hide applications:///
    const char* applicaion_icon_names[] = {"system-software-install", "applications-accessories", "application-x-executable"};
    // NOTE: g_themed_icon_new_from_names() accepts char**, but actually const char** is OK.
    Fm2::GIconPtr gicon{g_themed_icon_new_from_names((char**)applicaion_icon_names, G_N_ELEMENTS(applicaion_icon_names)), false};
    auto fmicon = Fm2::Icon::fromGIcon(std::move(gicon));
    applicationsItem = new PlacesModelItem(fmicon, tr("Applications"), Fm2::FilePath::fromUri("menu:///applications.menu/"));
    placesRoot->appendRow(applicationsItem);

    // FIXME: add an option to hide network:///
    if(true) {
        const char* network_icon_names[] = {"network", "folder-network", "folder"};
        // NOTE: g_themed_icon_new_from_names() accepts char**, but actually const char** is OK.
        Fm2::GIconPtr gicon{g_themed_icon_new_from_names((char**)network_icon_names, G_N_ELEMENTS(network_icon_names)), false};
        auto fmicon = Fm2::Icon::fromGIcon(std::move(gicon));
        networkItem = new PlacesModelItem(fmicon, tr("Network"), Fm2::FilePath::fromUri("network:///"));
        placesRoot->appendRow(networkItem);
    }
    else {
        networkItem = NULL;
    }

    devicesRoot = new QStandardItem(tr("Devices"));
    devicesRoot->setSelectable(false);
    devicesRoot->setColumnCount(2);
    appendRow(devicesRoot);

    // volumes
    volumeMonitor = g_volume_monitor_get();
    if(volumeMonitor) {
        g_signal_connect(volumeMonitor, "volume-added", G_CALLBACK(onVolumeAdded), this);
        g_signal_connect(volumeMonitor, "volume-removed", G_CALLBACK(onVolumeRemoved), this);
        g_signal_connect(volumeMonitor, "volume-changed", G_CALLBACK(onVolumeChanged), this);
        g_signal_connect(volumeMonitor, "mount-added", G_CALLBACK(onMountAdded), this);
        g_signal_connect(volumeMonitor, "mount-changed", G_CALLBACK(onMountChanged), this);
        g_signal_connect(volumeMonitor, "mount-removed", G_CALLBACK(onMountRemoved), this);

        // add volumes to side-pane
        GList* vols = g_volume_monitor_get_volumes(volumeMonitor);
        GList* l;
        for(l = vols; l; l = l->next) {
            GVolume* volume = G_VOLUME(l->data);
            onVolumeAdded(volumeMonitor, volume, this);
            g_object_unref(volume);
        }
        g_list_free(vols);

        /* add mounts to side-pane */
        vols = g_volume_monitor_get_mounts(volumeMonitor);
        for(l = vols; l; l = l->next) {
            GMount* mount = G_MOUNT(l->data);
            GVolume* volume = g_mount_get_volume(mount);
            if(volume) {
                g_object_unref(volume);
            }
            else { /* network mounts or others */
                gboolean shadowed = FALSE;
#if GLIB_CHECK_VERSION(2, 20, 0)
                shadowed = g_mount_is_shadowed(mount);
#endif
                // according to gio API doc, a shadowed mount should not be visible to the user
                if(shadowed) {
                    shadowedMounts_.push_back(mount);
                    continue;
                }
                else {
                    PlacesModelItem* item = new PlacesModelMountItem(mount);
                    devicesRoot->appendRow(item);
                }
            }
            g_object_unref(mount);
        }
        g_list_free(vols);
    }

    // bookmarks
    bookmarksRoot = new QStandardItem(tr("Bookmarks"));
    bookmarksRoot->setSelectable(false);
    bookmarksRoot->setColumnCount(2);
    appendRow(bookmarksRoot);

    bookmarks = Fm2::Bookmarks::globalInstance();
    loadBookmarks();
    connect(bookmarks.get(), &Fm2::Bookmarks::changed, this, &PlacesModel::onBookmarksChanged);

    // update some icons when the icon theme is changed
    connect(IconTheme::instance(), &IconTheme::changed, this, &PlacesModel::updateIcons);
}

void PlacesModel::loadBookmarks() {
    for(auto& bm_item: bookmarks->items()) {
        PlacesModelBookmarkItem* item = new PlacesModelBookmarkItem(bm_item);
        bookmarksRoot->appendRow(item);
    }
}

PlacesModel::~PlacesModel() {
    if(volumeMonitor) {
        g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onVolumeAdded), this);
        g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onVolumeRemoved), this);
        g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onVolumeChanged), this);
        g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onMountAdded), this);
        g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onMountChanged), this);
        g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onMountRemoved), this);
        g_object_unref(volumeMonitor);
    }
    if(trashMonitor_) {
        g_signal_handlers_disconnect_by_func(trashMonitor_, (gpointer)G_CALLBACK(onTrashChanged), this);
        g_object_unref(trashMonitor_);
    }

    Q_FOREACH(GMount* mount, shadowedMounts_) {
        g_object_unref(mount);
    }
}

// static
void PlacesModel::onTrashChanged(GFileMonitor* monitor, GFile* gf, GFile* other, GFileMonitorEvent evt, PlacesModel* pThis) {
    QTimer::singleShot(0, pThis, SLOT(updateTrash()));
}

void PlacesModel::updateTrash() {

    struct UpdateTrashData {
        QPointer<PlacesModel> model;
        GFile* gf;
        UpdateTrashData(PlacesModel* _model) : model(_model) {
            gf = fm_file_new_for_uri("trash:///");
        }
        ~UpdateTrashData() {
            g_object_unref(gf);
        }
    };

    if(trashItem_) {
        UpdateTrashData* data = new UpdateTrashData(this);
        g_file_query_info_async(data->gf, G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT, G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, NULL,
        [](GObject * source_object, GAsyncResult * res, gpointer user_data) {
            // the callback lambda function is called when the asyn query operation is finished
            UpdateTrashData* data = reinterpret_cast<UpdateTrashData*>(user_data);
            PlacesModel* _this = data->model.data();
            if(_this != nullptr) { // ensure that our model object is not deleted yet
                Fm2::GFileInfoPtr inf{g_file_query_info_finish(data->gf, res, NULL), false};
                if(inf) {
                    if(_this->trashItem_ != nullptr) { // it's possible that when we finish, the trash item is removed
                        guint32 n = g_file_info_get_attribute_uint32(inf.get(), G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT);
                        const char* icon_name = n > 0 ? "user-trash-full" : "user-trash";
                        auto icon = Fm2::Icon::fromName(icon_name);
                        _this->trashItem_->setIcon(std::move(icon));
                    }
                }
            }
            delete data; // free the data used for this async operation.
        }, data);
    }
}

void PlacesModel::createTrashItem() {
    GFile* gf;
    gf = fm_file_new_for_uri("trash:///");
    // check if trash is supported by the current vfs
    // if gvfs is not installed, this can be unavailable.
    if(!g_file_query_exists(gf, NULL)) {
        g_object_unref(gf);
        trashItem_ = NULL;
        trashMonitor_ = NULL;
        return;
    }
    trashItem_ = new PlacesModelItem("user-trash", tr("Trash"), Fm2::FilePath::fromUri("trash:///"));

    trashMonitor_ = fm_monitor_directory(gf, NULL);
    if(trashMonitor_) {
        g_signal_connect(trashMonitor_, "changed", G_CALLBACK(onTrashChanged), this);
    }
    g_object_unref(gf);

    placesRoot->insertRow(desktopItem->row() + 1, trashItem_);
    QTimer::singleShot(0, this, SLOT(updateTrash()));
}

void PlacesModel::setShowApplications(bool show) {
    if(showApplications_ != show) {
        showApplications_ = show;
    }
}

void PlacesModel::setShowDesktop(bool show) {
    if(showDesktop_ != show) {
        showDesktop_ = show;
    }
}

void PlacesModel::setShowTrash(bool show) {
    if(show) {
        if(!trashItem_) {
            createTrashItem();
        }
    }
    else {
        if(trashItem_) {
            if(trashMonitor_) {
                g_signal_handlers_disconnect_by_func(trashMonitor_, (gpointer)G_CALLBACK(onTrashChanged), this);
                g_object_unref(trashMonitor_);
                trashMonitor_ = NULL;
            }
            placesRoot->removeRow(trashItem_->row()); // delete trashItem_;
            trashItem_ = NULL;
        }
    }
}

PlacesModelItem* PlacesModel::itemFromPath(const Fm2::FilePath &path) {
    PlacesModelItem* item = itemFromPath(placesRoot, path);
    if(!item) {
        item = itemFromPath(devicesRoot, path);
    }
    if(!item) {
        item = itemFromPath(bookmarksRoot, path);
    }
    return item;
}

PlacesModelItem* PlacesModel::itemFromPath(QStandardItem* rootItem, const Fm2::FilePath &path) {
    int rowCount = rootItem->rowCount();
    for(int i = 0; i < rowCount; ++i) {
        PlacesModelItem* item = static_cast<PlacesModelItem*>(rootItem->child(i, 0));
        if(item->path() == path) {
            return item;
        }
    }
    return NULL;
}

PlacesModelVolumeItem* PlacesModel::itemFromVolume(GVolume* volume) {
    int rowCount = devicesRoot->rowCount();
    for(int i = 0; i < rowCount; ++i) {
        PlacesModelItem* item = static_cast<PlacesModelItem*>(devicesRoot->child(i, 0));
        if(item->type() == PlacesModelItem::Volume) {
            PlacesModelVolumeItem* volumeItem = static_cast<PlacesModelVolumeItem*>(item);
            if(volumeItem->volume() == volume) {
                return volumeItem;
            }
        }
    }
    return NULL;
}

PlacesModelMountItem* PlacesModel::itemFromMount(GMount* mount) {
    int rowCount = devicesRoot->rowCount();
    for(int i = 0; i < rowCount; ++i) {
        PlacesModelItem* item = static_cast<PlacesModelItem*>(devicesRoot->child(i, 0));
        if(item->type() == PlacesModelItem::Mount) {
            PlacesModelMountItem* mountItem = static_cast<PlacesModelMountItem*>(item);
            if(mountItem->mount() == mount) {
                return mountItem;
            }
        }
    }
    return NULL;
}

PlacesModelBookmarkItem* PlacesModel::itemFromBookmark(std::shared_ptr<const Fm2::BookmarkItem> bkitem) {
    int rowCount = bookmarksRoot->rowCount();
    for(int i = 0; i < rowCount; ++i) {
        PlacesModelBookmarkItem* item = static_cast<PlacesModelBookmarkItem*>(bookmarksRoot->child(i, 0));
        if(item->bookmark() == bkitem) {
            return item;
        }
    }
    return NULL;
}

void PlacesModel::onMountAdded(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis) {
    // according to gio API doc, a shadowed mount should not be visible to the user
#if GLIB_CHECK_VERSION(2, 20, 0)
    if(g_mount_is_shadowed(mount)) {
        if(pThis->shadowedMounts_.indexOf(mount) == -1) {
            pThis->shadowedMounts_.push_back(G_MOUNT(g_object_ref(mount)));
        }
        return;
    }
#endif
    GVolume* vol = g_mount_get_volume(mount);
    if(vol) { // mount-added is also emitted when a volume is newly mounted.
        PlacesModelVolumeItem* item = pThis->itemFromVolume(vol);
        if(item && !item->path()) {
            // update the mounted volume and show a button for eject.
            Fm2::FilePath path{g_mount_get_root(mount), false};
            item->setPath(path);
            // update the mount indicator (eject button)
            QStandardItem* ejectBtn = item->parent()->child(item->row(), 1);
            Q_ASSERT(ejectBtn);
            ejectBtn->setIcon(pThis->ejectIcon_);
        }
        g_object_unref(vol);
    }
    else { // network mounts and others
        PlacesModelMountItem* item = pThis->itemFromMount(mount);
        /* for some unknown reasons, sometimes we get repeated mount-added
         * signals and added a device more than one. So, make a sanity check here. */
        if(!item) {
            item = new PlacesModelMountItem(mount);
            QStandardItem* eject_btn = new QStandardItem(pThis->ejectIcon_, QString());
            pThis->devicesRoot->appendRow(QList<QStandardItem*>() << item << eject_btn);
        }
    }
}

void PlacesModel::onMountChanged(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis) {
    gboolean shadowed = FALSE;
    // according to gio API doc, a shadowed mount should not be visible to the user
#if GLIB_CHECK_VERSION(2, 20, 0)
    shadowed = g_mount_is_shadowed(mount);
    // qDebug() << "changed:" << mount << shadowed;
#endif
    PlacesModelMountItem* item = pThis->itemFromMount(mount);
    if(item) {
        if(shadowed) { // if a visible item becomes shadowed, remove it from the model
            pThis->shadowedMounts_.push_back(G_MOUNT(g_object_ref(mount))); // remember the shadowed mount
            pThis->devicesRoot->removeRow(item->row());
        }
        else { // otherwise, update its status
            item->update();
        }
    }
    else {
#if GLIB_CHECK_VERSION(2, 20, 0)
        if(!shadowed) { // if a mount is unshadowed
            int i = pThis->shadowedMounts_.indexOf(mount);
            if(i != -1) { // a previously shadowed mount is unshadowed
                pThis->shadowedMounts_.removeAt(i);
                onMountAdded(monitor, mount, pThis); // add it to our model again
            }
        }
#endif
    }
}

void PlacesModel::onMountRemoved(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis) {
    GVolume* vol = g_mount_get_volume(mount);
    // qDebug() << "mount removed" << mount << "volume umounted: " << vol;
    if(vol) {
        // a volume is being unmounted
        // NOTE: Due to some problems of gvfs, sometimes the volume does not receive
        // "change" signal so it does not update the eject button. Let's workaround
        // this by calling onVolumeChanged() handler manually. (This is needed for mtp://)
        onVolumeChanged(monitor, vol, pThis);
        g_object_unref(vol);
    }
    else { // network mounts and others
        PlacesModelMountItem* item = pThis->itemFromMount(mount);
        if(item) {
            pThis->devicesRoot->removeRow(item->row());
        }
    }

#if GLIB_CHECK_VERSION(2, 20, 0)
    // NOTE: g_mount_is_shadowed() sometimes returns FALSE here even if the mount is shadowed.
    // I don't know whether this is a bug in gvfs or not.
    // So let's check if its in our list instead.
    if(pThis->shadowedMounts_.removeOne(mount)) {
        // if this is a shadowed mount
        // qDebug() << "remove shadow mount";
        g_object_unref(mount);
    }
#endif
}

void PlacesModel::onVolumeAdded(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis) {
    // for some unknown reasons, sometimes we get repeated volume-added
    // signals and added a device more than one. So, make a sanity check here.
    PlacesModelVolumeItem* volumeItem = pThis->itemFromVolume(volume);
    if(!volumeItem) {
        volumeItem = new PlacesModelVolumeItem(volume);
        QStandardItem* ejectBtn = new QStandardItem();
        if(volumeItem->isMounted()) {
            ejectBtn->setIcon(pThis->ejectIcon_);
        }
        pThis->devicesRoot->appendRow(QList<QStandardItem*>() << volumeItem << ejectBtn);
    }
}

void PlacesModel::onVolumeChanged(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis) {
    PlacesModelVolumeItem* item = pThis->itemFromVolume(volume);
    if(item) {
        item->update();
        if(!item->isMounted()) { // the volume is unmounted, remove the eject button if needed
            // remove the eject button for the volume (at column 1 of the same row)
            QStandardItem* ejectBtn = item->parent()->child(item->row(), 1);
            Q_ASSERT(ejectBtn);
            ejectBtn->setIcon(QIcon());
        }
    }
}

void PlacesModel::onVolumeRemoved(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis) {
    PlacesModelVolumeItem* item = pThis->itemFromVolume(volume);
    if(item) {
        pThis->devicesRoot->removeRow(item->row());
    }
}

void PlacesModel::onBookmarksChanged() {
    // remove all items
    bookmarksRoot->removeRows(0, bookmarksRoot->rowCount());
    loadBookmarks();
}

void PlacesModel::updateIcons() {
    // the icon theme is changed and we need to update the icons
    PlacesModelItem* item;
    int row;
    int n = placesRoot->rowCount();
    for(row = 0; row < n; ++row) {
        item = static_cast<PlacesModelItem*>(placesRoot->child(row));
        item->updateIcon();
    }
    n = devicesRoot->rowCount();
    for(row = 0; row < n; ++row) {
        item = static_cast<PlacesModelItem*>(devicesRoot->child(row));
        item->updateIcon();
    }
}

Qt::ItemFlags PlacesModel::flags(const QModelIndex& index) const {
    if(index.column() == 1) { // make 2nd column of every row selectable.
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
    if(!index.parent().isValid()) { // root items
        if(index.row() == 2) { // bookmarks root
            return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
        }
        else {
            return Qt::ItemIsEnabled;
        }
    }
    return QStandardItemModel::flags(index);
}


QVariant PlacesModel::data(const QModelIndex& index, int role) const {
    if(index.column() == 0 && index.parent().isValid()) {
        PlacesModelItem* item = static_cast<PlacesModelItem*>(QStandardItemModel::itemFromIndex(index));
        if(item != nullptr) {
            switch(role) {
            // FIXME: can we use smart pointers here?
            case FileInfoRole:
                return QVariant::fromValue<void*>((void*)item->fileInfo().get());
            case FmIconRole:
                return QVariant::fromValue<void*>((void*)item->icon().get());
            }
        }
    }
    return QStandardItemModel::data(index, role);
}

std::shared_ptr<PlacesModel> PlacesModel::globalInstance() {
    auto model = globalInstance_.lock();
    if(!model) {
        model = std::make_shared<PlacesModel>();
        globalInstance_ = model;
    }
    return model;
}


bool PlacesModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
    QStandardItem* item = itemFromIndex(parent);
    if(data->hasFormat("application/x-bookmark-row")) { // the data being dopped is a bookmark row
        // decode it and do bookmark reordering
        QByteArray buf = data->data("application/x-bookmark-row");
        QDataStream stream(&buf, QIODevice::ReadOnly);
        int oldPos = -1;
        char* pathStr = NULL;
        stream >> oldPos >> pathStr;
        // find the source bookmark item being dragged
        auto allBookmarks = bookmarks->items();
        auto& draggedItem = allBookmarks[oldPos];
        // If we cannot find the dragged bookmark item at position <oldRow>, or we find an item,
        // but the path of the item is not the same as what we expected, than it's the wrong item.
        // This means that the bookmarks are changed during our dnd processing, which is an extremely rare case.
        auto draggedPath = Fm2::FilePath{pathStr};
        if(!draggedItem || draggedItem->path() != draggedPath) {
            delete []pathStr;
            return false;
        }
        delete []pathStr;

        int newPos = -1;
        if(row == -1 && column == -1) { // drop on an item
            // we only allow dropping on an bookmark item
            if(item && item->parent() == bookmarksRoot) {
                newPos = parent.row();
            }
        }
        else { // drop on a position between items
            if(item == bookmarksRoot) { // we only allow dropping on a bookmark item
                newPos = row;
            }
        }
        if(newPos != -1 && newPos != oldPos) { // reorder the bookmark item
            bookmarks->reorder(draggedItem, newPos);
        }
    }
    else if(data->hasUrls()) { // files uris are dropped
        if(row == -1 && column == -1) { // drop uris on an item
            if(item && item->parent()) { // need to be a child item
                PlacesModelItem* placesItem = static_cast<PlacesModelItem*>(item);
                if(placesItem->path()) {
                    qDebug() << "dropped dest:" << placesItem->text();
                    // TODO: copy or move the dragged files to the dir pointed by the item.
                    qDebug() << "drop on" << item->text();
                }
            }
        }
        else { // drop uris on a position between items
            if(item == bookmarksRoot) { // we only allow dropping on blank row of bookmarks section
                auto paths = pathListFromQUrls(data->urls());
                for(auto& path: paths) {
                    // FIXME: this is a blocking call
                    if(g_file_query_file_type(path.gfile().get(), G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                              NULL) == G_FILE_TYPE_DIRECTORY) {
                        auto disp_name = path.baseName();
                        bookmarks->insert(path, disp_name.get(), row);
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

// we only support dragging bookmark items and use our own
// custom pseudo-mime-type: application/x-bookmark-row
QMimeData* PlacesModel::mimeData(const QModelIndexList& indexes) const {
    if(!indexes.isEmpty()) {
        // we only allow dragging one bookmark item at a time, so handle the first index only.
        QModelIndex index = indexes.first();
        QStandardItem* item = itemFromIndex(index);
        // ensure that it's really a bookmark item
        if(item && item->parent() == bookmarksRoot) {
            PlacesModelBookmarkItem* bookmarkItem = static_cast<PlacesModelBookmarkItem*>(item);
            QMimeData* mime = new QMimeData();
            QByteArray data;
            QDataStream stream(&data, QIODevice::WriteOnly);
            // There is no safe and cross-process way to store a reference of a row.
            // Let's store the pos, name, and path of the bookmark item instead.
            auto pathStr = bookmarkItem->path().toString();
            stream << index.row() << pathStr.get();
            mime->setData("application/x-bookmark-row", data);
            return mime;
        }
    }
    return NULL;
}

QStringList PlacesModel::mimeTypes() const {
    return QStringList() << "application/x-bookmark-row" << "text/uri-list";
}

Qt::DropActions PlacesModel::supportedDropActions() const {
    return QStandardItemModel::supportedDropActions();
}


} // namespace Fm

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

#include "volumelistmodel.h"
#include "foldermodel.h"

namespace Fm {

VolumeListModel::VolumeListModel(QObject* parent):
    QAbstractTableModel(parent),
    volumeManager_{VolumeManager::globalInstance()} {
    volumes_ = volumeManager_->volumes();
    connect(volumeManager_.get(), &VolumeManager::volumeAdded, this, &VolumeListModel::onVolumeAdded);
    connect(volumeManager_.get(), &VolumeManager::volumeRemoved, this, &VolumeListModel::onVolumeRemoved);
    connect(volumeManager_.get(), &VolumeManager::volumeChanged, this, &VolumeListModel::onVolumeChanged);
    connect(volumeManager_.get(), &VolumeManager::mountAdded, this, &VolumeListModel::onMountChanged);
    connect(volumeManager_.get(), &VolumeManager::mountRemoved, this, &VolumeListModel::onMountChanged);
    connect(volumeManager_.get(), &VolumeManager::mountChanged, this, &VolumeListModel::onMountChanged);
}

VolumeListModel::~VolumeListModel() {
}

int VolumeListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(volumes_.size());
}

int VolumeListModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : FolderModel::NumOfColumns;
}

QVariant VolumeListModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid() || index.row() >= static_cast<int>(volumes_.size())) {
        return QVariant();
    }
    const Volume& vol = volumes_.at(static_cast<size_t>(index.row()));
    if(role == VolumeRole) { // every column, so a row selected by any cell is recognized
        return QVariant::fromValue(std::make_shared<const Volume>(vol));
    }
    if(index.column() != FolderModel::ColumnFileName) {
        return QVariant();
    }
    switch(role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        if(auto name = vol.name()) {
            return QString::fromUtf8(name.get());
        }
        return QVariant();
    case Qt::DecorationRole: {
        auto icon = vol.icon();
        return icon ? QVariant(icon->qicon()) : QVariant();
    }
    default:
        // deliberately invalid for everything else, FolderModel::FileInfoRole
        // included -- that's how callers tell a volume row from a file row.
        return QVariant();
    }
}

Qt::ItemFlags VolumeListModel::flags(const QModelIndex& index) const {
    if(!index.isValid()) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if(index.column() == FolderModel::ColumnFileName) { // like FolderModel::flags()
        f |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
    return f;
}

QStringList VolumeListModel::mimeTypes() const {
    return {QStringLiteral("text/uri-list"), QStringLiteral("libfm/files")};
}

Qt::DropActions VolumeListModel::supportedDropActions() const {
    return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

bool VolumeListModel::dropMimeData(const QMimeData* /*data*/, Qt::DropAction /*action*/, int /*row*/, int /*column*/, const QModelIndex& /*parent*/) {
    return false;
}

void VolumeListModel::onVolumeAdded(const Volume& vol) {
    int row = static_cast<int>(volumes_.size());
    beginInsertRows(QModelIndex(), row, row);
    volumes_.push_back(vol);
    endInsertRows();
}

// GVolumeMonitor hands out the same GVolume object for a volume's whole life,
// so pointer identity is the reliable key (uuid/device can be missing or shared)
void VolumeListModel::onVolumeRemoved(const Volume& vol) {
    for(size_t i = 0; i < volumes_.size(); ++i) {
        if(volumes_[i].get() == vol.get()) {
            beginRemoveRows(QModelIndex(), static_cast<int>(i), static_cast<int>(i));
            volumes_.erase(volumes_.begin() + static_cast<long>(i));
            endRemoveRows();
            break;
        }
    }
}

void VolumeListModel::onVolumeChanged(const Volume& vol) {
    emitRowChanged(vol.get());
}

void VolumeListModel::onMountChanged(const Mount& mnt) {
    if(auto gvol = mnt.volume()) {
        emitRowChanged(gvol.get());
    }
}

void VolumeListModel::emitRowChanged(GVolume* gvol) {
    for(size_t i = 0; i < volumes_.size(); ++i) {
        if(volumes_[i].get() == gvol) {
            const int row = static_cast<int>(i);
            Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
            break;
        }
    }
}

}

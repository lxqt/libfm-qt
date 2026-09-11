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

namespace Fm {

// stable identity for a volume, used to find its row again on removal/mount changes
static std::string volumeId(const Volume& vol) {
    if(auto uuid = vol.uuid()) {
        return uuid.get();
    }
    if(auto dev = vol.device()) {
        return dev.get();
    }
    return std::string();
}

VolumeListModel::VolumeListModel(QObject* parent):
    QAbstractListModel(parent),
    volumeManager_{VolumeManager::globalInstance()} {
    volumes_ = volumeManager_->volumes();
    connect(volumeManager_.get(), &VolumeManager::volumeAdded, this, &VolumeListModel::onVolumeAdded);
    connect(volumeManager_.get(), &VolumeManager::volumeRemoved, this, &VolumeListModel::onVolumeRemoved);
    connect(volumeManager_.get(), &VolumeManager::mountAdded, this, &VolumeListModel::onMountChanged);
    connect(volumeManager_.get(), &VolumeManager::mountRemoved, this, &VolumeListModel::onMountChanged);
}

VolumeListModel::~VolumeListModel() {
}

int VolumeListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(volumes_.size());
}

QVariant VolumeListModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid() || index.row() >= static_cast<int>(volumes_.size())) {
        return QVariant();
    }
    const Volume& vol = volumes_.at(static_cast<size_t>(index.row()));
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
    case VolumeRole:
        return QVariant::fromValue(std::make_shared<const Volume>(vol));
    default:
        // deliberately invalid for everything else, FolderModel::FileInfoRole
        // included -- that's how callers tell a volume row from a file row.
        return QVariant();
    }
}

void VolumeListModel::onVolumeAdded(const Volume& vol) {
    int row = static_cast<int>(volumes_.size());
    beginInsertRows(QModelIndex(), row, row);
    volumes_.push_back(vol);
    endInsertRows();
}

void VolumeListModel::onVolumeRemoved(const Volume& vol) {
    const auto id = volumeId(vol);
    for(size_t i = 0; i < volumes_.size(); ++i) {
        if(volumeId(volumes_[i]) == id) {
            beginRemoveRows(QModelIndex(), static_cast<int>(i), static_cast<int>(i));
            volumes_.erase(volumes_.begin() + static_cast<long>(i));
            endRemoveRows();
            break;
        }
    }
}

void VolumeListModel::onMountChanged(const Mount& mnt) {
    auto gvol = mnt.volume();
    if(!gvol) {
        return;
    }
    const auto id = volumeId(Volume{gvol});
    for(size_t i = 0; i < volumes_.size(); ++i) {
        if(volumeId(volumes_[i]) == id) {
            QModelIndex idx = index(static_cast<int>(i), 0);
            Q_EMIT dataChanged(idx, idx);
            break;
        }
    }
}

}

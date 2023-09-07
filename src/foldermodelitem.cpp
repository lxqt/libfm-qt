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


#include "foldermodelitem.h"
#include <QDateTime>
#include <QPainter>
#include "utilities.h"
#include "core/userinfocache.h"

namespace Fm {

FolderModelItem::FolderModelItem(const std::shared_ptr<const Fm::FileInfo>& _info):
    info{_info},
    isCut{false} {
    thumbnails.reserve(2);
}

FolderModelItem::FolderModelItem(const FolderModelItem& other):
    info{other.info},
    thumbnails{other.thumbnails},
    isCut{other.isCut} {
}

FolderModelItem::~FolderModelItem() {
}

QString FolderModelItem::ownerName() const {
    QString name;
    auto user = Fm::UserInfoCache::globalInstance()->userFromId(info->uid());
    if(user) {
        name = user->name();
    }
    return name;
}

QString FolderModelItem::ownerGroup() const {
    auto group = Fm::UserInfoCache::globalInstance()->groupFromId(info->gid());
    return group ? group->name() : QString();
}

const QString &FolderModelItem::displayMtime() const {
    if(dispMtime_.isEmpty()) {
        if(info->mtime() == 0) {
            dispMtime_ = QObject::tr("N/A");
        }
        else {
            auto mtime = QDateTime::fromMSecsSinceEpoch(info->mtime() * 1000);
            dispMtime_ = QLocale().toString(mtime, QLocale::ShortFormat);
        }
    }
    return dispMtime_;
}

const QString &FolderModelItem::displayCrtime() const {
    if(dispCrtime_.isEmpty()) {
        if(info->crtime() == 0) {
            dispCrtime_ = QObject::tr("N/A");
        }
        else {
            auto crtime = QDateTime::fromMSecsSinceEpoch(info->crtime() * 1000);
            dispCrtime_ = QLocale().toString(crtime, QLocale::ShortFormat);
        }
    }
    return dispCrtime_;
}

const QString &FolderModelItem::displayDtime() const {
    if(dispDtime_.isEmpty() && info->dtime() > 0) {
        auto dtime = QDateTime::fromMSecsSinceEpoch(info->dtime() * 1000);
        dispDtime_ = QLocale().toString(dtime, QLocale::ShortFormat);
    }
    return dispDtime_;
}

const QString& FolderModelItem::displaySize() const {
    if(!info->isDir()) {
        // FIXME: choose IEC or SI units
        dispSize_ = Fm::formatFileSize(info->size(), false);
    }
    return dispSize_;
}

// find thumbnail of the specified size
// The returned thumbnail item is temporary and short-lived
// If you need to use the struct later, copy it to your own struct to keep it.
FolderModelItem::Thumbnail* FolderModelItem::findThumbnail(int size) {
    QList<Thumbnail>::iterator it;
    for(it = thumbnails.begin(); it != thumbnails.end(); ++it) {
        if(it->size == size) { // an image of the same size is found
            return &(*it);
        }
    }
    if(it == thumbnails.end()) {
        Thumbnail thumbnail;
        thumbnail.status = ThumbnailNotChecked;
        thumbnail.size = size;
        thumbnails.append(thumbnail);
    }
    return &thumbnails.back();
}

// remove cached thumbnail of the specified size
void FolderModelItem::removeThumbnail(int size) {
    QList<Thumbnail>::iterator it;
    for(it = thumbnails.begin(); it != thumbnails.end(); ++it) {
        if(it->size == size) { // an image of the same size is found
            thumbnails.erase(it);
            break;
        }
    }
}


} // namespace Fm

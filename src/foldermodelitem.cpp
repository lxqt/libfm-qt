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
    info{_info} {
    thumbnails.reserve(2);
}

FolderModelItem::FolderModelItem(const FolderModelItem& other):
    info{other.info},
    thumbnails{other.thumbnails} {
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
        auto mtime = QDateTime::fromMSecsSinceEpoch(info->mtime() * 1000);
        dispMtime_ = mtime.toString(Qt::SystemLocaleShortDate);
    }
    return dispMtime_;
}

const QString& FolderModelItem::displaySize() const {
    if(!info->isDir()) {
        // FIXME: choose IEC or SI units
        dispSize_ = Fm::formatFileSize(info->size(), false);
    }
    return dispSize_;
}

bool FolderModelItem::isCut() const {
    return !cutFilesHashSet_.expired() || info->isCut();
}

void FolderModelItem::bindCutFiles(const std::shared_ptr<const HashSet>& cutFilesHashSet) {
    cutFilesHashSet_ = cutFilesHashSet;
}

// find thumbnail of the specified size
// The returned thumbnail item is temporary and short-lived
// If you need to use the struct later, copy it to your own struct to keep it.
FolderModelItem::Thumbnail* FolderModelItem::findThumbnail(int size, bool transparent) {
    QVector<Thumbnail>::iterator it;
    Thumbnail* transThumb = nullptr;
    for(it = thumbnails.begin(); it != thumbnails.end(); ++it) {
        if(it->size == size) {
            if(it->status != ThumbnailLoaded) {
                return it;
            }
            else { // it->status == ThumbnailLoaded
                if(it->transparent == false && transparent == true
                        && size < 48 /* (dirty) needed only for 'compact' and 'details list' view */ ) {
                    transThumb = it; // save thumb to add transparency later
                }
                else {
                    return it; // an image of the same size and transparency is found
                }
            }
        }
    }
    if(transThumb) {
        QImage image(transThumb->image);

        if(!image.hasAlphaChannel()) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }

        // add transparency to image
        QPainter p;
        p.begin(&image);
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.fillRect(image.rect(), QColor(0, 0, 0, 115 /* alpha 45% */));
        p.end();

        // add image to thumbnails
        Thumbnail thumbnail;
        thumbnail.status = ThumbnailLoaded;
        thumbnail.image = image;
        thumbnail.size = size;
        thumbnail.transparent = true;
        thumbnails.append(thumbnail);
    }
    else if(it == thumbnails.end()) {
        Thumbnail thumbnail;
        thumbnail.status = ThumbnailNotChecked;
        thumbnail.size = size;
        thumbnail.transparent = false;
        thumbnails.append(thumbnail);
    }
    return &thumbnails.back();
}

// remove cached thumbnail of the specified size
void FolderModelItem::removeThumbnail(int size) {
    QVector<Thumbnail>::iterator it;
    for(it = thumbnails.begin(); it != thumbnails.end(); ++it) {
        if(it->size == size) { // an image of the same size is found
            thumbnails.erase(it);
            break;
        }
    }
}


} // namespace Fm

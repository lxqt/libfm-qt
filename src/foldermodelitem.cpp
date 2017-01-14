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

namespace Fm {

FolderModelItem::FolderModelItem(const std::shared_ptr<const Fm2::FileInfo>& _info):
    info{_info} {
    thumbnails.reserve(2);
}

FolderModelItem::FolderModelItem(const FolderModelItem& other):
    info{other.info},
    thumbnails{other.thumbnails} {
}

FolderModelItem::~FolderModelItem() {
}

// find thumbnail of the specified size
// The returned thumbnail item is temporary and short-lived
// If you need to use the struct later, copy it to your own struct to keep it.
FolderModelItem::Thumbnail* FolderModelItem::findThumbnail(int size) {
    QVector<Thumbnail>::iterator it;
    for(it = thumbnails.begin(); it != thumbnails.end(); ++it) {
        if(it->size == size) { // an image of the same size is found
            return it;
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
    QVector<Thumbnail>::iterator it;
    for(it = thumbnails.begin(); it != thumbnails.end(); ++it) {
        if(it->size == size) { // an image of the same size is found
            thumbnails.erase(it);
            break;
        }
    }
}

#if 0
// cache the thumbnail of the specified size in the folder item
void FolderModelItem::setThumbnail(int size, QImage image) {
    QVector<Thumbnail>::iterator it;
    for(it = thumbnails.begin(); it != thumbnails.end(); ++it) {
        if(it->size == size) { // an image of the same size already exists
            it->image = image; // replace it
            it->status = ThumbnailLoaded;
            break;
        }
    }
    if(it == thumbnails.end()) { // the image is not found
        Thumbnail thumbnail;
        thumbnail.size = size;
        thumbnail.status = ThumbnailLoaded;
        thumbnail.image = image;
        thumbnails.append(thumbnail); // add a new entry
    }
}
#endif


} // namespace Fm

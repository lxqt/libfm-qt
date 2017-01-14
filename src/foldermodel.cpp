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


#include "foldermodel.h"
#include "icontheme.h"
#include <iostream>
#include <QtAlgorithms>
#include <QVector>
#include <qmimedata.h>
#include <QMimeData>
#include <QByteArray>
#include <QPixmap>
#include <QPainter>
#include "utilities.h"
#include "fileoperation.h"
#include "thumbnailloader.h"

namespace Fm {

FolderModel::FolderModel() {
/*
    ColumnIcon,
    ColumnName,
    ColumnFileType,
    ColumnMTime,
    NumOfColumns
*/
  thumbnailRefCounts.reserve(4);
}

FolderModel::~FolderModel() {
  qDebug("delete FolderModel");

  // if the thumbnail requests list is not empty, cancel them
  if(!thumbnailResults.empty()) {
    Q_FOREACH(FmThumbnailLoader* res, thumbnailResults) {
      ThumbnailLoader::cancel(res);
    }
  }
}

void FolderModel::setFolder(const std::shared_ptr<Fm2::Folder> &new_folder) {
  if(folder_) {
    removeAll();        // remove old items
  }
  if(new_folder) {
    folder_ = new_folder;
    connect(folder_.get(), &Fm2::Folder::startLoading, this, &FolderModel::onStartLoading);
    connect(folder_.get(), &Fm2::Folder::finishLoading, this, &FolderModel::onFinishLoading);
    connect(folder_.get(), &Fm2::Folder::filesAdded, this, &FolderModel::onFilesAdded);
    connect(folder_.get(), &Fm2::Folder::filesChanged, this, &FolderModel::onFilesChanged);
    connect(folder_.get(), &Fm2::Folder::filesRemoved, this, &FolderModel::onFilesRemoved);
    // handle the case if the folder is already loaded
    if(folder_->isLoaded()) {
        insertFiles(0, folder_->getFiles());
    }
  }
}

void FolderModel::onStartLoading() {
  // remove all items
  removeAll();
}

void FolderModel::onFinishLoading() {
}

void FolderModel::onFilesAdded(const Fm2::FileInfoList &files) {
  int n_files = files.size();
  beginInsertRows(QModelIndex(), items.count(), items.count() + n_files - 1);
  for(auto& info: files) {
    FolderModelItem item(info);
/*
    if(fm_file_info_is_hidden(info)) {
      model->hiddenItems.append(item);
      continue;
    }
*/
    items.append(item);
  }
  endInsertRows();
}

//static
void FolderModel::onFilesChanged(std::vector<Fm2::FileInfoPair>& files) {
  for(auto& change: files) {
    int row;
    auto& oldInfo = change.first;
    auto& newInfo = change.second;
    QList<FolderModelItem>::iterator it = findItemByFileInfo(oldInfo.get(), &row);
    if(it != items.end()) {
      FolderModelItem& item = *it;
      // try to update the item
      item.info = newInfo;
      item.thumbnails.clear();
      QModelIndex index = createIndex(row, 0, &item);
      Q_EMIT dataChanged(index, index);
    }
  }
}

//static
void FolderModel::onFilesRemoved(const Fm2::FileInfoList &files) {
  for(auto& info: files) {
    int row;
    QList<FolderModelItem>::iterator it = findItemByName(info->getName().c_str(), &row);
    if(it != items.end()) {
      beginRemoveRows(QModelIndex(), row, row);
      items.erase(it);
      endRemoveRows();
    }
  }
}

void FolderModel::insertFiles(int row, const Fm2::FileInfoList& files) {
  int n_files = files.size();
  beginInsertRows(QModelIndex(), row, row + n_files - 1);
  for(auto& info: files) {
    FolderModelItem item(info);
    items.append(item);
  }
  endInsertRows();
}

void FolderModel::removeAll() {
  if(items.empty())
    return;
  beginRemoveRows(QModelIndex(), 0, items.size() - 1);
  items.clear();
  endRemoveRows();
}

int FolderModel::rowCount(const QModelIndex & parent) const {
  if(parent.isValid())
    return 0;
  return items.size();
}

int FolderModel::columnCount (const QModelIndex & parent = QModelIndex()) const {
  if(parent.isValid())
    return 0;
  return NumOfColumns;
}

FolderModelItem* FolderModel::itemFromIndex(const QModelIndex& index) const {
  return reinterpret_cast<FolderModelItem*>(index.internalPointer());
}

std::shared_ptr<const Fm2::FileInfo> FolderModel::fileInfoFromIndex(const QModelIndex& index) const {
  FolderModelItem* item = itemFromIndex(index);
  return item ? item->info : nullptr;
}

QVariant FolderModel::data(const QModelIndex & index, int role/* = Qt::DisplayRole*/) const {
  if(!index.isValid() || index.row() > items.size() || index.column() >= NumOfColumns) {
    return QVariant();
  }
  FolderModelItem* item = itemFromIndex(index);
  auto info = item->info;

  switch(role) {
    case Qt::ToolTipRole:
      return QVariant(item->displayName());
    case Qt::DisplayRole:  {
      switch(index.column()) {
        case ColumnFileName: {
          return QVariant(item->displayName());
        }
        case ColumnFileType: {
          auto mime = info->getMimeType();
          return QString::fromUtf8(mime->desc());
        }
        case ColumnFileMTime: {
          // FIXME:
          // const char* name = fm_file_info_get_disp_mtime(info);
          // return QString::fromUtf8(name);
        }
        case ColumnFileSize: {
          // FIXME:
          // const char* name = fm_file_info_get_disp_size(info);
          // return QString::fromUtf8(name);
        }
        case ColumnFileOwner: {
          // FIXME:
          // const char* name = fm_file_info_get_disp_owner(info);
          // return QString::fromUtf8(name);
        }
      }
    }
    case Qt::DecorationRole: {
      if(index.column() == 0) {
        return QVariant(item->icon());
      }
      break;
    }
    case FileInfoRole:
      return qVariantFromValue((void*)info.get());
  }
  return QVariant();
}

QVariant FolderModel::headerData(int section, Qt::Orientation orientation, int role/* = Qt::DisplayRole*/) const {
  if(role == Qt::DisplayRole) {
    if(orientation == Qt::Horizontal) {
      QString title;
      switch(section) {
        case ColumnFileName:
          title = tr("Name");
          break;
        case ColumnFileType:
          title = tr("Type");
          break;
        case ColumnFileSize:
          title = tr("Size");
          break;
        case ColumnFileMTime:
          title = tr("Modified");
          break;
        case ColumnFileOwner:
          title = tr("Owner");
          break;
      }
      return QVariant(title);
    }
  }
  return QVariant();
}

QModelIndex FolderModel::index(int row, int column, const QModelIndex & parent) const {
  if(row <0 || row >= items.size() || column < 0 || column >= NumOfColumns)
    return QModelIndex();
  const FolderModelItem& item = items.at(row);
  return createIndex(row, column, (void*)&item);
}

QModelIndex FolderModel::parent(const QModelIndex & index) const {
  return QModelIndex();
}

Qt::ItemFlags FolderModel::flags(const QModelIndex& index) const {
  // FIXME: should not return same flags unconditionally for all columns
  Qt::ItemFlags flags;
  if(index.isValid()) {
    flags = Qt::ItemIsEnabled|Qt::ItemIsSelectable;
    if(index.column() == ColumnFileName)
      flags |= (Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled);
  }
  else {
    flags = Qt::ItemIsDropEnabled;
  }
  return flags;
}

// FIXME: this is very inefficient and should be replaced with a
// more reasonable implementation later.
QList<FolderModelItem>::iterator FolderModel::findItemByPath(const Fm2::FilePath &path, int* row) {
  QList<FolderModelItem>::iterator it = items.begin();
  int i = 0;
  while(it != items.end()) {
    FolderModelItem& item = *it;
    auto item_path = item.info->path();
    if(item_path == path) {
      *row = i;
      return it;
    }
    ++it;
    ++i;
  }
  return items.end();
}

// FIXME: this is very inefficient and should be replaced with a
// more reasonable implementation later.
QList<FolderModelItem>::iterator FolderModel::findItemByName(const char* name, int* row) {
  QList<FolderModelItem>::iterator it = items.begin();
  int i = 0;
  while(it != items.end()) {
    FolderModelItem& item = *it;
    if(item.info->getName() == name) {
      *row = i;
      return it;
    }
    ++it;
    ++i;
  }
  return items.end();
}

QList< FolderModelItem >::iterator FolderModel::findItemByFileInfo(const Fm2::FileInfo* info, int* row) {
  QList<FolderModelItem>::iterator it = items.begin();
  int i = 0;
  while(it != items.end()) {
    FolderModelItem& item = *it;
    if(item.info.get() == info) {
      *row = i;
      return it;
    }
    ++it;
    ++i;
  }
  return items.end();
}

QStringList FolderModel::mimeTypes() const {
  qDebug("FolderModel::mimeTypes");
  QStringList types = QAbstractItemModel::mimeTypes();
  // now types contains "application/x-qabstractitemmodeldatalist"

  // add support for freedesktop Xdnd direct save (XDS) protocol.
  // http://www.freedesktop.org/wiki/Specifications/XDS/#index4h2
  // the real implementation is in FolderView::childDropEvent().
  types << "XdndDirectSave0";
  types << "text/uri-list";
  // types << "x-special/gnome-copied-files";
  return types;
}

QMimeData* FolderModel::mimeData(const QModelIndexList& indexes) const {
  QMimeData* data = QAbstractItemModel::mimeData(indexes);
  qDebug("FolderModel::mimeData");
  // build a uri list
  QByteArray urilist;
  urilist.reserve(4096);

  for(const auto &index : indexes) {
    FolderModelItem* item = itemFromIndex(index);
    if(item && item->info) {
      auto path = item->info->path();
      if(path.isValid()) {
        auto uri = path.uri();
        urilist.append(uri.get());
        urilist.append('\n');
      }
    }
  }
  data->setData("text/uri-list", urilist);

  return data;
}

bool FolderModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
  qDebug("FolderModel::dropMimeData");
  if(!folder_)
    return false;
  Fm2::FilePath destPath;
  if(parent.isValid()) { // drop on an item
    std::shared_ptr<const Fm2::FileInfo> info;
    if(row == -1 && column == -1)
      info = fileInfoFromIndex(parent);
    else {
      QModelIndex itemIndex = parent.child(row, column);
      info = fileInfoFromIndex(itemIndex);
    }
    if(info)
      destPath = info->path();
    else
      return false;
  }
  else { // drop on blank area of the folder
    destPath = path();
  }

  // FIXME: should we put this in dropEvent handler of FolderView instead?
  if(data->hasUrls()) {
    // FIXME: port this to new Fm2 APIs
    qDebug("drop action: %d", action);
    FmPathList* srcPaths = pathListFromQUrls(data->urls());
    FmPath* _destPath = fm_path_new_for_gfile(destPath.gfile().get());
    switch(action) {
      case Qt::CopyAction:
        FileOperation::copyFiles(srcPaths, _destPath);
        break;
      case Qt::MoveAction:
        FileOperation::moveFiles(srcPaths, _destPath);
        break;
      case Qt::LinkAction:
        FileOperation::symlinkFiles(srcPaths, _destPath);
      default:
        fm_path_unref(_destPath);
        fm_path_list_unref(srcPaths);
        return false;
    }
    fm_path_unref(_destPath);
    fm_path_list_unref(srcPaths);
    return true;
  }
  else if(data->hasFormat("application/x-qabstractitemmodeldatalist")) {
    return true;
  }
  return QAbstractListModel::dropMimeData(data, action, row, column, parent);
}

Qt::DropActions FolderModel::supportedDropActions() const {
  qDebug("FolderModel::supportedDropActions");
  return Qt::CopyAction|Qt::MoveAction|Qt::LinkAction;
}

// ask the model to load thumbnails of the specified size
void FolderModel::cacheThumbnails(const int size) {
  QVector<QPair<int, int>>::iterator it = thumbnailRefCounts.begin();
  while (it != thumbnailRefCounts.end()) {
    if (it->first == size) {
      ++it->second;
      return;
    } else ++it;
  }
  thumbnailRefCounts.append(QPair<int, int>(size, 1));
}

// ask the model to free cached thumbnails of the specified size
void FolderModel::releaseThumbnails(int size) {
  QVector<QPair<int, int> >::iterator it;
  for(it = thumbnailRefCounts.begin(); it != thumbnailRefCounts.end(); ++it) {
    if(it->first == size) {
      break;
    }
  }
  if(it != thumbnailRefCounts.end()) {
    --it->second;
    if(it->second == 0) {
      thumbnailRefCounts.erase(it);

      // remove thumbnails that ara queued for loading from thumbnailResults
      QLinkedList<FmThumbnailLoader*>::iterator it;
      for(it = thumbnailResults.begin(); it != thumbnailResults.end();) {
        QLinkedList<FmThumbnailLoader*>::iterator next = it + 1;
        FmThumbnailLoader* res = *it;
        if(ThumbnailLoader::size(res) == size) {
          ThumbnailLoader::cancel(res);
          thumbnailResults.erase(it);
        }
        it = next;
      }

      // remove all cached thumbnails of the specified size
      QList<FolderModelItem>::iterator itemIt;
      for(itemIt = items.begin(); itemIt != items.end(); ++itemIt) {
        FolderModelItem& item = *itemIt;
        item.removeThumbnail(size);
      }
    }
  }
}

void FolderModel::onThumbnailLoaded(FmThumbnailLoader* res, gpointer user_data) {
  // FIXME: port this to the new Fm2 API
  FolderModel* pThis = reinterpret_cast<FolderModel*>(user_data);
  QLinkedList<FmThumbnailLoader*>::iterator it;
  for(it = pThis->thumbnailResults.begin(); it != pThis->thumbnailResults.end(); ++it) {
    if(*it == res) { // the thumbnail result is in our list
      pThis->thumbnailResults.erase(it); // remove it from the list
      FmFileInfo* info = ThumbnailLoader::fileInfo(res);
      int row = -1;
      // find the model item this thumbnail belongs to
      QList<FolderModelItem>::iterator it = pThis->findItemByName(fm_file_info_get_name(info), &row);
      if(it != pThis->items.end()) {
        // the file is found in our model
        FolderModelItem& item = *it;
        QModelIndex index = pThis->createIndex(row, 0, (void*)&item);
        // store the image in the folder model item.
        int size = ThumbnailLoader::size(res);
        QImage image = ThumbnailLoader::image(res);
        FolderModelItem::Thumbnail* thumbnail = item.findThumbnail(size);
        thumbnail->image = image;
        // qDebug("thumbnail loaded for: %s, size: %d", item.displayName.toUtf8().constData(), size);
        if(image.isNull())
          thumbnail->status = FolderModelItem::ThumbnailFailed;
        else {
          thumbnail->status = FolderModelItem::ThumbnailLoaded;
          // FIXME: due to bugs in Qt's QStyledItemDelegate, if the image width and height
          // are not the same, painting errors will happen. It's quite unfortunate.
          // Let's do some padding to make its width and height equals.
          // This greatly decrease performance :-(
          // Later if we can re-implement our own item delegate, this can be avoided.
          QPixmap pixmap = QPixmap(size, size);
          pixmap.fill(QColor(0, 0, 0, 0)); // fill the pixmap with transparent color (alpha:0)
          QPainter painter(&pixmap);
          int x = (size - image.width()) / 2;
          int y = (size - image.height()) / 2;
          painter.drawImage(QPoint(x, y), image); // draw the image to the pixmap at center.
          // FIXME: should we cache QPixmap instead for performance reason?
          thumbnail->image = pixmap.toImage(); // convert it back to image

          // tell the world that we have the thumbnail loaded
          Q_EMIT pThis->thumbnailLoaded(index, size);
        }
      }
      break;
    }
  }
}

// get a thumbnail of size at the index
// if a thumbnail is not yet loaded, this will initiate loading of the thumbnail.
QImage FolderModel::thumbnailFromIndex(const QModelIndex& index, int size) {

    // FIXME: port to the new Fm2 APIs

#if 0
  FolderModelItem* item = itemFromIndex(index);
  if(item) {
    FolderModelItem::Thumbnail* thumbnail = item->findThumbnail(size);
    // qDebug("FolderModel::thumbnailFromIndex: %d, %s", thumbnail->status, item->displayName.toUtf8().data());
    switch(thumbnail->status) {
      case FolderModelItem::ThumbnailNotChecked: {
        // load the thumbnail
        FmThumbnailLoader* res = ThumbnailLoader::load(item->info, size, onThumbnailLoaded, this);
        thumbnailResults.push_back(res);
        thumbnail->status = FolderModelItem::ThumbnailLoading;
        break;
      }
      case FolderModelItem::ThumbnailLoaded:
        return thumbnail->image;
      default:;
    }
  }
#endif
  return QImage();
}


} // namespace Fm

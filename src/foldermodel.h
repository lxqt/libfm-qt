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


#ifndef FM_FOLDERMODEL_H
#define FM_FOLDERMODEL_H

#include "libfmqtglobals.h"
#include <QAbstractListModel>
#include <QIcon>
#include <QImage>
#include <libfm/fm.h>
#include <QList>
#include <QVector>
#include <QLinkedList>
#include <QPair>
#include <vector>
#include "foldermodelitem.h"

#include "core/folder.h"
#include "core/thumbnailloader.h"

namespace Fm {

class LIBFM_QT_API FolderModel : public QAbstractListModel {
Q_OBJECT
public:

  enum Role {
    FileInfoRole = Qt::UserRole
  };

  enum ColumnId {
    ColumnFileName,
    ColumnFileType,
    ColumnFileSize,
    ColumnFileMTime,
    ColumnFileOwner,
    NumOfColumns
  };

public:
  FolderModel();
  virtual ~FolderModel();

  const std::shared_ptr<Fm2::Folder>& folder() const {
    return folder_;
  }

  void setFolder(const std::shared_ptr<Fm2::Folder>& new_folder);

  Fm2::FilePath path() {
    return folder_ ? folder_->getPath() : Fm2::FilePath();
  }

  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  int columnCount (const QModelIndex & parent) const;
  QVariant data(const QModelIndex & index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
  QModelIndex parent( const QModelIndex & index ) const;
  // void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

  Qt::ItemFlags flags(const QModelIndex & index) const;

  virtual QStringList mimeTypes() const;
  virtual QMimeData* mimeData(const QModelIndexList & indexes) const;
  virtual Qt::DropActions supportedDropActions() const;
  virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

  std::shared_ptr<const Fm2::FileInfo> fileInfoFromIndex(const QModelIndex& index) const;
  FolderModelItem* itemFromIndex(const QModelIndex& index) const;
  QImage thumbnailFromIndex(const QModelIndex& index, int size);

  void cacheThumbnails(int size);
  void releaseThumbnails(int size);

Q_SIGNALS:
  void thumbnailLoaded(const QModelIndex& index, int size);

protected Q_SLOTS:

  void onStartLoading();
  void onFinishLoading();
  void onFilesAdded(const Fm2::FileInfoList& files);
  void onFilesChanged(std::vector<Fm2::FileInfoPair>& files);
  void onFilesRemoved(const Fm2::FileInfoList &files);

protected:
  static void onThumbnailLoaded(FmThumbnailLoader *res, gpointer user_data);

  void insertFiles(int row, const Fm2::FileInfoList& files);
  void removeAll();
  QList<FolderModelItem>::iterator findItemByPath(const Fm2::FilePath& path, int* row);
  QList<FolderModelItem>::iterator findItemByName(const char* name, int* row);
  QList<FolderModelItem>::iterator findItemByFileInfo(const Fm2::FileInfo *info, int* row);

private:
  std::shared_ptr<Fm2::Folder> folder_;
  // FIXME: should we use a hash table here so item lookup becomes much faster?
  QList<FolderModelItem> items;

  // record what size of thumbnails we should cache in an array of <size, refCount> pairs.
  QVector<QPair<int, int> > thumbnailRefCounts;
  QLinkedList<FmThumbnailLoader*> thumbnailResults;
};

}

#endif // FM_FOLDERMODEL_H

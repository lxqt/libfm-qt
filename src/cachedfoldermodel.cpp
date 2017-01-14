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

#include "cachedfoldermodel.h"

namespace Fm {

CachedFolderModel::CachedFolderModel(const std::shared_ptr<Fm2::Folder>& folder):
  FolderModel(),
  refCount(1) {
  FolderModel::setFolder(folder);
}

CachedFolderModel::~CachedFolderModel() {
}

CachedFolderModel* CachedFolderModel::modelFromFolder(const std::shared_ptr<Fm2::Folder>& folder) {
  CachedFolderModel* model = NULL;
  QVariant cache = folder->property(cacheKey);
  model = cache.value<CachedFolderModel*>();
  if(model) {
    // qDebug("cache found!!");
    model->ref();
  }
  else {
    model = new CachedFolderModel(folder);
    cache = QVariant::fromValue(model);
    folder->setProperty(cacheKey, cache);
  }
  return model;
}

CachedFolderModel* CachedFolderModel::modelFromPath(const Fm2::FilePath &path) {
  auto folder = Fm2::Folder::fromPath(path);
  if(folder) {
    CachedFolderModel* model = modelFromFolder(folder);
    return model;
  }
  return NULL;
}

void CachedFolderModel::unref() {
  // qDebug("unref cache");
  --refCount;
  if(refCount <= 0) {
    folder()->setProperty(cacheKey, QVariant());
    deleteLater();
  }
}


} // namespace Fm

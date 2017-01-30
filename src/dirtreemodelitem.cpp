/*
 * Copyright (C) 2014 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#include "dirtreemodelitem.h"
#include "dirtreemodel.h"
#include "icontheme.h"
#include <QDebug>

namespace Fm {

DirTreeModelItem::DirTreeModelItem():
    fileInfo_(nullptr),
    folder_(nullptr),
    expanded_(false),
    loaded_(false),
    parent_(nullptr),
    placeHolderChild_(nullptr),
    model_(nullptr) {
}

DirTreeModelItem::DirTreeModelItem(std::shared_ptr<const Fm2::FileInfo> info, DirTreeModel* model, DirTreeModelItem* parent):
    fileInfo_{std::move(info)},
    expanded_(false),
    loaded_(false),
    parent_(parent),
    placeHolderChild_(nullptr),
    model_(model) {

    if(fileInfo_) {
        displayName_ = fileInfo_->displayName();
        icon_ = fileInfo_->icon()->qicon();
        addPlaceHolderChild();
    }
}

DirTreeModelItem::~DirTreeModelItem() {
    freeFolder();
    // delete child items if needed
    if(!children_.empty()) {
        Q_FOREACH(DirTreeModelItem* item, children_) {
            delete item;
        }
    }
    if(!hiddenChildren_.empty()) {
        Q_FOREACH(DirTreeModelItem* item, hiddenChildren_) {
            delete item;
        }
    }
}

void DirTreeModelItem::freeFolder() {
    if(folder_) {
        QObject::disconnect(onFolderFinishLoadingConn_);
        QObject::disconnect(onFolderFilesAddedConn_);
        QObject::disconnect(onFolderFilesRemovedConn_);
        QObject::disconnect(onFolderFilesChangedConn_);
        folder_.reset();
    }
}

void DirTreeModelItem::addPlaceHolderChild() {
    placeHolderChild_ = new DirTreeModelItem();
    placeHolderChild_->parent_ = this;
    placeHolderChild_->model_ = model_;
    placeHolderChild_->displayName_ = DirTreeModel::tr("Loading...");
    children_.push_back(placeHolderChild_);
}

void DirTreeModelItem::loadFolder() {
    if(!expanded_) {
        /* dynamically load content of the folder. */
        folder_ =  Fm2::Folder::fromPath(fileInfo_->path());
        /* g_debug("fm_dir_tree_model_load_row()"); */
        /* associate the data with loaded handler */

        onFolderFinishLoadingConn_ = QObject::connect(folder_.get(), &Fm2::Folder::finishLoading, model_, [=]() {
            onFolderFinishLoading();
        });
        onFolderFilesAddedConn_ = QObject::connect(folder_.get(), &Fm2::Folder::filesAdded, model_, [=](Fm2::FileInfoList files) {
            onFolderFilesAdded(files);
        });
        onFolderFilesRemovedConn_ = QObject::connect(folder_.get(), &Fm2::Folder::filesRemoved, model_, [=](Fm2::FileInfoList files) {
            onFolderFilesRemoved(files);
        });
        onFolderFilesChangedConn_ = QObject::connect(folder_.get(), &Fm2::Folder::filesChanged, model_, [=](std::vector<Fm2::FileInfoPair>& changes) {
            onFolderFilesChanged(changes);
        });

        /* set 'expanded' flag beforehand as callback may check it */
        expanded_ = true;
        /* if the folder is already loaded, call "loaded" handler ourselves */
        if(folder_->isLoaded()) { // already loaded
            for(auto& fi: folder_->files()) {
                if(fi->isDir()) {
                    insertFileInfo(fi);
                }
            }
            onFolderFinishLoading();
        }
    }
}

void DirTreeModelItem::unloadFolder() {
    if(expanded_) { /* do some cleanup */
        /* remove all children, and replace them with a dummy child
          * item to keep expander in the tree view around. */

        // delete all visible child items
        model_->beginRemoveRows(index(), 0, children_.size() - 1);
        if(!children_.empty()) {
            Q_FOREACH(DirTreeModelItem* item, children_) {
                delete item;
            }
            children_.clear();
        }
        model_->endRemoveRows();

        // remove hidden children
        if(!hiddenChildren_.empty()) {
            Q_FOREACH(DirTreeModelItem* item, hiddenChildren_) {
                delete item;
            }
            hiddenChildren_.clear();
        }

        /* now, we have no child since all child items are removed.
         * So we add a place holder child item to keep the expander around. */
        addPlaceHolderChild();
        /* deactivate folder since it will be reactivated on expand */
        freeFolder();
        expanded_ = false;
        loaded_ = false;
    }
}

QModelIndex DirTreeModelItem::index() {
    Q_ASSERT(model_);
    return model_->indexFromItem(this);
}

/* Add file info to parent node to proper position.
 * GtkTreePath tp is the tree path of parent node. */
DirTreeModelItem* DirTreeModelItem::insertFileInfo(std::shared_ptr<const Fm2::FileInfo> fi) {
    // qDebug() << "insertFileInfo: " << fm_file_info_get_disp_name(fi);
    DirTreeModelItem* item = new DirTreeModelItem(std::move(fi), model_);
    insertItem(item);
    return item;
}

// find a good position to insert the new item
int DirTreeModelItem::insertItem(DirTreeModelItem* newItem) {
    if(model_->showHidden() || !newItem->fileInfo_ || !newItem->fileInfo_->isHidden()) {
        auto newName = newItem->fileInfo_->displayName();
        auto it = std::lower_bound(children_.cbegin(), children_.cend(), newItem, [=](const DirTreeModelItem* a, const DirTreeModelItem* b) {
            return QString::localeAwareCompare(a->fileInfo_->displayName(), b->fileInfo_->displayName()) < 0;
        });
        // inform the world that we're about to insert the item
        auto position = it - children_.begin();
        model_->beginInsertRows(index(), position, position);
        newItem->parent_ = this;
        children_.insert(it, newItem);
        model_->endInsertRows();
        return position;
    }
    else { // hidden folder
        hiddenChildren_.push_back(newItem);
    }
    return -1;
}


// FmFolder signal handlers

void DirTreeModelItem::onFolderFinishLoading() {
    DirTreeModel* model = model_;
    /* set 'loaded' flag beforehand as callback may check it */
    loaded_ = true;
    QModelIndex idx = index();
    qDebug() << "folder loaded";
    // remove the placeholder child if needed
    if(children_.size() == 1) { // we have no other child other than the place holder item, leave it
        placeHolderChild_->displayName_ = DirTreeModel::tr("<No sub folders>");
        QModelIndex placeHolderIndex = placeHolderChild_->index();
        // qDebug() << "placeHolderIndex: "<<placeHolderIndex;
        Q_EMIT model->dataChanged(placeHolderIndex, placeHolderIndex);
    }
    else {
        auto it = std::find(children_.cbegin(), children_.cend(), placeHolderChild_);
        auto pos = it - children_.cbegin();
        model->beginRemoveRows(idx, pos, pos);
        children_.erase(it);
        delete placeHolderChild_;
        model->endRemoveRows();
        placeHolderChild_ = nullptr;
    }

    Q_EMIT model->rowLoaded(idx);
}

void DirTreeModelItem::onFolderFilesAdded(Fm2::FileInfoList& files) {
    for(auto& fi: files) {
        if(fi->isDir()) { /* FIXME: maybe adding files can be allowed later */
            /* Ideally FmFolder should not emit files-added signals for files that
             * already exists. So there is no need to check for duplication here. */
            insertFileInfo(fi);
        }
    }
}

void DirTreeModelItem::onFolderFilesRemoved(Fm2::FileInfoList& files) {
    DirTreeModel* model = model_;

    for(auto& fi: files) {
        int pos;
        DirTreeModelItem* child  = childFromName(fi->name().c_str(), &pos);
        if(child) {
            model->beginRemoveRows(index(), pos, pos);
            children_.erase(children_.cbegin() + pos);
            delete child;
            model->endRemoveRows();
        }
    }
}

void DirTreeModelItem::onFolderFilesChanged(std::vector<Fm2::FileInfoPair> &changes) {
    DirTreeModel* model = model_;
    for(auto& changePair: changes) {
        int pos;
        auto& changedFile = changePair.first;
        DirTreeModelItem* child = childFromName(changedFile->name().c_str(), &pos);
        if(child) {
            QModelIndex childIndex = child->index();
            Q_EMIT model->dataChanged(childIndex, childIndex);
        }
    }
}

DirTreeModelItem* DirTreeModelItem::childFromName(const char* utf8_name, int* pos) {
    int i = 0;
    for(const auto item : children_) {
        if(item->fileInfo_ && item->fileInfo_->name() == utf8_name) {
            if(pos) {
                *pos = i;
            }
            return item;
        }
        ++i;
    }
    return nullptr;
}

DirTreeModelItem* DirTreeModelItem::childFromPath(Fm2::FilePath path, bool recursive) const {
    Q_ASSERT(path != nullptr);

    Q_FOREACH(DirTreeModelItem* item, children_) {
        // if(item->fileInfo_)
        //  qDebug() << "child: " << QString::fromUtf8(fm_file_info_get_disp_name(item->fileInfo_));
        if(item->fileInfo_ && item->fileInfo_->path() == path) {
            return item;
        }
        else if(recursive) {
            DirTreeModelItem* child = item->childFromPath(std::move(path), true);
            if(child) {
                return child;
            }
        }
    }
    return nullptr;
}

void DirTreeModelItem::setShowHidden(bool show) {
    if(show) {
        // move all hidden children to visible list
        for(auto item: hiddenChildren_) {
            insertItem(item);
        }
        hiddenChildren_.clear();
    }
    else { // hide hidden folders
        QModelIndex _index = index();
        int pos = 0;
        for(auto it = children_.begin(); it != children_.end(); ++pos) {
            DirTreeModelItem* item = *it;
            auto next = it + 1;
            if(item->fileInfo_) {
                if(item->fileInfo_->isHidden()) { // hidden folder
                    // remove from the model and add to the hiddenChildren_ list
                    model_->beginRemoveRows(_index, pos, pos);
                    children_.erase(it);
                    hiddenChildren_.push_back(item);
                    model_->endRemoveRows();
                }
                else { // visible folder, recursively filter its children
                    item->setShowHidden(show);
                }
            }
            it = next;
        }
    }
}



} // namespace Fm

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

#ifndef FM_DIRTREEMODEL_H
#define FM_DIRTREEMODEL_H

#include "libfmqtglobals.h"
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QIcon>
#include <QList>
#include <QSharedPointer>
#include <vector>

#include "core/fileinfo.h"
#include "core/filepath.h"

namespace Fm {

class DirTreeModelItem;
class DirTreeView;

class LIBFM_QT_API DirTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    friend class DirTreeModelItem; // allow direct access of private members in DirTreeModelItem
    friend class DirTreeView; // allow direct access of private members in DirTreeView

    enum Role {
        FileInfoRole = Qt::UserRole
    };

    explicit DirTreeModel(QObject* parent);
    ~DirTreeModel();

    void addRoots(Fm::FilePathList rootPaths);

    void loadRow(const QModelIndex& index);
    void unloadRow(const QModelIndex& index);

    bool isLoaded(const QModelIndex& index);
    QIcon icon(const QModelIndex& index);
    std::shared_ptr<const Fm::FileInfo> fileInfo(const QModelIndex& index);
    Fm::FilePath filePath(const QModelIndex& index);
    QString dispName(const QModelIndex& index);

    void setShowHidden(bool show_hidden);
    bool showHidden() const {
        return showHidden_;
    }

    QModelIndex indexFromPath(const Fm::FilePath& path) const;

    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual int rowCount(const QModelIndex& parent) const;
    virtual QModelIndex parent(const QModelIndex& child) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent) const;
    virtual bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

Q_SIGNALS:
    void rowLoaded(const QModelIndex& index);

private Q_SLOTS:
    void onFileInfoJobFinished();

private:
    QModelIndex addRoot(std::shared_ptr<const Fm::FileInfo> root);

    DirTreeModelItem* itemFromPath(const Fm::FilePath& path) const;
    DirTreeModelItem* itemFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromItem(DirTreeModelItem* item) const;

private:
    bool showHidden_;
    std::vector<DirTreeModelItem*> rootItems_;
};

}

#endif // FM_DIRTREEMODEL_H

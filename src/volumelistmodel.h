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

#ifndef FM_VOLUMELISTMODEL_H
#define FM_VOLUMELISTMODEL_H

#include "libfmqtglobals.h"
#include <QAbstractTableModel>
#include "core/volumemanager.h"

namespace Fm {

// A minimal, self-contained model listing live system volumes. Deliberately
// NOT part of FolderModel/FolderModelItem -- meant to be concatenated onto a
// file-backed ProxyFolderModel via QConcatenateTablesProxyModel instead (see
// FolderView::setShowVolumes()), so the file model stays untouched.
// Rows are drag- and drop-enabled, but the data is handled elsewhere:
// FolderView's view model leaves volumes out of file drags (a mixed drag would
// assert in QConcatenateTablesProxyModel::mimeData()), and
// FolderView::childDropEvent() performs drops on a volume, mounting it if needed.
class LIBFM_QT_API VolumeListModel : public QAbstractTableModel {
    Q_OBJECT
public:
    // a volume row never answers FolderModel::FileInfoRole (it has no file),
    // so callers use this role to recognize one instead.
    enum Role {
        VolumeRole = Qt::UserRole + 500
    };

    explicit VolumeListModel(QObject* parent = nullptr);
    ~VolumeListModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    // QConcatenateTablesProxyModel keeps only the smallest column count of its
    // sources, so match FolderModel's or the detailed list view loses columns
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    // file URIs, so a view accepts dragging files over a volume
    QStringList mimeTypes() const override;
    Qt::DropActions supportedDropActions() const override;
    // always false: FolderView::childDropEvent() does the drop (see above)
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

private Q_SLOTS:
    void onVolumeAdded(const Fm::Volume& vol);
    void onVolumeRemoved(const Fm::Volume& vol);
    void onVolumeChanged(const Fm::Volume& vol);
    void onMountChanged(const Fm::Mount& mnt);

private:
    void emitRowChanged(GVolume* gvol);

    std::shared_ptr<VolumeManager> volumeManager_;
    std::vector<Volume> volumes_;
};

}

#endif // FM_VOLUMELISTMODEL_H

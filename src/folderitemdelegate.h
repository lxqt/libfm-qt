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


#ifndef FM_FOLDERITEMDELEGATE_H
#define FM_FOLDERITEMDELEGATE_H

#include "libfmqtglobals.h"
#include <QStyledItemDelegate>
#include <QAbstractItemView>

namespace Fm {

class LIBFM_QT_API FolderItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit FolderItemDelegate(QAbstractItemView* view, QObject* parent = nullptr);
    virtual ~FolderItemDelegate();

    inline void setGridSize(QSize size) {
        gridSize_ = size;
    }

    inline QSize gridSize() const {
        return gridSize_;
    }

    int fileInfoRole() {
        return fileInfoRole_;
    }

    void setFileInfoRole(int role) {
        fileInfoRole_ = role;
    }

    int iconInfoRole() {
        return iconInfoRole_;
    }

    void setIconInfoRole(int role) {
        iconInfoRole_ = role;
    }

    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
    void drawText(QPainter* painter, QStyleOptionViewItem& opt, QRectF& textRect) const;
    static QIcon::Mode iconModeFromState(QStyle::State state);

private:
    QAbstractItemView* view_;
    QIcon symlinkIcon_;
    QSize gridSize_;
    int fileInfoRole_;
    int iconInfoRole_;
};

}

#endif // FM_FOLDERITEMDELEGATE_H

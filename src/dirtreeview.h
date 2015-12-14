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
 
#ifndef FM_DIRTREEVIEW_H
#define FM_DIRTREEVIEW_H

#include "libfmqtglobals.h"
#include <QTreeView>
#include <libfm/fm.h>
#include "path.h"

class QItemSelection;

namespace Fm {

class FileMenu;
class DirTreeModelItem;

class LIBFM_QT_API DirTreeView : public QTreeView {
  Q_OBJECT

public:
  DirTreeView(QWidget* parent);
  ~DirTreeView();

  FmPath* currentPath() {
    return currentPath_;
  }

  void setCurrentPath(FmPath* path);

  // libfm-gtk compatible alias
  FmPath* getCwd() {
    return currentPath();
  }

  void chdir(FmPath* path) {
    setCurrentPath(path);
  }

  virtual void setModel(QAbstractItemModel* model);

protected:
  virtual void mousePressEvent(QMouseEvent* event);

private:
  void cancelPendingChdir();
  void expandPendingPath();

Q_SIGNALS:
  void chdirRequested(int type, FmPath* path);
  void openFolderInNewWindowRequested(FmPath* path);
  void openFolderInNewTabRequested(FmPath* path);
  void openFolderInTerminalRequested(FmPath* path);
  void createNewFolderRequested(FmPath* path);
  void prepareFileMenu(Fm::FileMenu* menu); // emit before showing a Fm::FileMenu

protected Q_SLOTS:
  void onCollapsed(const QModelIndex & index);
  void onExpanded(const QModelIndex & index);
  void onRowLoaded(const QModelIndex& index);
  void onSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
  void onCustomContextMenuRequested(const QPoint& pos);
  void onOpen();
  void onNewWindow();
  void onNewTab();
  void onOpenInTerminal();
  void onNewFolder();

private:
  FmPath* currentPath_;
  QList<Path> pathsToExpand_;
  DirTreeModelItem* currentExpandingItem_;
};

}

#endif // FM_DIRTREEVIEW_H

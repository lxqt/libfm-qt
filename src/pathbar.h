/*
 * Copyright (C) 2016  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef FM_PATHBAR_H
#define FM_PATHBAR_H

#include "libfmqtglobals.h"
#include <QWidget>
#include "path.h"

class QToolButton;
class QScrollArea;
class QPushButton;
class QHBoxLayout;

namespace Fm {

class PathEdit;

class LIBFM_QT_API PathBar: public QWidget {
  Q_OBJECT
public:
  explicit PathBar(QWidget *parent = 0);

  Path path() {
    return currentPath_;
  }

  void setPath(Path path);

Q_SIGNALS:
  void chdir(FmPath* path);
  void middleClickChdir(FmPath* path);
  void editingFinished();

public Q_SLOTS:
  void openEditor();
  void closeEditor();
  void copyPath();

private Q_SLOTS:
  void onButtonToggled(bool checked);
  void onScrollButtonClicked();
  void onReturnPressed();

protected:
  void resizeEvent(QResizeEvent* event);
  void mousePressEvent(QMouseEvent *event);
  void contextMenuEvent(QContextMenuEvent *event);

private:
  void updateScrollButtonVisibility();

private:
  QToolButton* leftArrow_;
  QToolButton* rightArrow_;
  QScrollArea* scrollArea_;
  QWidget* buttonsWidget_;
  QHBoxLayout* buttonsLayout_;
  PathEdit* tempPathEdit_;

  Path currentPath_;   // currently active path
};

} // namespace Fm

#endif // FM_PATHBAR_H

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

#ifndef FM_PATHBAR_P_H
#define FM_PATHBAR_P_H

#include <QToolButton>
#include <QStyle>
#include <QEvent>
#include "path.h"

namespace Fm {

class PathButton: public QToolButton {
  Q_OBJECT
public:
  PathButton(Fm::Path pathElement, QWidget* parent = nullptr):
    QToolButton(parent),
    pathElement_(pathElement) {

      setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
      setCheckable(true);
      setAutoExclusive(true);
      setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
      /* respect the toolbar icon size (can be set with some styles) */
      int icnSize = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
      setIconSize(QSize(icnSize, icnSize));

      char* label = pathElement.displayBasename();
      setText(label);
      g_free(label);

      if(pathElement.getParent().isNull()) { /* this element is root */
        QIcon icon = QIcon::fromTheme("drive-harddisk");
        setIcon(icon);
      }
  }

  Path pathElement() {
    return pathElement_;
  }

  void setPathElement(Path pathElement) {
    pathElement_ = pathElement;
  }

  void changeEvent(QEvent* event) override {
    QToolButton::changeEvent(event);
    if(event->type() == QEvent::StyleChange) {
      int icnSize = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
      setIconSize(QSize(icnSize, icnSize));
    }
  }

private:
  Path pathElement_;
};

} // namespace Fm

#endif // FM_PATHBAR_P_H

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

#ifndef FM_ICONENGINE_H
#define FM_ICONENGINE_H

#include <QIconEngine>
#include "libfmqtglobals.h"
#include "icon.h"
#include <gio/gio.h>

namespace Fm {

class IconEngineData;

class LIBFM_QT_API IconEngine: public QIconEngine {
public:
  IconEngine();
  IconEngine(const char** iconNames, int len);
  IconEngine(QStringList& iconNames);
  IconEngine(Fm::Icon& icon): IconEngine(icon.dataPtr()) {
  }
  IconEngine(FmIcon* fmIcon);
  IconEngine(GIcon* gIcon);

  ~IconEngine();

  virtual QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state);

  // not supported
  virtual void addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state) {}

  // not supported
  virtual void addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state) {}

  virtual QIconEngine* clone() const;

  virtual QString key() const;

  virtual void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state);

  virtual QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state);

  virtual void virtual_hook(int id, void *data);

  // called by Fm::IconTheme to initialize the icon engine module
  static void init();

  // called by Fm::IconTheme when icon theme change happens
  static void invalidateCache();

private:
  void initFromFmIcon(FmIcon* fmIcon);

private:
  IconEngineData* data_;
  static QIcon fallbackQIcon_;
};

} // namespace Fm

#endif // FM_ICONENGINE_H

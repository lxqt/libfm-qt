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


#include "icontheme.h"
#include <libfm/fm.h>
#include <QList>
#include <QIcon>
#include <QtGlobal>
#include <QApplication>
#include <QDesktopWidget>
#include "iconengine.h"
#include <QDebug>

namespace Fm {

static IconTheme* theIconTheme = NULL; // the global single instance of IconTheme.


IconTheme::IconTheme():
  currentThemeName_(QIcon::themeName()) {
  // NOTE: only one instance is allowed
  Q_ASSERT(theIconTheme == NULL);
  Q_ASSERT(qApp != NULL); // QApplication should exists before contructing IconTheme.

  theIconTheme = this;

  IconEngine::init();

  // We need to get notified when there is a QEvent::StyleChange event so
  // we can check if the current icon theme name is changed.
  // To do this, we can filter QApplication object itself to intercept
  // signals of all widgets, but this may be too inefficient.
  // So, we only filter the events on QDesktopWidget instead.
  qApp->desktop()->installEventFilter(this);
}

IconTheme::~IconTheme() {
}

IconTheme* IconTheme::instance() {
  return theIconTheme;
}

// check if the icon theme name is changed and emit "changed()" signal if any change is detected.
void IconTheme::checkChanged() {
  qDebug("checkChanged!!");
  qDebug() << QIcon::themeName();
  // if the icon theme is changed
  if(QIcon::themeName() != theIconTheme->currentThemeName_) {
    theIconTheme->currentThemeName_ = QIcon::themeName();
    IconEngine::invalidateCache();  // invalidate the cached data
    Q_EMIT theIconTheme->changed();
  }
}

//static
QIcon IconTheme::icon(FmIcon* fmicon) {
  return QIcon(new IconEngine(fmicon));
}

//static
QIcon IconTheme::icon(GIcon* gicon) {
  return QIcon(new IconEngine(gicon));
}

// static
QList<Icon> IconTheme::emblems(FmIcon* fmicon) {
  return emblems(G_ICON(fmicon));
}

//static
QList<Icon> IconTheme::emblems(GIcon* gicon) {
  if(G_IS_EMBLEMED_ICON(gicon)) {  // if this gicon contains emblems
    QList<Icon> emblemFmIcons;
    GList* emblems = g_emblemed_icon_get_emblems(G_EMBLEMED_ICON(gicon));
    for(GList* l = emblems; l; l = l->next) {
      GIcon* emblem_gicon = g_emblem_get_icon(G_EMBLEM(l->data));
      emblemFmIcons.append(Icon::fromGicon(emblem_gicon));
    }
    return emblemFmIcons;
  }
  return QList<Icon>();
}

// this method is called whenever there is an event on the QDesktopWidget object.
bool IconTheme::eventFilter(QObject* obj, QEvent* event) {
  qDebug() << event->type();
  // we're only interested in the StyleChange event.
  if(event->type() == QEvent::StyleChange || event->type() == QEvent::ThemeChange) {
    checkChanged(); // check if the icon theme is changed
  }
  return QObject::eventFilter(obj, event);
}


} // namespace Fm

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

namespace Fm {

class IconCacheData {
public:
    QIcon qicon;
    QList<Icon> emblems;
};

static IconTheme* theIconTheme = nullptr; // the global single instance of IconTheme.
static const char* fallbackNames[] = {"unknown", "application-octet-stream", nullptr};

static void fmIconDataDestroy(gpointer user_data) {
  IconCacheData* data = reinterpret_cast<IconCacheData*>(user_data);
  delete data;
}

IconTheme::IconTheme():
  currentThemeName_(QIcon::themeName()) {
  // NOTE: only one instance is allowed
  Q_ASSERT(theIconTheme == nullptr);
  Q_ASSERT(qApp != nullptr); // QApplication should exists before contructing IconTheme.

  theIconTheme = this;
  fm_icon_set_user_data_destroy(reinterpret_cast<GDestroyNotify>(fmIconDataDestroy));
  fallbackIcon_ = iconFromNames(fallbackNames);

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
  if(QIcon::themeName() != theIconTheme->currentThemeName_) {
    // if the icon theme is changed
    theIconTheme->currentThemeName_ = QIcon::themeName();
    // invalidate the cached data
    fm_icon_reset_user_data_cache(fm_qdata_id);

    theIconTheme->fallbackIcon_ = iconFromNames(fallbackNames);
    Q_EMIT theIconTheme->changed();
  }
}

QIcon IconTheme::iconFromNames(const char* const* names) {
  const gchar* const* name;
  // qDebug("names: %p", names);
  for(name = names; *name; ++name) {
    // qDebug("icon name=%s", *name);
    QString qname = *name;
    QIcon qicon = QIcon::fromTheme(qname);
    if(!qicon.isNull()) {
      return qicon;
    }
  }
  return QIcon();
}

QIcon IconTheme::convertFromGIconWithoutEmblems(GIcon* gicon) {
  if(G_IS_THEMED_ICON(gicon)) {
    const gchar * const * names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
    QIcon icon = iconFromNames(names);
    if(!icon.isNull())
      return icon;
  }
  else if(G_IS_FILE_ICON(gicon)) {
    GFile* file = g_file_icon_get_file(G_FILE_ICON(gicon));
    char* fpath = g_file_get_path(file);
    QString path = fpath;
    g_free(fpath);
    return QIcon(path);
  }
  return theIconTheme->fallbackIcon_;
}


// static
IconCacheData* IconTheme::ensureCacheData(FmIcon* fmicon) {
  IconCacheData* data = reinterpret_cast<IconCacheData*>(fm_icon_get_user_data(fmicon));
  if(!data) { // we don't have a cache yet
    data = new IconCacheData();
    GIcon* gicon = G_ICON(fmicon);
    if(G_IS_EMBLEMED_ICON(gicon)) { // special handling for emblemed icon
      GList* emblems = g_emblemed_icon_get_emblems(G_EMBLEMED_ICON(gicon));
      for(GList* l = emblems; l; l = l->next) {
        GIcon* emblem_gicon = g_emblem_get_icon(G_EMBLEM(l->data));
        data->emblems.append(Icon::fromGicon(emblem_gicon));
      }
      gicon = g_emblemed_icon_get_icon(G_EMBLEMED_ICON(gicon));  // get an emblemless GIcon
    }
    data->qicon = convertFromGIconWithoutEmblems(gicon);
    fm_icon_set_user_data(fmicon, data); // store it in FmIcon
  }
  return data;
}

//static
QIcon IconTheme::icon(FmIcon* fmicon) {
  IconCacheData* data = ensureCacheData(fmicon);
  return data->qicon;
}

//static
QIcon IconTheme::icon(GIcon* gicon) {
  if(G_IS_EMBLEMED_ICON(gicon)) // get an emblemless GIcon
    gicon = g_emblemed_icon_get_icon(G_EMBLEMED_ICON(gicon));
  if(G_IS_THEMED_ICON(gicon)) {
    FmIcon* fmicon = fm_icon_from_gicon(gicon);
    QIcon qicon = icon(fmicon);
    fm_icon_unref(fmicon);
    return qicon;
  }
  else if(G_IS_FILE_ICON(gicon)) {
    // we do not map GFileIcon to FmIcon deliberately.
    return convertFromGIconWithoutEmblems(gicon);
  }
  return theIconTheme->fallbackIcon_;
}

// static
QList<Icon> IconTheme::emblems(FmIcon* fmicon) {
  IconCacheData* data = ensureCacheData(fmicon);
  return data->emblems;
}

//static
QList<Icon> IconTheme::emblems(GIcon* gicon) {
  if(G_IS_EMBLEMED_ICON(gicon)) {  // if this gicon contains emblems
    Icon fmicon = Icon::fromGicon(gicon);
    return emblems(fmicon.dataPtr());
  }
  return QList<Icon>();
}

// this method is called whenever there is an event on the QDesktopWidget object.
bool IconTheme::eventFilter(QObject* obj, QEvent* event) {
  // we're only interested in the StyleChange event.
  if(event->type() == QEvent::StyleChange) {
    checkChanged(); // check if the icon theme is changed
  }
  return QObject::eventFilter(obj, event);
}


} // namespace Fm

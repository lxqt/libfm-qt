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

#include "iconengine.h"
#include <QDebug>

namespace Fm {

class IconEngineData {
public:

  IconEngineData(FmIcon* fmIcon): fmIcon_(fmIcon) {
  }

  static void destroy(IconEngineData* data) {
    delete data;
  }

  void reloadQIcon(const QIcon& fallbackQIcon = QIcon()) {
    // Find an appropriate icon for it.
    GIcon* gicon = G_ICON(fmIcon_);

    // if the GIcon is an embled icon, get the main icon here
    // emblems will be handled separately in other places
    if(G_IS_EMBLEMED_ICON(gicon)) {
      gicon = g_emblemed_icon_get_icon(G_EMBLEMED_ICON(gicon));
    }
    if(G_IS_THEMED_ICON(gicon)) {  // themed icon
      const gchar* const* iconNames = g_themed_icon_get_names(G_THEMED_ICON(gicon));
      // try all alternative icon names until we successfully load an icon.
      for(const gchar* const* name = iconNames; *name; ++name) {
        qDebug("icon: %s", *name);
        QIcon qicon = QIcon::fromTheme(QString(*name));

        if(!qicon.isNull()) {
          qIcon_ = qicon;
          break;
        }
      }
    }
    else if(G_IS_FILE_ICON(gicon)) {  // the icon is an image file
      GFile* file = g_file_icon_get_file(G_FILE_ICON(gicon));
      char* fpath = g_file_get_path(file);
      qIcon_ = QIcon(QString(fpath));
      g_free(fpath);
    }

    if(qIcon_.isNull()) {  // fallback to the default icon
      qIcon_ = fallbackQIcon;
    }
  }

  QIcon qIcon_;
  FmIcon* fmIcon_;
};

// static
QIcon IconEngine::fallbackQIcon_;

IconEngine::IconEngine(): QIconEngine(), data_(nullptr) {
}

IconEngine::IconEngine(const char** iconNames, int len): data_(nullptr) {
  GIcon* gicon = g_themed_icon_new(iconNames[0]);
  for(int i = 1; i < len; ++i)
    g_themed_icon_append_name(G_THEMED_ICON(gicon), iconNames[i]);
  FmIcon* fmIcon = fm_icon_from_gicon(gicon);
  initFromFmIcon(fmIcon);
  g_object_unref(fmIcon);
  g_object_unref(gicon);
}

IconEngine::IconEngine(QStringList& iconNames): data_(nullptr) {
  GIcon* gicon = g_themed_icon_new(iconNames[0].toLatin1().constData());
  for(int i = 1; i < iconNames.length(); ++i)
    g_themed_icon_append_name(G_THEMED_ICON(gicon), iconNames[i].toLatin1().constData());
  FmIcon* fmIcon = fm_icon_from_gicon(gicon);
  initFromFmIcon(fmIcon);
  g_object_unref(fmIcon);
  g_object_unref(gicon);
}

IconEngine::IconEngine(FmIcon* fmIcon): data_(nullptr) {
  initFromFmIcon(fmIcon);
}

IconEngine::IconEngine(GIcon* gIcon): data_(nullptr) {
  FmIcon* fmIcon = fm_icon_from_gicon(gIcon);
  initFromFmIcon(fmIcon);
  g_object_unref(fmIcon);
}

void IconEngine::initFromFmIcon(FmIcon* fmIcon) {
  IconEngineData* data = reinterpret_cast<IconEngineData*>(fm_icon_get_user_data(fmIcon));
  if(data == nullptr) {
    data = new IconEngineData(fmIcon);
    data->reloadQIcon(fallbackQIcon_);
    // Cache the IconEngineData object in the FmIcon object
    // The FmIcon object takes the ownership of IconEngineData.
    fm_icon_set_user_data(fmIcon, data);
  }
  data_ = data;
}

IconEngine::~IconEngine() {
  // We do not need to free the cached IconEngineData object.
  // The data is owned by the FmIcon object and will be freed when the
  // FmIcon is destroyed (reference count becomes 0).
}

// called by Fm::IconTheme to initialize the icon engine module
void IconEngine::init() {
  fm_icon_set_user_data_destroy(reinterpret_cast<GDestroyNotify>(IconEngineData::destroy));
  invalidateCache();
}

static void reloadIcon(GIcon* key, FmIcon* fmIcon) {
  IconEngineData* data = reinterpret_cast<IconEngineData*>(fm_icon_get_user_data(fmIcon));
  qDebug("reload: %p", fmIcon);
  if(data != nullptr) {
    data->reloadQIcon();
  }
}

// called by Fm::IconTheme when icon theme change happens
void IconEngine::invalidateCache() {
  qDebug("INVALIDATE CACHE");
  // invalidate the cached data
  fm_icon_cache_foreach((GHFunc)reloadIcon, nullptr);

  const char* fallbackNames[] = {"unknown", "application-octet-stream"};
  IconEngine* fallbackIconEngine = new IconEngine(fallbackNames, G_N_ELEMENTS(fallbackNames));
  fallbackQIcon_ = QIcon(fallbackIconEngine);
}

QSize IconEngine::actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) {
  if(data_ != nullptr)
    return data_->qIcon_.actualSize(size, mode, state);
  return size;
}

QIconEngine* IconEngine::clone() const {
  IconEngine* engine = new IconEngine();
  engine->data_ = data_;
  return engine;
}

QString IconEngine::key() const {
  return QStringLiteral("Fm::IconEngine");
}

void IconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) {
  if(data_ != nullptr) {
    data_->qIcon_.paint(painter, rect, Qt::AlignCenter, mode, state);
  }
}

QPixmap IconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) {
  if(data_ != nullptr) {
    return data_->qIcon_.pixmap(size, mode, state);
  }
  return QPixmap();
}

void IconEngine::virtual_hook(int id, void *data) {
  switch(id) {
  case QIconEngine::AvailableSizesHook:
    if(data_ != nullptr) {
      auto* args = reinterpret_cast<QIconEngine::AvailableSizesArgument*>(data);
      args->sizes = data_->qIcon_.availableSizes(args->mode, args->state);
    }
    break;
  case QIconEngine::IconNameHook: {
    QString* result = reinterpret_cast<QString*>(data);
    if(data_ != nullptr) {
      *result = data_->qIcon_.name();
    }
    break;
  }
  case QIconEngine::IsNullHook: {
    bool* result = reinterpret_cast<bool*>(data);
    if(data_ != nullptr) {
      *result = data_->qIcon_.isNull();
    }
    else {
      *result = true;
    }
    break;
  }
  }
}


} // namespace Fm

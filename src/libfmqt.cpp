/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#include <libfm/fm.h>
#include "libfmqt.h"
#include <QLocale>
#include "icontheme.h"
#include "thumbnailloader.h"
#include "core/thumbnailer.h"
#include "xdndworkaround.h"

namespace Fm {

struct LibFmQtData {
  LibFmQtData();
  ~LibFmQtData();

  IconTheme* iconTheme;
  ThumbnailLoader* thumbnailLoader;
  QTranslator translator;
  XdndWorkaround workaround;
  int refCount;
  Q_DISABLE_COPY(LibFmQtData)
};

static LibFmQtData* theLibFmData = NULL;

LibFmQtData::LibFmQtData(): refCount(1) {
#if !GLIB_CHECK_VERSION(2, 36, 0)
  g_type_init();
#endif
  fm_init(NULL);
  // turn on glib debug message
  // g_setenv("G_MESSAGES_DEBUG", "all", true);
  iconTheme = new IconTheme();
  thumbnailLoader = new ThumbnailLoader();
  Fm2::Thumbnailer::loadAll();
  translator.load("libfm-qt_" + QLocale::system().name(), LIBFM_QT_DATA_DIR "/translations");
}

LibFmQtData::~LibFmQtData() {
  delete iconTheme;
  delete thumbnailLoader;
  fm_finalize();
}

LibFmQt::LibFmQt() {
  if(!theLibFmData) {
    theLibFmData = new LibFmQtData();
  }
  else
    ++theLibFmData->refCount;
  d = theLibFmData;
}

LibFmQt::~LibFmQt() {
  if(--d->refCount == 0) {
    delete d;
    theLibFmData = NULL;
  }
}

QTranslator* LibFmQt::translator() {
  return &d->translator;
}

} // namespace Fm

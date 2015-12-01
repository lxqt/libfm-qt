/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef FM_PATHEDIT_P_H
#define FM_PATHEDIT_P_H

#include <QObject>
#include <libfm/fm.h>

namespace Fm {

class PathEdit;

class PathEditJob : public QObject {
  Q_OBJECT
public:
  GCancellable* cancellable;
  GFile* dirName;
  QStringList subDirs;
  PathEdit* edit;
  bool triggeredByFocusInEvent;

  ~PathEditJob() {
    g_object_unref(dirName);
    g_object_unref(cancellable);
  }

Q_SIGNALS:
  void finished();

public Q_SLOTS:
  void runJob();

};

}

#endif // FM_PATHEDIT_P_H

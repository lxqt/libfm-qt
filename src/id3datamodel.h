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
#ifndef FM_ID3DATAMODEL_H
#define FM_ID3DATAMODEL_H

#include "libfmqtglobals.h"
#include <QAbstractListModel>
#include <QIcon>
#include <QImage>
#include <libfm/fm.h>
#include <QList>
#include <QVector>
#include <QLinkedList>
#include <QPair>
#include "foldermodelitem.h"
#include "foldermodel.h"
#include <regex>
#include <id3/tag.h>
#include <id3/field.h>
#include <cstring>

namespace Fm {

class LIBFM_QT_API ID3DataModel {
public:
  ID3DataModel (const char*);

  enum ColumnId {
    ColumnFileID3Title,
    ColumnFileID3Artist,
    ColumnFileID3Album
  };

  std::string parseData (enum ColumnId);

  void setPath (const char*);

protected:

private:
  char* filePath;
  std::regex acceptName;
  
  ID3_Tag fileTag;
  ID3_Frame* fileTagFrame;
};

}

#endif // FM_ID3DATAMODEL_H

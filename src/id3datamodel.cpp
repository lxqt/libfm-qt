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
#include "foldermodel.h"
#include "id3datamodel.h"
#include <codecvt>
#include <string>

namespace Fm {
  ID3DataModel::ID3DataModel (const char* path) {
    filePath = (char *)path;
    acceptName = std::regex ("^.*(\.mp3)$");
  }

  std::string ID3DataModel::parseData (ColumnId type) {
    if ( ! std::regex_match (filePath, acceptName)) {
      return std::string("");
    }

    fileTag.Link (filePath, ID3TE_UTF8 | ID3TT_ALL);
    switch (type) {
      case ColumnFileID3Title: {
        fileTagFrame = fileTag.Find (ID3FID_TITLE);
        break;
      }
      case ColumnFileID3Artist: {
        fileTagFrame = fileTag.Find (ID3FID_LEADARTIST);
        break;
      }
      case ColumnFileID3Album: {
        fileTagFrame = fileTag.Find (ID3FID_ALBUM);
        break;
      }
    }
    if (fileTagFrame) {
      const char *tmp = fileTagFrame -> GetField(ID3FN_TEXT) -> GetRawText();
      if (tmp) { return std::string(tmp); }
      return std::string("");
    } else {
      return std::string("");
    }
  }

  void ID3DataModel::setPath (const char* path) {
    filePath = (char *)path;
  }
}

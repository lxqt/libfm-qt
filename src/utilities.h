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

#ifndef FM_UTILITIES_H
#define FM_UTILITIES_H

#include "libfmqtglobals.h"
#include <QClipboard>
#include <QUrl>
#include <QList>
#include <libfm/fm.h>
#include <sys/types.h>

#include "core/filepath.h"
#include "core/fileinfo.h"

class QDialog;

namespace Fm {

LIBFM_QT_API Fm2::FilePathList pathListFromUriList(const char* uriList);

LIBFM_QT_API QByteArray pathListToUriList(const Fm2::FilePathList& paths);

LIBFM_QT_API Fm2::FilePathList pathListFromQUrls(QList<QUrl> urls);

LIBFM_QT_API void pasteFilesFromClipboard(const Fm2::FilePath& destPath, QWidget* parent = 0);

LIBFM_QT_API void copyFilesToClipboard(const Fm2::FilePathList& files);

LIBFM_QT_API void cutFilesToClipboard(const Fm2::FilePathList& files);

LIBFM_QT_API void renameFile(std::shared_ptr<const Fm2::FileInfo> file, QWidget* parent = 0);

enum CreateFileType {
    CreateNewFolder,
    CreateNewTextFile,
    CreateWithTemplate
};

LIBFM_QT_API void createFileOrFolder(CreateFileType type, Fm2::FilePath parentDir, FmTemplate* templ = NULL, QWidget* parent = 0);

LIBFM_QT_API uid_t uidFromName(QString name);

LIBFM_QT_API QString uidToName(uid_t uid);

LIBFM_QT_API gid_t gidFromName(QString name);

LIBFM_QT_API QString gidToName(gid_t gid);

LIBFM_QT_API int execModelessDialog(QDialog* dlg);

// NOTE: this does not work reliably due to some problems in gio/gvfs
// Use uriExists() whenever possible.
LIBFM_QT_API bool isUriSchemeSupported(const char* uriScheme);

LIBFM_QT_API bool uriExists(const char* uri);

}

#endif // FM_UTILITIES_H

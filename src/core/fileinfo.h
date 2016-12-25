/*
 * Copyright (C) 2016 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef __LIBFM_QT_FM2_FILE_INFO_H__
#define __LIBFM_QT_FM2_FILE_INFO_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <vector>
#include <utility>
#include <string>

#include "gobjectptr.h"
#include "cstrptr.h"
#include "filepath.h"
#include "icon.h"
#include "mimetype.h"


namespace Fm2 {

class LIBFM_QT_API FileInfo {
public:

    explicit FileInfo();

    explicit FileInfo(GObjectPtr<GFileInfo> inf);

    virtual ~FileInfo();

    bool canSetHidden() const {
        return isHiddenChangeable_;
    }

    bool canSetIcon() const {
        return isIconChangeable_;
    }

    bool canSetName() const {
        return isNameChangeable_;
    }

    bool canThumbnail() const;

    gid_t getGid() const {
        return gid_;
    }

    uid_t getUid() const {
        return uid_;
    }

    const char* getFsId() {
        return filesystemId_;
    }

    const std::shared_ptr<const Icon>& getIcon() const {
        return icon_;
    }

    time_t getCtime() const {
        return ctime_;
    }


    time_t getAtime() const {
        return atime_;
    }

    time_t getMtime() const {
        return mtime_;
    }

    const std::string& getTarget() const {
        return target_;
    }

    bool isWritableDirectory() const {
        return (!isReadOnly_ && isDir());
    }

    bool isAccessible() const {
        return isAccessible_;
    }

    bool isExecutableType() const;

    bool isBackup() const {
        return isBackup_;
    }

    bool isHidden() const {
        // FIXME: we might treat backup files as hidden
        return isHidden_;
    }

    bool isUnknownType() const {
        return mimeType_->isUnknownType();
    }

    bool isDesktopEntry() const {
        return mimeType_->isDesktopEntry();
    }

    bool isText() const {
        return mimeType_->isText();
    }

    bool isImage() const {
        return mimeType_->isImage();
    }

    bool isMountable() const {
        return mimeType_->isMountable();
    }

    bool isShortcut() const {
        return isShortcut_;
    }

    bool isSymlink() const {
        return S_ISLNK(mode_) ? true : false;
    }

    bool isDir() const {
        return mimeType_->isDir();
    }

    const std::shared_ptr<const MimeType>& mimeType() const {
        return mimeType_;
    }

    bool isNative() const {
        return dirPath_.isNative();
    }

    mode_t getMode() const {
        return mode_;
    }

    uint64_t getBlocks() const {
        return blksize_ *blocks_;
    }

    uint64_t getSize() const {
        return size_;
    }

    const std::string& getName() const {
        return name_;
    }

    const QString& getDispName() const {
        return dispName_;
    }

    FilePath path() const {
        return dirPath_.child(name_.c_str());
    }

    const FilePath& dirPath() const {
        return dirPath_;
    }

    const char* filesystemId() const {
        return filesystemId_;
    }

    void setFromGFileInfo(GObjectPtr<GFileInfo> inf);

private:
    std::string name_;
    QString dispName_;

    FilePath dirPath_;

    mode_t mode_;
    const char* filesystemId_;
    uid_t uid_;
    gid_t gid_;
    uint64_t size_;
    time_t mtime_;
    time_t atime_;
    time_t ctime_;

    uint64_t blksize_;
    uint64_t blocks_;

    std::shared_ptr<const MimeType> mimeType_;
    std::shared_ptr<const Icon> icon_;

    std::string target_; /* target of shortcut or mountable. */

    bool isShortcut_ : 1; /* TRUE if file is shortcut type */
    bool isAccessible_ : 1; /* TRUE if can be read by user */
    bool isHidden_ : 1; /* TRUE if file is hidden */
    bool isBackup_ : 1; /* TRUE if file is backup */
    bool isNameChangeable_ : 1; /* TRUE if name can be changed */
    bool isIconChangeable_ : 1; /* TRUE if icon can be changed */
    bool isHiddenChangeable_ : 1; /* TRUE if hidden can be changed */
    bool isReadOnly_ : 1; /* TRUE if host FS is R/O */

    // std::vector<std::tuple<int, void*, void(void*)>> extraData_;
};


class FileInfoList: public std::vector<std::shared_ptr<const FileInfo>> {
public:

    bool isSameType() const;

    bool isSameFilesystem() const;
};


typedef std::pair<std::shared_ptr<const FileInfo>, std::shared_ptr<const FileInfo>> FileInfoPair;


}

#endif // __LIBFM_QT_FM2_FILE_INFO_H__

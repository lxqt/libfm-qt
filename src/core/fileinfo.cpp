#include "fileinfo.h"
#include "fileinfo_p.h"
#include <gio/gio.h>

namespace Fm2 {

const char gfile_info_query_attribs[] = "standard::*,"
                                        "unix::*,"
                                        "time::*,"
                                        "access::*,"
                                        "id::filesystem,"
                                        "metadata::emblems";

FileInfo::FileInfo() {
    // FIXME: initialize numeric data members
}

FileInfo::FileInfo(const GFileInfoPtr& inf, const FilePath& parentDirPath) {
    setFromGFileInfo(inf, parentDirPath);
}

FileInfo::~FileInfo() {
}

void FileInfo::setFromGFileInfo(const GObjectPtr<GFileInfo>& inf, const FilePath& parentDirPath) {
    dirPath_ = parentDirPath;
    const char *tmp, *uri;
    GIcon* gicon;
    GFileType type;

    name_ = g_file_info_get_name(inf.get());

    dispName_ = g_file_info_get_display_name(inf.get());

    size_ = g_file_info_get_size(inf.get());

    tmp = g_file_info_get_content_type(inf.get());
    if(!tmp) {
        tmp = "application/octet-stream";
    }
    mimeType_ = MimeType::fromName(tmp);

    mode_ = g_file_info_get_attribute_uint32(inf.get(), G_FILE_ATTRIBUTE_UNIX_MODE);

    uid_ = gid_ = -1;
    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_UNIX_UID))
        uid_ = g_file_info_get_attribute_uint32(inf.get(), G_FILE_ATTRIBUTE_UNIX_UID);
    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_UNIX_GID))
        gid_ = g_file_info_get_attribute_uint32(inf.get(), G_FILE_ATTRIBUTE_UNIX_GID);

    type = g_file_info_get_file_type(inf.get());
    if(0 == mode_) { /* if UNIX file mode is not available, compose a fake one. */
        switch(type)
        {
        case G_FILE_TYPE_REGULAR:
            mode_ |= S_IFREG;
            break;
        case G_FILE_TYPE_DIRECTORY:
            mode_ |= S_IFDIR;
            break;
        case G_FILE_TYPE_SYMBOLIC_LINK:
            mode_ |= S_IFLNK;
            break;
        case G_FILE_TYPE_SHORTCUT:
            break;
        case G_FILE_TYPE_MOUNTABLE:
            break;
        case G_FILE_TYPE_SPECIAL:
            if(mode_)
                break;
        /* if it's a special file but it doesn't have UNIX mode, compose a fake one. */
            if(strcmp(tmp, "inode/chardevice")==0)
                mode_ |= S_IFCHR;
            else if(strcmp(tmp, "inode/blockdevice")==0)
                mode_ |= S_IFBLK;
            else if(strcmp(tmp, "inode/fifo")==0)
                mode_ |= S_IFIFO;
#ifdef S_IFSOCK
            else if(strcmp(tmp, "inode/socket")==0)
                mode_ |= S_IFSOCK;
#endif
            break;
        case G_FILE_TYPE_UNKNOWN:
            ;
        }
    }

    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
        isAccessible_ = g_file_info_get_attribute_boolean(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
    else
        /* assume it's accessible */
        isAccessible_ = true;

    /* special handling for symlinks */
    if (g_file_info_get_is_symlink(inf.get())) {
        mode_ &= ~S_IFMT; /* reset type */
        mode_ |= S_IFLNK; /* set type to symlink */
        goto _file_is_symlink;
    }

    switch(type) {
    case G_FILE_TYPE_SHORTCUT:
        isShortcut_ = true;
    case G_FILE_TYPE_MOUNTABLE:
        uri = g_file_info_get_attribute_string(inf.get(), G_FILE_ATTRIBUTE_STANDARD_TARGET_URI);
        if(uri) {
            if(g_str_has_prefix(uri, "file:///")) {
                auto filename = CStrPtr{g_filename_from_uri(uri, nullptr, nullptr)};
                target_ = filename.get();
            }
            else
                target_ = uri;
            if(!mimeType_)
                mimeType_ = MimeType::guessFromFileName(target_.c_str());
        }

        /* if the mime-type is not determined or is unknown */
        if(G_UNLIKELY(!mimeType_ || mimeType_->isUnknownType())) {
            /* FIXME: is this appropriate? */
            if(type == G_FILE_TYPE_SHORTCUT)
                mimeType_ = MimeType::inodeShortcut();
            else
                mimeType_ = MimeType::inodeMountPoint();
        }
        break;
    case G_FILE_TYPE_DIRECTORY:
        if(!mimeType_)
            mimeType_ = MimeType::inodeDirectory();
        isReadOnly_ = false; /* default is R/W */
        if (g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_FILESYSTEM_READONLY))
            isReadOnly_ = g_file_info_get_attribute_boolean(inf.get(), G_FILE_ATTRIBUTE_FILESYSTEM_READONLY);
        break;
    case G_FILE_TYPE_SYMBOLIC_LINK:
_file_is_symlink:
        uri = g_file_info_get_symlink_target(inf.get());
        if(uri)
        {
            if(g_str_has_prefix(uri, "file:///")) {
                auto filename = CStrPtr{g_filename_from_uri(uri, nullptr, nullptr)};
                target_ = filename.get();
            }
            else {
                target_ = uri;
            }
            if(!mimeType_)
                mimeType_ = MimeType::guessFromFileName(target_.c_str());
        }
        /* continue with absent mime type */
    default: /* G_FILE_TYPE_UNKNOWN G_FILE_TYPE_REGULAR G_FILE_TYPE_SPECIAL */
        if(G_UNLIKELY(!mimeType_)) {
            if(!mimeType_)
                mimeType_ = MimeType::guessFromFileName(name_.c_str());
        }
    }

    /* try file-specific icon first */
    gicon = g_file_info_get_icon(inf.get());
    if(gicon) {
        icon_ = IconInfo::fromGIcon(gicon);
    }

#if 0
    /* set "locked" icon on unaccesible folder */
    else if(!accessible && type == G_FILE_TYPE_DIRECTORY)
        icon = g_object_ref(icon_locked_folder);
    else
        icon = g_object_ref(fm_mime_type_get_icon(mime_type));
#endif

    /* if the file has emblems, add them to the icon */
    auto emblem_names = g_file_info_get_attribute_stringv(inf.get(), "metadata::emblems");
    if(emblem_names) {
        auto n_emblems = g_strv_length(emblem_names);
        for(int i = n_emblems - 1; i >= 0; --i) {
            emblems_.emplace_front(Fm2::IconInfo::fromName(emblem_names[i]));
        }
    }

    tmp = g_file_info_get_attribute_string(inf.get(), G_FILE_ATTRIBUTE_ID_FILESYSTEM);
    filesystemId_ = g_intern_string(tmp);

    mtime_ = g_file_info_get_attribute_uint64(inf.get(), G_FILE_ATTRIBUTE_TIME_MODIFIED);
    atime_ = g_file_info_get_attribute_uint64(inf.get(), G_FILE_ATTRIBUTE_TIME_ACCESS);
    ctime_ = g_file_info_get_attribute_uint64(inf.get(), G_FILE_ATTRIBUTE_TIME_CHANGED);
    isHidden_ = g_file_info_get_is_hidden(inf.get());
    isBackup_ = g_file_info_get_is_backup(inf.get());
    isNameChangeable_ = true; /* GVFS tends to ignore this attribute */
    isIconChangeable_ = isHiddenChangeable_ = false;
    if (g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME))
        isNameChangeable_ = g_file_info_get_attribute_boolean(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME);

#if 0
    GFile *_gf = nullptr;
    GFileAttributeInfoList *list;
    auto list = g_file_query_settable_attributes(gf, nullptr, nullptr);
    if (G_LIKELY(list))
    {
        if (g_file_attribute_info_list_lookup(list, G_FILE_ATTRIBUTE_STANDARD_ICON))
            icon_is_changeable = true;
        if (g_file_attribute_info_list_lookup(list, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN))
            hidden_is_changeable = true;
        g_file_attribute_info_list_unref(list);
    }
    if (G_UNLIKELY(_gf))
        g_object_unref(_gf);
#endif
}

bool FileInfo::canThumbnail() const {
    /* We cannot use S_ISREG here as this exclude all symlinks */
    if( size_ == 0 || /* don't generate thumbnails for empty files */
        !(mode_ & S_IFREG) ||
        isDesktopEntry() ||
        isUnknownType())
        return false;
    return true;
}

/* full path of the file is required by this function */
bool FileInfo::isExecutableType() const {
    if(isText()) { /* g_content_type_can_be_executable reports text files as executables too */
        /* We don't execute remote files nor files in trash */
        if(isNative() && (mode_ & (S_IXOTH|S_IXGRP|S_IXUSR))) {
            /* it has executable bits so lets check shell-bang */
            auto pathStr = path().toString();
            int fd = open(pathStr.get(), O_RDONLY);
            if(fd >= 0) {
                char buf[2];
                ssize_t rdlen = read(fd, &buf, 2);
                close(fd);
                if(rdlen == 2 && buf[0] == '#' && buf[1] == '!')
                    return true;
            }
        }
        return false;
    }
    return mimeType_->canBeExecutable();
}


bool FileInfoList::isSameType() const {
    if(!empty()) {
        auto& item = front();
        for(auto it = cbegin() + 1; it != cend(); ++it) {
            auto& item2 = *it;
            if(item->mimeType() != item2->mimeType())
                return false;
        }
    }
    return true;
}

bool FileInfoList::isSameFilesystem() const {
    if(!empty()) {
        auto& item = front();
        for(auto it = cbegin() + 1; it != cend(); ++it) {
            auto& item2 = *it;
            if(item->filesystemId() != item2->filesystemId())
                return false;
        }
    }
    return true;
}



} // namespace Fm2


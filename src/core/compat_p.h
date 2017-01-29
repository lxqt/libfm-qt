#ifndef LIBFM_QT_COMPAT_P_H
#define LIBFM_QT_COMPAT_P_H

#include "libfmqtglobals.h"
#include "core/filepath.h"
#include "core/fileinfo.h"
#include "core/gioptrs.h"

// deprecated
#include <libfm/fm.h>
#include "path.h"

// compatibility functions bridging the old libfm C APIs and new C++ APIs.

namespace Fm2 {

inline FM_QT_DEPRECATED Fm::Path _convertPath(const Fm2::FilePath& path) {
    return Fm::Path::newForGfile(path.gfile().get());
}

inline FM_QT_DEPRECATED Fm::PathList _convertPathList(const Fm2::FilePathList& srcFiles) {
    Fm::PathList ret;
    for(auto& file: srcFiles) {
        ret.pushTail(_convertPath(file));
    }
    return ret;
}

inline FM_QT_DEPRECATED FmFileInfo* _convertFileInfo(const std::shared_ptr<const Fm2::FileInfo>& info) {
    // conver to GFileInfo first
    GFileInfoPtr ginfo{g_file_info_new(), false};
    g_file_info_set_name(ginfo.get(), info->name().c_str());
    g_file_info_set_display_name(ginfo.get(), info->displayName().toUtf8().constData());
    g_file_info_set_content_type(ginfo.get(), info->mimeType()->name());

    auto mode = info->mode();
    g_file_info_set_attribute_uint32(ginfo.get(), G_FILE_ATTRIBUTE_UNIX_MODE, mode);
    GFileType ftype = info->isDir() ? G_FILE_TYPE_DIRECTORY : G_FILE_TYPE_REGULAR;  // FIXME: generate more accurate type
    g_file_info_set_file_type(ginfo.get(), ftype);
    g_file_info_set_size(ginfo.get(), info->size());
    g_file_info_set_icon(ginfo.get(), info->icon()->gicon().get());

    g_file_info_set_attribute_uint64(ginfo.get(), G_FILE_ATTRIBUTE_TIME_MODIFIED, info->mtime());
    g_file_info_set_attribute_uint64(ginfo.get(), G_FILE_ATTRIBUTE_TIME_ACCESS, info->atime());
    g_file_info_set_attribute_uint64(ginfo.get(), G_FILE_ATTRIBUTE_TIME_CHANGED, info->ctime());

    auto fmpath = Fm::Path::newForGfile(info->path().gfile().get());
    return fm_file_info_new_from_gfileinfo(fmpath.dataPtr(), ginfo.get());
}

}

#endif // LIBFM_QT_COMPAT_P_H

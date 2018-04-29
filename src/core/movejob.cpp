#include "core/movejob.h"
#include "core/totalsizejob.h"
#include <tuple>

namespace Fm {

MoveJob::MoveJob(FilePathList srcPaths): CopyJob{std::move(srcPaths)} {
}

MoveJob::MoveJob(FilePathList srcPaths, FilePathList destPaths):
    CopyJob{std::move(srcPaths), std::move(destPaths)} {
}

MoveJob::MoveJob(FilePathList srcPaths, const FilePath &destDirPath):
    CopyJob{std::move(srcPaths), destDirPath} {
}

void MoveJob::exec() {
    if(srcPaths_.size() != destPaths_.size()) {
        qWarning("error: srcPaths.size() != destPaths.size() when copying files");
        return;
    }

    // check if the source and dest files are on the same filesystem
    FilePathList sameFsSrcPaths, sameFsDestPaths;
    FilePathList crossFsSrcPaths, crossFsDestPaths;
    for(size_t i = 0; i < srcPaths_.size(); ++i) {
        if(isCancelled()) {
            break;
        }
        const auto& srcPath = srcPaths_[i];
        const auto& destPath = destPaths_[i];
        auto destDirPath = destPath.parent();

        GErrorPtr err;
        GFileInfoPtr srcInfo = GFileInfoPtr {
            g_file_query_info(srcPath.gfile().get(),
            gfile_info_query_attribs,
            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
            cancellable().get(), &err),
            false
        };
        if(!srcInfo || isCancelled()) {
            // FIXME: report error
            return;
        }
        GFileInfoPtr destDirInfo = GFileInfoPtr {
            g_file_query_info(destDirPath.gfile().get(),
            "id::filesystem",
            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
            cancellable().get(), &err),
            false
        };

        // If src and dest are on the same filesystem, do move.
        // Otherwise, do copy & delete src files.
        auto src_fs = g_file_info_get_attribute_string(srcInfo.get(), "id::filesystem");
        auto dest_fs = g_file_info_get_attribute_string(destDirInfo.get(), "id::filesystem");
        if(g_strcmp0(src_fs, dest_fs) == 0) {
            // src and dest are on the same filesystem
            sameFsSrcPaths.emplace_back(srcPath);
            sameFsDestPaths.emplace_back(destPath);
        }
        else {
            // cross device/filesystem move: copy & delete
            crossFsSrcPaths.emplace_back(srcPath);
            crossFsDestPaths.emplace_back(destPath);
        }
    }

    // calculate total amount of work
    // same filesystem
    if(!sameFsSrcPaths.empty()) {
        TotalSizeJob sameFsSize{sameFsSrcPaths};
    }
    // cross filesystem
    if(!crossFsSrcPaths.empty()) {
        TotalSizeJob sameFsSize{crossFsSrcPaths};

    }

    // perform the actual move
    // same filesystem
    if(!sameFsSrcPaths.empty()) {
    }

    // cross filesystem
    if(!crossFsSrcPaths.empty()) {
        // copy the src files to dest

        // delete source
    }

}

} // namespace Fm

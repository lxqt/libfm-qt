#include "fileinfojob.h"
#include "fileinfo_p.h"

namespace Fm {

FileInfoJob::FileInfoJob(FilePathList paths, FilePath commonDirPath):
    Job(),
    paths_{std::move(paths)},
    commonDirPath_{std::move(commonDirPath)} {
}

void FileInfoJob::exec() {
    for(const auto& path: paths_) {
        if(!isCancelled()) {
            GErrorPtr err;
            GFileInfoPtr inf{
                g_file_query_info(path.gfile().get(), gfile_info_query_attribs,
                                  G_FILE_QUERY_INFO_NONE, cancellable().get(), &err),
                false
            };

            // Reuse the same dirPath object when the path remains the same (optimize for files in the same dir)
            auto dirPath = commonDirPath_.isValid() ? commonDirPath_ : path.parent();
            std::shared_ptr<const FileInfo> fileInfo = std::make_shared<FileInfo>(inf, dirPath);
            results_.push_back(fileInfo);
            Q_EMIT gotInfo(path, fileInfo);
        }
    }
}

} // namespace Fm

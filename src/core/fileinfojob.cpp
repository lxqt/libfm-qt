#include "fileinfojob.h"
#include "fileinfo_p.h"

namespace Fm {

FileInfoJob::FileInfoJob(FilePathList paths, FilePathList deletionPaths, FilePath commonDirPath, const std::shared_ptr<const HashSet>& cutFilesHashSet):
    Job(),
    paths_{std::move(paths)},
    deletionPaths_{std::move(deletionPaths)},
    commonDirPath_{std::move(commonDirPath)},
    cutFilesHashSet_{cutFilesHashSet} {
}

void FileInfoJob::exec() {
    for(const auto& path: paths_) {
        if(!isCancelled()) {
            GErrorPtr err;
            GFileInfoPtr inf{
                g_file_query_info(path.gfile().get(), defaultGFileInfoQueryAttribs,
                                  G_FILE_QUERY_INFO_NONE, cancellable().get(), &err),
                false
            };
            if(!inf) {
                continue;
            }

            // Reuse the same dirPath object when the path remains the same (optimize for files in the same dir)
            auto dirPath = commonDirPath_.isValid() ? commonDirPath_ : path.parent();
            FileInfo fileInfo(inf, dirPath);

            if(cutFilesHashSet_
                    && cutFilesHashSet_->count(fileInfo.path().hash())) {
                fileInfo.bindCutFiles(cutFilesHashSet_);
            }

            auto fileInfoPtr = std::make_shared<const FileInfo>(fileInfo);

            results_.push_back(fileInfoPtr);
            Q_EMIT gotInfo(path, fileInfoPtr);
        }
    }
}

} // namespace Fm

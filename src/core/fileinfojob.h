#ifndef FM2_FILEINFOJOB_H
#define FM2_FILEINFOJOB_H

#include "../libfmqtglobals.h"
#include "job.h"
#include "filepath.h"
#include "fileinfo.h"

namespace Fm {


class LIBFM_QT_API FileInfoJob : public Job {
    Q_OBJECT
public:

    explicit FileInfoJob(FilePathList paths, FilePathList deletionPaths = FilePathList(), FilePath commonDirPath = FilePath(), const std::shared_ptr<const HashSet>& cutFilesHashSet = nullptr);

    const FilePathList& paths() const {
        return paths_;
    }

    const FilePathList& deletionPaths() const {
        return deletionPaths_;
    }

    const FileInfoList& files() const {
        return results_;
    }

    const FilePath& currentPath() const {
        return currentPath_;
    }

Q_SIGNALS:
    void gotInfo(const FilePath& path, std::shared_ptr<const FileInfo>& info);

protected:
    void exec() override;

private:
    FilePathList paths_;
    FilePathList deletionPaths_;
    FileInfoList results_;
    FilePath commonDirPath_;
    const std::shared_ptr<const HashSet> cutFilesHashSet_;
    FilePath currentPath_;
};

} // namespace Fm

#endif // FM2_FILEINFOJOB_H

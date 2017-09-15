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

    explicit FileInfoJob(FilePathList paths, FilePath commonDirPath = FilePath(), const std::shared_ptr<const HashSet>& cutFilesHashSet = nullptr);

    const FilePathList& paths() const {
        return paths_;
    }

    const FileInfoList& files() const {
        return results_;
    }

Q_SIGNALS:
    void gotInfo(const FilePath& path, std::shared_ptr<const FileInfo>& info);

protected:
    void exec() override;

private:
    FilePathList paths_;
    FileInfoList results_;
    FilePath commonDirPath_;
    const std::shared_ptr<const HashSet> cutFilesHashSet_;
};

} // namespace Fm

#endif // FM2_FILEINFOJOB_H

#ifndef FM2_FILEINFOJOB_H
#define FM2_FILEINFOJOB_H

#include "libfmqtglobals.h"
#include "job.h"
#include "filepath.h"
#include "fileinfo.h"

namespace Fm2 {


class LIBFM_QT_API FileInfoJob : public Job {
    Q_OBJECT
public:

    explicit FileInfoJob(const FilePathList& paths): Job(), paths_{paths} {
    }

    explicit FileInfoJob(FilePathList&& paths): Job(), paths_{paths} {
    }

    virtual ~FileInfoJob() {
    }

    const FilePathList& paths() const {
        return paths_;
    }

    const FileInfoList& files() const {
        return results_;
    }

    void run() override;

Q_SIGNALS:
    void gotInfo(const FilePath& path, std::shared_ptr<const FileInfo>& info);

private:
    FilePathList paths_;
    FileInfoList results_;
};

} // namespace Fm2

#endif // FM2_FILEINFOJOB_H

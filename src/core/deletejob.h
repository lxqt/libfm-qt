#ifndef FM2_DELETEJOB_H
#define FM2_DELETEJOB_H

#include "libfmqtglobals.h"
#include "fileoperationjob.h"
#include "filepath.h"
#include "gioptrs.h"

namespace Fm {

class LIBFM_QT_API DeleteJob : public Fm::FileOperationJob {
    Q_OBJECT
public:
    DeleteJob(const FilePathList& paths): paths_{paths} {
    }

    DeleteJob(FilePathList&& paths): paths_{paths} {
    }

    ~DeleteJob() {
    }

protected:
    void exec() override;

private:
    bool deleteFile(const FilePath& path, GFileInfoPtr inf);
    bool deleteDirContent(const FilePath& path, GFileInfoPtr inf);

private:
    FilePathList paths_;
};

} // namespace Fm

#endif // FM2_DELETEJOB_H

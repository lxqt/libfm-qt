#ifndef FM2_TRASHJOB_H
#define FM2_TRASHJOB_H

#include "libfmqtglobals.h"
#include "fileoperationjob.h"
#include "filepath.h"

namespace Fm2 {

class LIBFM_QT_API TrashJob : public Fm2::FileOperationJob {
    Q_OBJECT
public:
    TrashJob(const FilePathList& paths);
    TrashJob(const FilePathList&& paths);

    void run() override;

    FilePathList unsupportedFiles() const {
        return unsupportedFiles_;
    }

private:
    FilePathList paths_;
    FilePathList unsupportedFiles_;
};

} // namespace Fm2

#endif // FM2_TRASHJOB_H

#ifndef FM_MOVEJOB_H
#define FM_MOVEJOB_H

#include "../libfmqtglobals.h"
#include "core/copyjob.h"

namespace Fm {

class LIBFM_QT_API MoveJob: public Fm::CopyJob {
public:
    explicit MoveJob(FilePathList srcPaths);
    explicit MoveJob(FilePathList srcPaths, FilePathList destPaths);
    explicit MoveJob(FilePathList srcPaths, const FilePath &destDirPath);

protected:
    void exec() override;

private:
};

} // namespace Fm

#endif // FM_MOVEJOB_H

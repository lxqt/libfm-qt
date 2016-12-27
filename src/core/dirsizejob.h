#ifndef FM2_DIRSIZEJOB_H
#define FM2_DIRSIZEJOB_H

#include "fileoperationjob.h"

namespace Fm2 {

class DirSizeJob : public Fm2::FileOperationJob {
    Q_OBJECT
public:
    DirSizeJob();
};

} // namespace Fm2

#endif // FM2_DIRSIZEJOB_H

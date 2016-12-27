#ifndef FM2_COPYJOB_H
#define FM2_COPYJOB_H

#include "fileoperationjob.h"

namespace Fm2 {

class CopyJob : public Fm2::FileOperationJob {
    Q_OBJECT
public:
    CopyJob();
};

} // namespace Fm2

#endif // FM2_COPYJOB_H

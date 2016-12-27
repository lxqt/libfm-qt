#ifndef FM2_MOVEJOB_H
#define FM2_MOVEJOB_H

#include "fileoperationjob.h"

namespace Fm2 {

class MoveJob : public Fm2::FileOperationJob {
    Q_OBJECT
public:
    MoveJob();
};

} // namespace Fm2

#endif // FM2_MOVEJOB_H

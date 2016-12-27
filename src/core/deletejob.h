#ifndef FM2_DELETEJOB_H
#define FM2_DELETEJOB_H

#include "fileoperationjob.h"

namespace Fm2 {

class DeleteJob : public Fm2::FileOperationJob {
    Q_OBJECT
public:
    DeleteJob();
};

} // namespace Fm2

#endif // FM2_DELETEJOB_H

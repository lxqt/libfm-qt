#ifndef FM2_TRASHJOB_H
#define FM2_TRASHJOB_H

#include "fileoperationjob.h"

namespace Fm2 {

class TrashJob : public Fm2::FileOperationJob {
    Q_OBJECT
public:
    TrashJob();
};

} // namespace Fm2

#endif // FM2_TRASHJOB_H

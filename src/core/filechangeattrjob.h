#ifndef FM2_FILECHANGEATTRJOB_H
#define FM2_FILECHANGEATTRJOB_H

#include "libfmqtglobals.h"
#include "fileoperationjob.h"

namespace Fm {

class LIBFM_QT_API FileChangeAttrJob : public Fm::FileOperationJob {
    Q_OBJECT
public:
    FileChangeAttrJob();
};

} // namespace Fm

#endif // FM2_FILECHANGEATTRJOB_H

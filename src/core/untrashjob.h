#ifndef FM2_UNTRASHJOB_H
#define FM2_UNTRASHJOB_H

#include "../libfmqtglobals.h"
#include "fileoperationjob.h"

namespace Fm {

class LIBFM_QT_API UntrashJob : public Fm::FileOperationJob {
public:
    UntrashJob();

protected:
    void exec() override;

private:
    bool ensure_parent_dir(GFile *orig_path);
};

} // namespace Fm

#endif // FM2_UNTRASHJOB_H

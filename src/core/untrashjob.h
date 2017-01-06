#ifndef FM2_UNTRASHJOB_H
#define FM2_UNTRASHJOB_H

#include "fileoperationjob.h"

namespace Fm2 {

class UntrashJob : public Fm2::FileOperationJob {
public:
    UntrashJob();

    void run() override;

private:
    bool ensure_parent_dir(GFile *orig_path);
};

} // namespace Fm2

#endif // FM2_UNTRASHJOB_H

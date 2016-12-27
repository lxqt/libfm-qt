#ifndef FM2_TOTALSIZEJOB_H
#define FM2_TOTALSIZEJOB_H

#include "fileoperationjob.h"
#include "filepath.h"

namespace Fm2 {

class TotalSizeJob : public Fm2::FileOperationJob {
    Q_OBJECT
public:
    enum Flags {
        DEFAULT = 0,
        FOLLOW_LINKS = 1<<0,
        SAME_FS = 1<<1,
        PREPARE_MOVE = 1<<2,
        PREPARE_DELETE = 1 <<3
    };

    TotalSizeJob();

    TotalSizeJob(const FilePathList& paths): paths_{paths} {
    }

    TotalSizeJob(FilePathList&& paths): paths_{paths} {
    }

    void run() override;

private:
    void run(FilePath& path, GObjectPtr<GFileInfo> &inf);

private:
    FilePathList paths_;

    Flags flags;
    goffset total_size;
    goffset total_ondisk_size;
    guint count;
    const char* dest_fs_id;
};

} // namespace Fm2

#endif // FM2_TOTALSIZEJOB_H

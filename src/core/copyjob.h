#ifndef FM2_COPYJOB_H
#define FM2_COPYJOB_H

#include "libfmqtglobals.h"
#include "fileoperationjob.h"
#include "gioptrs.h"

namespace Fm {

class LIBFM_QT_API CopyJob : public Fm::FileOperationJob {
    Q_OBJECT
public:

    enum class Mode {
        COPY,
        MOVE
    };

    CopyJob(const FilePathList& paths, const FilePath& destDirPath, Mode mode = Mode::COPY);

    CopyJob(const FilePathList&& paths, const FilePath&& destDirPath, Mode mode = Mode::COPY);

protected:
    void exec() override;

private:
    bool copyPath(const FilePath& srcPath, const FilePath& destPath, const char *destFileName);
    bool copyPath(const FilePath &srcPath, const GFileInfoPtr &srcInfo, const FilePath &destDirPath, const char *destFileName);
    bool copyRegularFile(const FilePath &srcPath, GFileInfoPtr srcFile, const FilePath& destPath);
    bool copySpecialFile(const FilePath &srcPath, GFileInfoPtr srcFile, const FilePath& destPath);
    bool copyDir(const FilePath &srcPath, GFileInfoPtr srcFile, const FilePath& destPath);
    bool makeDir(const FilePath &srcPath, GFileInfoPtr srcFile, const FilePath& dirPath);

    static void gfileProgressCallback(goffset current_num_bytes, goffset total_num_bytes, CopyJob* _this);

private:
    FilePathList srcPaths_;
    FilePath destDirPath_;
    Mode mode_;
    bool skip_dir_content;
};


} // namespace Fm

#endif // FM2_COPYJOB_H

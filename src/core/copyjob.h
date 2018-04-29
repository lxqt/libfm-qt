#ifndef FM2_COPYJOB_H
#define FM2_COPYJOB_H

#include "../libfmqtglobals.h"
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

    explicit CopyJob(FilePathList srcPaths, Mode mode = Mode::COPY);
    explicit CopyJob(FilePathList srcPaths, FilePathList destPaths, Mode mode = Mode::COPY);
    explicit CopyJob(FilePathList srcPaths, const FilePath &destDirPath, Mode mode = Mode::COPY);

    void setDestPaths(FilePathList destPaths);
    void setDestDirPath(const FilePath &destDirPath);

protected:
    void exec() override;

private:
    bool processPath(const FilePath& srcPath, const FilePath& destPath, const char *destFileName);
    bool movePath(const FilePath &srcPath, const GFileInfoPtr &srcInfo, const FilePath &destDirPath, const char *destFileName);
    bool copyPath(const FilePath &srcPath, const GFileInfoPtr &srcInfo, const FilePath &destDirPath, const char *destFileName, bool &deleteSrc);
    bool copyOrMoveFile(Mode mode, const FilePath &srcPath, const GFileInfoPtr& srcInfo, FilePath &destPath, bool &deleteSrc);
    bool copySpecialFile(const FilePath &srcPath, const GFileInfoPtr& srcInfo, FilePath& destPath);
    bool copyDir(const FilePath &srcPath, GFileInfoPtr srcInfo, FilePath &destPath, bool &deleteSrc);
    bool makeDir(const FilePath &srcPath, GFileInfoPtr srcInfo, FilePath &destPath);

    static void gfileCopyProgressCallback(goffset current_num_bytes, goffset total_num_bytes, CopyJob* _this);

protected:
    FilePathList srcPaths_;
    FilePathList destPaths_;
    Mode mode_;
    bool skipDirContent_;
};


} // namespace Fm

#endif // FM2_COPYJOB_H

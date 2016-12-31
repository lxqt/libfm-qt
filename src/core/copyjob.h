#ifndef FM2_COPYJOB_H
#define FM2_COPYJOB_H

#include "fileoperationjob.h"
#include "gobjectptr.h"

namespace Fm2 {

class CopyJob : public Fm2::FileOperationJob {
    Q_OBJECT
public:
    CopyJob(const FilePathList& paths, const FilePath& destDirPath);
    CopyJob(const FilePathList&& paths, const FilePath&& destDirPath);

    void run() override;

private:
    bool copyPath(const FilePath& srcPath, const FilePath& destPath, const char *destFileName);
    bool copyPath(const FilePath &srcPath, const GObjectPtr<GFileInfo> &srcInfo, const FilePath &destDirPath, const char *destFileName);
    bool copyRegularFile(const FilePath &srcPath, GObjectPtr<GFileInfo> srcFile, const FilePath& destPath);
    bool copySpecialFile(const FilePath &srcPath, GObjectPtr<GFileInfo> srcFile, const FilePath& destPath);
    bool copyDir(const FilePath &srcPath, GObjectPtr<GFileInfo> srcFile, const FilePath& destPath);
    bool makeDir(const FilePath &srcPath, GObjectPtr<GFileInfo> srcFile, const FilePath& dirPath);

    static void gfileProgressCallback(goffset current_num_bytes, goffset total_num_bytes, CopyJob* _this);

private:
    FilePathList srcPaths_;
    FilePath destDirPath_;
    bool skip_dir_content;
};


} // namespace Fm2

#endif // FM2_COPYJOB_H

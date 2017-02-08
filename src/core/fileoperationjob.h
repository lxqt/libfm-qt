#ifndef FM2_FILEOPERATIONJOB_H
#define FM2_FILEOPERATIONJOB_H

#include "libfmqtglobals.h"
#include "job.h"
#include <string>
#include <mutex>
#include <cstdint>
#include "fileinfo.h"
#include "filepath.h"

namespace Fm {

class LIBFM_QT_API FileOperationJob : public Fm::Job {
    Q_OBJECT
public:
    enum FileExistsAction {
        CANCEL = 0,
        OVERWRITE = 1<<0,
        RENAME = 1<<1,
        SKIP = 1<<2,
        SKIP_ERROR = 1<<3
    };

    FileOperationJob();

    bool totalAmount(std::uint64_t& fileSize, std::uint64_t& fileCount) const;

    bool finishedAmount(std::uint64_t& finishedSize, std::uint64_t& finishedCount) const;

    bool currentFileProgress(FilePath& path, std::uint64_t& totalSize, std::uint64_t& finishedSize) const;

Q_SIGNALS:

    void preparedToRun();

    // void currentFile(const char* file);

    // void progress(uint32_t percent);

    // to correctly handle the signal, connect with Qt::BlockingQueuedConnection so it's a sync call.
    void fileExists(const FileInfo& src, const FileInfo& dest, FileExistsAction& response, FilePath& newDest);

protected:

    FileExistsAction askRename(const FileInfo& src, const FileInfo& dest, FilePath& newDest);

    void setTotalAmount(std::uint64_t fileSize, std::uint64_t fileCount);

    void setFinishedAmount(std::uint64_t finishedSize, std::uint64_t finishedCount);

    void addFinishedAmount(std::uint64_t finishedSize, std::uint64_t finishedCount);

    void setCurrentFile(const FilePath &path);

    void setCurrentFileProgress(uint64_t totalSize, uint64_t finishedSize);

private:
    bool hasTotalAmount_;
    std::uint64_t totalSize_;
    std::uint64_t totalCount_;
    std::uint64_t finishedSize_;
    std::uint64_t finishedCount_;

    FilePath currentFile_;
    std::uint64_t currentFileSize_;
    std::uint64_t currentFileFinished_;
    mutable std::mutex mutex_;
};

} // namespace Fm

#endif // FM2_FILEOPERATIONJOB_H

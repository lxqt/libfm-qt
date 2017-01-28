#ifndef FM2_THUMBNAILJOB_H
#define FM2_THUMBNAILJOB_H

#include "libfmqtglobals.h"
#include "fileinfo.h"
#include "gioptrs.h"
#include "job.h"
#include <QThreadPool>

namespace Fm2 {

class LIBFM_QT_API ThumbnailJob: public Job {
    Q_OBJECT
public:

    ThumbnailJob(FileInfoList files, int size);

    int size() const {
        return size_;
    }

    void run() override;

    static QThreadPool* threadPool();


    static void setLocalFilesOnly(bool value) {
        localFilesOnly_ = value;
        if(fm_config) {
            fm_config->thumbnail_local = localFilesOnly_;
        }
    }

    static bool localFilesOnly() {
        return localFilesOnly_;
    }

    static int maxThumbnailFileSize() {
        return maxThumbnailFileSize_;
    }

    static void setMaxThumbnailFileSize(int size) {
        maxThumbnailFileSize_ = size;
        if(fm_config) {
            fm_config->thumbnail_max = maxThumbnailFileSize_;
        }
    }

Q_SIGNALS:
    void thumbnailLoaded(const std::shared_ptr<const FileInfo>& file, int size, const QImage& thumbnail);

private:

    bool isSupportedImageType(const std::shared_ptr<const MimeType>& mimeType) const;

    bool isThumbnailOutdated(const std::shared_ptr<const FileInfo>& file, const QImage& thumbnail) const;

    QImage generateThumbnail(const std::shared_ptr<const FileInfo>& file, const FilePath& origPath, const char* uri, const QString& thumbnailFilename);

    QImage readImageFromStream(GInputStream* stream, size_t len);

    QImage loadForFile(const std::shared_ptr<const FileInfo>& file);

    bool readJpegExif(GInputStream* stream, QImage& thumbnail, int& rotate_degrees);

private:
    FileInfoList files_;
    int size_;
    std::vector<QImage> results_;
    GCancellablePtr cancellable_;

    static QThreadPool* threadPool_;

    static bool localFilesOnly_;
    static int maxThumbnailFileSize_;
};

} // namespace Fm2

#endif // FM2_THUMBNAILJOB_H

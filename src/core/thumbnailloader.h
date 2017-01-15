#ifndef FM2_THUMBNAILLOADER_H
#define FM2_THUMBNAILLOADER_H

#include "libfmqtglobals.h"
#include <functional>
#include "fileinfo.h"
#include "gioptrs.h"

#include <QRunnable>
#include <QThreadPool>

namespace Fm2 {

class LIBFM_QT_API ThumbnailLoader: public QRunnable {
public:

    ThumbnailLoader(std::shared_ptr<const Fm2::FileInfo> file, int size, std::function<void(ThumbnailLoader& loader)> callback = nullptr);

    QImage result() const {
        return result_;
    }

    void cancel() {
        g_cancellable_cancel(cancellable_.get());
    }

    bool isCancelled() const {
        return g_cancellable_is_cancelled(cancellable_.get());
    }

    void run() override;

    static QThreadPool* threadPool();

    const std::shared_ptr<const Fm2::FileInfo>& file() const {
        return file_;
    }

    int size() const {
        return size_;
    }

private:
    QImage readImageFromStream(GInputStream *stream, size_t len);

    bool isSupportedImageType() const;
    bool isThumbnailOutdated(const QImage& thumbnail) const;
    QImage generateThumbnail(const FilePath &origPath, const char* uri, const QString& thumbnailFilename);

private:
    std::shared_ptr<const Fm2::FileInfo> file_;
    int size_;
    QImage result_;
    GCancellablePtr cancellable_;
    std::function<void(ThumbnailLoader& loader)> callback_;

    static QThreadPool* threadPool_;
};

} // namespace Fm2

#endif // FM2_THUMBNAILLOADER_H

#ifndef FM2_BOOKMARKS_H
#define FM2_BOOKMARKS_H

#include "libfmqtglobals.h"
#include <QObject>
#include "gobjectptr.h"
#include "fileinfo.h"


namespace Fm2 {

class LIBFM_QT_API BookmarkItem {
public:
    friend class Bookmarks;

    BookmarkItem(const FilePath& path, const QString name): path_{path}, name_{name} {
    }

    const QString& name() const {
        return name_;
    }

    const FilePath& path() const {
        return path_;
    }

    const std::shared_ptr<const FmFileInfo>& info() const {
        return info_;
    }

private:
    void setInfo(const std::shared_ptr<const FmFileInfo>& info) {
        info_ = info;
    }

    void setName(const QString& name) {
        name_ = name;
    }

private:
    FilePath path_;
    QString name_;
    std::shared_ptr<const FmFileInfo> info_;
};


class LIBFM_QT_API Bookmarks : public QObject {
    Q_OBJECT
public:
    explicit Bookmarks(QObject* parent = 0);

    ~Bookmarks();

    const std::shared_ptr<const BookmarkItem> &insert(const FilePath& path, const QString& name, int pos);

    void remove(const std::shared_ptr<const BookmarkItem>& item);

    void reorder(const std::shared_ptr<const BookmarkItem> &item, int pos);

    void rename(const std::shared_ptr<const BookmarkItem>& item, QString new_name);

    const std::vector<std::shared_ptr<const BookmarkItem>>& items() const {
        return items_;
    }

    static std::shared_ptr<Bookmarks> globalInstance();

Q_SIGNALS:
    void changed();

private Q_SLOTS:
    void save();

private:
    void load();
    void queueSave();

    static void _onFileChanged(GFileMonitor* mon, GFile* gf, GFile* other, GFileMonitorEvent evt, Bookmarks* _this) {
        _this->onFileChanged(mon, gf, other, evt);
    }
    void onFileChanged(GFileMonitor* mon, GFile* gf, GFile* other, GFileMonitorEvent evt);

private:
    FilePath file;
    GObjectPtr<GFileMonitor> mon;
    std::vector<std::shared_ptr<const BookmarkItem>> items_;
    static std::weak_ptr<Bookmarks> globalInstance_;
    bool idle_handler;
};

} // namespace Fm2

#endif // FM2_BOOKMARKS_H

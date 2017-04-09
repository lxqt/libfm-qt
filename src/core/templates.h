#ifndef TEMPLATES_H
#define TEMPLATES_H

#include "../libfmqtglobals.h"

#include <QObject>
#include <memory>
#include <vector>
#include "folder.h"
#include "fileinfo.h"
#include "mimetype.h"
#include "iconinfo.h"

namespace Fm {

class LIBFM_QT_API TemplateItem {
public:
    explicit TemplateItem(std::shared_ptr<const FileInfo> fileInfo);

    QString name() const {
        return fileInfo_->displayName();
    }

    std::shared_ptr<const IconInfo> icon() const {
        return fileInfo_->icon();
    }

    std::shared_ptr<const FileInfo> fileInfo() const {
        return fileInfo_;
    }

    std::shared_ptr<const MimeType> mimeType() const {
        return fileInfo_->mimeType();
    }

    FilePath filePath() const;

    bool createNewFile(FilePath& dest, GErrorPtr& err) const;

private:
    std::shared_ptr<const FileInfo> fileInfo_;
};


class LIBFM_QT_API Templates : public QObject {
    Q_OBJECT
public:
    explicit Templates();

    // FIXME: the first call to this method will get no templates since dir loading is in progress.
    static std::shared_ptr<Templates> globalInstance();

    void forEachItem(std::function<void (const std::shared_ptr<const TemplateItem>&)> func) const {
        for(const auto& item : items_) {
            func(item);
        }
    }

    std::vector<std::shared_ptr<const TemplateItem>> items() const {
        std::vector<std::shared_ptr<const TemplateItem>> tmp_items;
        for(auto& item: items_) {
            tmp_items.emplace_back(item);
        }
        return tmp_items;
    }

    bool hasTemplates() const {
        return !items_.empty();
    }

private:
    void addTemplateDir(const char* dirPathName);

private Q_SLOTS:
    void onFilesAdded(FileInfoList& addedFiles);

    void onFilesChanged(std::vector<FileInfoPair>& changePairs);

    void onFilesRemoved(FileInfoList& removedFiles);

private:
    std::vector<std::shared_ptr<TemplateItem>> items_;
    std::vector<std::shared_ptr<Folder>> templateFolders_;
    static std::shared_ptr<Templates> globalInstance_;
};

} // namespace Fm

#endif // TEMPLATES_H

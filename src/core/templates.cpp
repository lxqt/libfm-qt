#include "templates.h"
#include <algorithm>
#include <QDebug>

using namespace std;

namespace Fm {

std::shared_ptr<Templates> Templates::globalInstance_;

TemplateItem::TemplateItem(std::shared_ptr<const FileInfo> file): fileInfo_{file} {
}

FilePath TemplateItem::filePath() const {
    auto& target = fileInfo_->target();
    if(fileInfo_->isDesktopEntry() && !target.empty()) {
        if(target[0] == '/') { // target is an absolute path
            return FilePath::fromLocalPath(target.c_str());
        }
        else { // resolve relative path
            return fileInfo_->dirPath().relativePath(target.c_str());
        }
    }
    return fileInfo_->path();
}

bool TemplateItem::createNewFile(FilePath &dest, GErrorPtr &err) const {
    return false;
}

Templates::Templates() : QObject() {
    auto* data_dirs = g_get_system_data_dirs();
    // system-wide template dirs
    for(auto data_dir = data_dirs; *data_dir; ++data_dir) {
        CStrPtr dir_name{g_build_filename(*data_dir, "templates", nullptr)};
        addTemplateDir(dir_name.get());
    }

    // user-specific template dir
    CStrPtr dir_name{g_build_filename(g_get_user_data_dir(), "templates", nullptr)};
    addTemplateDir(dir_name.get());

    // $XDG_TEMPLATES_DIR (FIXME: this might change at runtime)
    addTemplateDir(g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES));
}


shared_ptr<Templates> Templates::globalInstance() {
    if(!globalInstance_) {
        globalInstance_ = make_shared<Templates>();
    }
    return globalInstance_;
}

void Templates::addTemplateDir(const char* dirPathName) {
    auto dir_path = FilePath::fromLocalPath(dirPathName);
    auto folder = Folder::fromPath(dir_path);
    connect(folder.get(), &Folder::filesAdded, this, &Templates::onFilesAdded);
    connect(folder.get(), &Folder::filesChanged, this, &Templates::onFilesChanged);
    connect(folder.get(), &Folder::filesRemoved, this, &Templates::onFilesRemoved);
    templateFolders_.emplace_back(std::move(folder));
}

void Templates::onFilesAdded(FileInfoList& addedFiles) {
    for(auto& file : addedFiles) {
        if(file->isHidden()) {
            continue;
        }
        // FIXME: handle subdirs
        items_.push_back(std::make_shared<TemplateItem>(file));
        if(file->isDir()) {
            addTemplateDir(file->path().localPath().get());
        }
    }
}

void Templates::onFilesChanged(std::vector<FileInfoPair>& changePairs) {
    for(auto& change: changePairs) {
        auto& old_file = change.first;
        auto& new_file = change.second;
        if(old_file->isHidden() || old_file->isDir()) {
            continue;
        }
        auto it = std::find_if(items_.begin(), items_.end(), [&](const std::shared_ptr<TemplateItem>& item) {
            return item->fileInfo() == old_file;
        });
        if(it != items_.end()) {
            *it = std::make_shared<TemplateItem>(new_file);
        }
    }
}

void Templates::onFilesRemoved(FileInfoList& removedFiles) {
    for(auto& file : removedFiles) {
        if(file->isHidden()) {
            continue;
        }
        bool isDir = file->isDir();
        auto filePath = isDir ? file->path() : FilePath{};
        auto it = std::remove_if(items_.begin(), items_.end(), [&](const std::shared_ptr<TemplateItem>& item) {
            if(isDir) {
                return filePath.isPrefixOf(item->fileInfo()->path());
            }
            return item->fileInfo() == file;
        });
        items_.erase(it, items_.end());
    }
}

} // namespace Fm

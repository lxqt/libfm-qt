/*
 * Copyright (C) 2016 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __LIBFM2_QT_FM_FOLDER_H__
#define __LIBFM2_QT_FM_FOLDER_H__

#include <gio/gio.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>

#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"

#include "gobjectptr.h"
#include "fileinfo.h"
#include "gsignalhandler.h"


namespace Fm2 {

class DirListJob;
class FileSystemInfoJob;


class LIBFM_QT_API Folder: public QObject {
    Q_OBJECT
public:

    explicit Folder();

    explicit Folder(const FilePath& path);

    virtual ~Folder();

    static std::shared_ptr<Folder> fromPath(const FilePath& path);

    bool makeDirectory(const char* name, GError** error);

    void queryFilesystemInfo();

    bool getFilesystemInfo(uint64_t* total_size, uint64_t* free_size) const;

    void reload();

    bool isIncremental() const;

    bool isValid() const;

    bool isLoaded() const;

    std::shared_ptr<const FileInfo> getFileByName(const char* name) const;

    bool isEmpty() const;

    FileInfoList getFiles() const;

    const FilePath& getPath() const;

    const std::shared_ptr<const FileInfo> &getInfo() const;

    void unblockUpdates();

    void blockUpdates();

    void forEachFile(std::function<void (const std::shared_ptr<const FileInfo>&)> func) const {
        std::lock_guard<std::mutex> lock{mutex_};
        for(auto it = files_.begin(); it != files_.end(); ++it) {
            func(it->second);
        }
    }

Q_SIGNALS:
    void startLoading();

    void finishLoading();

    void filesAdded(FileInfoList& addedFiles);

    void filesChanged(std::vector<FileInfoPair>& changePairs);

    void filesRemoved(FileInfoList& removedFiles);

    void removed();

    void changed();

    void unmount();

    void contentChanged();

    void fileSystemChanged();

private:

    void onFileChangeEvents(GFileMonitor* monitor, GFile* file, GFile* other_file, GFileMonitorEvent event_type);
    void onDirChanged(GFileMonitorEvent event_type);

    void queueUpdate();
    void queueReload();

    bool eventFileAdded(const FilePath &path);
    bool eventFileChanged(const FilePath &path);
    void eventFileDeleted(const FilePath &path);

private Q_SLOTS:

    void processPendingChanges();

    void onDirListFinished();

    void onFileSystemInfoFinished();

    void onFileInfoFinished();

    void onIdleReload();

private:
    FilePath dirPath_;
    GObjectPtr<GFileMonitor> dirMonitor_;
    GSignalHandler<Folder, GFileMonitor, void, GFile*, GFile*, GFileMonitorEvent> dirMonitorChangedHandler_;

    std::shared_ptr<const FileInfo> dirInfo_;
    DirListJob* dirlist_job;
    FileSystemInfoJob* fsInfoJob_;

    /* for file monitor */
    bool has_idle_reload_handler;
    bool has_idle_update_handler;
    std::vector<FilePath> paths_to_add;
    std::vector<FilePath> paths_to_update;
    std::vector<FilePath> paths_to_del;
    // GSList* pending_jobs;
    bool pending_change_notify;
    bool filesystem_info_pending;

    bool wants_incremental;
    bool stop_emission; /* don't set it 1 bit to not lock other bits */

    std::unordered_map<const char*, std::shared_ptr<const FileInfo>, CStrHash, CStrEqual> files_;

    /* filesystem info - set in query thread, read in main */
    uint64_t fs_total_size;
    uint64_t fs_free_size;
    GObjectPtr<GCancellable> fs_size_cancellable;

    bool has_fs_info : 1;
    bool fs_info_not_avail : 1;
    bool defer_content_test : 1;

    static std::unordered_map<FilePath, std::weak_ptr<Folder>, FilePathHash> cache_;
    static std::mutex mutex_;
};

}

#endif // __LIBFM_QT_FM2_FOLDER_H__

/*
 * Copyright (C) 2012 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#ifndef FM_FILEOPERATION_H
#define FM_FILEOPERATION_H

#include "libfmqtglobals.h"
#include <QObject>
#include <QElapsedTimer>
#include <libfm/fm.h>
#include "core/filepath.h"
#include "core/fileoperationjob.h"

class QTimer;

namespace Fm {

class FileOperationDialog;

class LIBFM_QT_API FileOperation : public QObject {
    Q_OBJECT
public:
    enum Type {
        Copy = FM_FILE_OP_COPY,
        Move = FM_FILE_OP_MOVE,
        Link = FM_FILE_OP_LINK,
        Delete = FM_FILE_OP_DELETE,
        Trash = FM_FILE_OP_TRASH,
        UnTrash = FM_FILE_OP_UNTRASH,
        ChangeAttr = FM_FILE_OP_CHANGE_ATTR
    };

public:
    explicit FileOperation(Type type, Fm::FilePathList srcFiles, QObject* parent = 0);
    virtual ~FileOperation();

    void setDestination(Fm::FilePath dest);

    void setChmod(mode_t newMode, mode_t newModeMask) {
        if(legacyJob_) {
            fm_file_ops_job_set_chmod(legacyJob_, newMode, newModeMask);
        }
    }

    void setChown(gint uid, gint gid) {
        if(legacyJob_) {
            fm_file_ops_job_set_chown(legacyJob_, uid, gid);
        }
    }

    // This only work for change attr jobs.
    void setRecursiveChattr(bool recursive) {
        if(legacyJob_) {
            fm_file_ops_job_set_recursive(legacyJob_, (gboolean)recursive);
        }
    }

    bool run();

    void cancel() {
        if(legacyJob_) {
            fm_job_cancel(FM_JOB(legacyJob_));
        }
        else if(job_) {
            job_->cancel();
        }
    }

    bool isRunning() const {
        if(legacyJob_) {
            return legacyJob_ ? fm_job_is_running(FM_JOB(legacyJob_)) : false;
        }
        else if(job_) {
            return true;
        }
        return false;
    }

    bool isCancelled() const {
        if(legacyJob_) {
            return legacyJob_ ? fm_job_is_cancelled(FM_JOB(legacyJob_)) : false;
        }
        else if(job_) {
            return job_->isCancelled();
        }
        return false;
    }

    FmFileOpsJob* legacyJob() {
        return legacyJob_;
    }

    Fm::FileOperationJob* job() {
        return job_;
    }

    bool autoDestroy() {
        return autoDestroy_;
    }
    void setAutoDestroy(bool destroy = true) {
        autoDestroy_ = destroy;
    }

    Type type() {
        return type_;
    }

    // convinient static functions
    static FileOperation* copyFiles(Fm::FilePathList srcFiles, Fm::FilePath dest, QWidget* parent = 0);
    static FileOperation* moveFiles(Fm::FilePathList srcFiles, Fm::FilePath dest, QWidget* parent = 0);
    static FileOperation* symlinkFiles(Fm::FilePathList srcFiles, Fm::FilePath dest, QWidget* parent = 0);
    static FileOperation* deleteFiles(Fm::FilePathList srcFiles, bool promp = true, QWidget* parent = 0);
    static FileOperation* trashFiles(Fm::FilePathList srcFiles, bool promp = true, QWidget* parent = 0);
    static FileOperation* unTrashFiles(Fm::FilePathList srcFiles, QWidget* parent = 0);
    static FileOperation* changeAttrFiles(Fm::FilePathList srcFiles, QWidget* parent = 0);

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void onJobPrepared();
    void onJobFinish();
    void onJobCancalled();
    void onJobError(const GErrorPtr& err, Fm::Job::ErrorSeverity severity, Fm::Job::ErrorAction& response);
    void onJobFileExists(const FileInfo& src, const FileInfo& dest, Fm::FileOperationJob::FileExistsAction& response, FilePath& newDest);

private:
    static gint onFileOpsJobAsk(FmFileOpsJob* job, const char* question, char* const* options, FileOperation* pThis);
    static gint onFileOpsJobAskRename(FmFileOpsJob* job, FmFileInfo* src, FmFileInfo* dest, char** new_name, FileOperation* pThis);
    static FmJobErrorAction onFileOpsJobError(FmFileOpsJob* job, GError* err, FmJobErrorSeverity severity, FileOperation* pThis);
    static void onFileOpsJobPrepared(FmFileOpsJob* job, FileOperation* pThis);
    static void onFileOpsJobCurFile(FmFileOpsJob* job, const char* cur_file, FileOperation* pThis);
    static void onFileOpsJobPercent(FmFileOpsJob* job, guint percent, FileOperation* pThis);
    static void onFileOpsJobFinished(FmFileOpsJob* job, FileOperation* pThis);
    static void onFileOpsJobCancelled(FmFileOpsJob* job, FileOperation* pThis);

    void disconnectJob();
    void showDialog();

    void pauseElapsedTimer() {
        if(Q_LIKELY(elapsedTimer_ != nullptr)) {
            lastElapsed_ += elapsedTimer_->elapsed();
            elapsedTimer_->invalidate();
        }
    }

    void resumeElapsedTimer() {
        if(Q_LIKELY(elapsedTimer_ != nullptr)) {
            elapsedTimer_->start();
        }
    }

    qint64 elapsedTime() {
        if(Q_LIKELY(elapsedTimer_ != nullptr)) {
            return lastElapsed_ + elapsedTimer_->elapsed();
        }
        return 0;
    }

private Q_SLOTS:
    void onUiTimeout();

private:
    Type type_;
    Fm::FileOperationJob* job_;  // new C++ job implementations
    FmFileOpsJob* legacyJob_;   // legacy C jobs depending on libfm (backward compatibility)
    FileOperationDialog* dlg_;
    Fm::FilePath destPath;
    Fm::FilePath curFilePath_;
    Fm::FilePathList srcPaths_;
    QTimer* uiTimer_;
    QElapsedTimer* elapsedTimer_;
    qint64 lastElapsed_;
    bool updateRemainingTime_;
    QString curFile;
    bool autoDestroy_;
};

}

#endif // FM_FILEOPERATION_H

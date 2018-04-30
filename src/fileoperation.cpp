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


#include "fileoperation.h"
#include "fileoperationdialog.h"
#include <QTimer>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QDebug>
#include "path.h"

#include "core/deletejob.h"
#include "core/trashjob.h"
#include "core/untrashjob.h"
#include "core/filetransferjob.h"
#include "core/filechangeattrjob.h"
#include "utilities.h"

namespace Fm {

#define SHOW_DLG_DELAY  1000

FileOperation::FileOperation(Type type, Fm::FilePathList srcPaths, QObject* parent):
    QObject(parent),
    type_{type},
    job_{nullptr},
    dlg_{nullptr},
    srcPaths_{std::move(srcPaths)},
    uiTimer_(nullptr),
    elapsedTimer_(nullptr),
    lastElapsed_(0),
    updateRemainingTime_(true),
    autoDestroy_(true) {

    switch(type_) {
    case Copy:
        job_ = new FileTransferJob(srcPaths_, FileTransferJob::Mode::COPY);
        break;
    case Move:
        job_ = new FileTransferJob(srcPaths_, FileTransferJob::Mode::MOVE);
        break;
    case Link:
        job_ = new FileTransferJob(srcPaths_, FileTransferJob::Mode::LINK);
        break;
    case Delete:
        job_ = new Fm::DeleteJob(srcPaths_);
        break;
    case Trash:
        job_ = new Fm::TrashJob(srcPaths_);
        break;
    case UnTrash:
        job_ = new Fm::UntrashJob(srcPaths_);
        break;
    case ChangeAttr:
        job_ = new Fm::FileChangeAttrJob(srcPaths_);
        break;
    default:
        break;
    }

    if(job_) {
        // automatically delete the job object when it's finished.
        job_->setAutoDelete(true);

        // new C++ jobs
        connect(job_, &Fm::Job::finished, this, &Fm::FileOperation::onJobFinish);
        connect(job_, &Fm::Job::cancelled, this, &Fm::FileOperation::onJobCancalled);
        connect(job_, &Fm::Job::error, this, &Fm::FileOperation::onJobError, Qt::BlockingQueuedConnection);
        connect(job_, &Fm::FileOperationJob::fileExists, this, &Fm::FileOperation::onJobFileExists, Qt::BlockingQueuedConnection);

        // we block the job deliberately until we prepare to start (initiailize the timer) so we can calculate elapsed time correctly.
        connect(job_, &Fm::FileOperationJob::preparedToRun, this, &Fm::FileOperation::onJobPrepared, Qt::BlockingQueuedConnection);
    }
}

void FileOperation::disconnectJob() {
    if(job_) {
        disconnect(job_, &Fm::Job::finished, this, &Fm::FileOperation::onJobFinish);
        disconnect(job_, &Fm::Job::cancelled, this, &Fm::FileOperation::onJobCancalled);
        disconnect(job_, &Fm::Job::error, this, &Fm::FileOperation::onJobError);
        disconnect(job_, &Fm::FileOperationJob::fileExists, this, &Fm::FileOperation::onJobFileExists);
        disconnect(job_, &Fm::FileOperationJob::preparedToRun, this, &Fm::FileOperation::onJobPrepared);
    }
}

FileOperation::~FileOperation() {
    if(uiTimer_) {
        uiTimer_->stop();
        delete uiTimer_;
        uiTimer_ = nullptr;
    }
    if(elapsedTimer_) {
        delete elapsedTimer_;
        elapsedTimer_ = nullptr;
    }
}

void FileOperation::setDestination(Fm::FilePath dest) {
    destPath_ = std::move(dest);
    switch(type_) {
    case Copy:
    case Move:
    case Link:
        if(job_) {
            static_cast<FileTransferJob*>(job_)->setDestDirPath(destPath_);
        }
        break;
    };
}

void FileOperation::setChmod(mode_t newMode, mode_t newModeMask) {
    if(job_) {
        auto job = static_cast<FileChangeAttrJob*>(job_);
        job->setFileModeEnabled(true);
        job->setFileMode(newMode, newModeMask);
    }
}

void FileOperation::setChown(uid_t uid, gid_t gid) {
    if(job_) {
        auto job = static_cast<FileChangeAttrJob*>(job_);
        if(uid != INVALID_UID) {
            job->setOwnerEnabled(true);
            job->setOwner(uid);
        }
        if(gid != INVALID_GID) {
            job->setGroupEnabled(true);
            job->setGroup(gid);
        }
    }
}

void FileOperation::setRecursiveChattr(bool recursive) {
    if(job_) {
        auto job = static_cast<FileChangeAttrJob*>(job_);
        job->setRecursive(recursive);
    }
}

bool FileOperation::run() {
    delete uiTimer_;
    // run the job
    uiTimer_ = new QTimer();
    uiTimer_->start(SHOW_DLG_DELAY);
    connect(uiTimer_, &QTimer::timeout, this, &FileOperation::onUiTimeout);

    if(job_) {
        job_->runAsync();
        return true;
    }
    return false;
}

void FileOperation::cancel() {
    if(job_) {
        job_->cancel();
    }
}

void FileOperation::onUiTimeout() {
    if(dlg_) {
        // estimate remaining time based on past history
        if(job_) {
            Fm::FilePath curFilePath = job_->currentFile();
            std::uint64_t totalSize, totalCount, finishedSize, finishedCount;
            // calculate current job progress
            job_->totalAmount(totalSize, totalCount);
            job_->finishedAmount(finishedSize, finishedCount);
            // update progress bar
            double finishedRatio = job_->progress();
            if(finishedRatio > 0.0 && updateRemainingTime_) {
                dlg_->setPercent(int(finishedRatio * 100));
                dlg_->setDataTransferred(finishedSize, totalSize);

                double remainRatio = 1.0 - finishedRatio;
                gint64 remaining = elapsedTime() * (remainRatio / finishedRatio) / 1000;
                // qDebug("etime: %llu, finished: %lf, remain:%lf, remaining secs: %llu",
                //        elapsedTime(), finishedRatio, remainRatio, remaining);
                dlg_->setRemainingTime(remaining);
            }
            // update currently processed file
            if(curFilePath_ != curFilePath) {
                curFilePath_ = std::move(curFilePath);
                // FIXME: make this cleaner
                curFile = QString::fromUtf8(curFilePath_.toString().get());
                dlg_->setCurFile(curFile);
            }
        }
        // this timeout slot is called every 0.5 second.
        // by adding this flag, we can update remaining time every 1 second.
        updateRemainingTime_ = !updateRemainingTime_;
    }
    else {
        showDialog();
    }
}

void FileOperation::showDialog() {
    if(!dlg_) {
        dlg_ = new FileOperationDialog(this);
        dlg_->setSourceFiles(srcPaths_);

        if(destPath_) {
            dlg_->setDestPath(destPath_);
        }

        if(curFile.isEmpty()) {
            dlg_->setPrepared();
            dlg_->setCurFile(curFile);
        }
        uiTimer_->setInterval(500); // change the interval of the timer
        // now the timer is used to update current file display
        dlg_->show();
    }
}

gint FileOperation::onFileOpsJobAsk(FmFileOpsJob* /*job*/, const char* question, char* const* options, FileOperation* pThis) {
    pThis->pauseElapsedTimer();
    pThis->showDialog();
    int ret = pThis->dlg_->ask(QString::fromUtf8(question), options);
    pThis->resumeElapsedTimer();
    return ret;
}

void FileOperation::onJobFileExists(const FileInfo& src, const FileInfo& dest, Fm::FileOperationJob::FileExistsAction& response, FilePath& newDest) {
    pauseElapsedTimer();
    showDialog();
    response = dlg_->askRename(src, dest, newDest);
    resumeElapsedTimer();
}

gint FileOperation::onFileOpsJobAskRename(FmFileOpsJob* /*job*/, FmFileInfo* src, FmFileInfo* dest, char** new_name, FileOperation* pThis) {
    qDebug("not implemented");
    return 0;
}

void FileOperation::onJobCancalled() {
    qDebug("file operation is cancelled!");
}

void FileOperation::onFileOpsJobCancelled(FmFileOpsJob* /*job*/, FileOperation* pThis) {
    pThis->onJobCancalled();
}

void FileOperation::onFileOpsJobCurFile(FmFileOpsJob* /*job*/, const char* cur_file, FileOperation* pThis) {
    pThis->curFile = QString::fromUtf8(cur_file);

    // We update the current file name in a timeout slot because drawing a string
    // in the UI is expansive. Updating the label text too often cause
    // significant impact on performance.
    // if(pThis->dlg)
    //  pThis->dlg->setCurFile(pThis->curFile);
}

void FileOperation::onJobError(const GErrorPtr& err, Fm::Job::ErrorSeverity severity, Fm::Job::ErrorAction& response) {
    pauseElapsedTimer();
    showDialog();
    response = Fm::Job::ErrorAction(dlg_->error(err.get(), FmJobErrorSeverity(severity)));
    resumeElapsedTimer();
}

FmJobErrorAction FileOperation::onFileOpsJobError(FmFileOpsJob* /*job*/, GError* err, FmJobErrorSeverity severity, FileOperation* pThis) {
    pThis->pauseElapsedTimer();
    pThis->showDialog();
    FmJobErrorAction act = pThis->dlg_->error(err, severity);
    pThis->resumeElapsedTimer();
    return act;
}

void FileOperation::onFileOpsJobFinished(FmFileOpsJob* /*job*/, FileOperation* pThis) {
    pThis->onJobFinish();
}

void FileOperation::onFileOpsJobPercent(FmFileOpsJob* job, guint percent, FileOperation* pThis) {
    if(pThis->dlg_) {
        pThis->dlg_->setPercent(percent);
        pThis->dlg_->setDataTransferred(job->finished, job->total);
    }
}

void FileOperation::onJobPrepared() {
    if(!elapsedTimer_) {
        elapsedTimer_ = new QElapsedTimer();
        elapsedTimer_->start();
    }
    if(dlg_) {
        dlg_->setPrepared();
    }
}

void FileOperation::onFileOpsJobPrepared(FmFileOpsJob* /*job*/, FileOperation* pThis) {
    pThis->onJobPrepared();
}

void FileOperation::onJobFinish() {
    disconnectJob();

    if(uiTimer_) {
        uiTimer_->stop();
        delete uiTimer_;
        uiTimer_ = nullptr;
    }

    if(dlg_) {
        dlg_->done(QDialog::Accepted);
        delete dlg_;
        dlg_ = nullptr;
    }
    Q_EMIT finished();

    // special handling for trash job
    if(type_ == Trash && !job_->isCancelled()) {
        auto trashJob = static_cast<Fm::TrashJob*>(job_);
        /* some files cannot be trashed because underlying filesystems don't support it. */
        auto unsupportedFiles = trashJob->unsupportedFiles();
        if(!unsupportedFiles.empty()) { /* delete them instead */
            /* FIXME: parent window might be already destroyed! */
            QWidget* parent = nullptr; // FIXME: currently, parent window is not set
            if(QMessageBox::question(parent, tr("Error"),
                                     tr("Some files cannot be moved to trash can because "
                                        "the underlying file systems don't support this operation.\n"
                                        "Do you want to delete them instead?")) == QMessageBox::Yes) {
                deleteFiles(std::move(unsupportedFiles), false);
            }
        }
    }

    if(autoDestroy_) {
        delete this;
    }
}

// static
FileOperation* FileOperation::copyFiles(Fm::FilePathList srcFiles, Fm::FilePath dest, QWidget* parent) {
    FileOperation* op = new FileOperation(FileOperation::Copy, std::move(srcFiles), parent);
    op->setDestination(dest);
    op->run();
    return op;
}

// static
FileOperation* FileOperation::moveFiles(Fm::FilePathList srcFiles, Fm::FilePath dest, QWidget* parent) {
    FileOperation* op = new FileOperation(FileOperation::Move, std::move(srcFiles), parent);
    op->setDestination(dest);
    op->run();
    return op;
}

//static
FileOperation* FileOperation::symlinkFiles(Fm::FilePathList srcFiles, Fm::FilePath dest, QWidget* parent) {
    FileOperation* op = new FileOperation(FileOperation::Link, std::move(srcFiles), parent);
    op->setDestination(dest);
    op->run();
    return op;
}

//static
FileOperation* FileOperation::deleteFiles(Fm::FilePathList srcFiles, bool prompt, QWidget* parent) {
    if(prompt) {
        int result = QMessageBox::warning(parent, tr("Confirm"),
                                          tr("Do you want to delete the selected files?"),
                                          QMessageBox::Yes | QMessageBox::No,
                                          QMessageBox::No);
        if(result != QMessageBox::Yes) {
            return nullptr;
        }
    }

    FileOperation* op = new FileOperation(FileOperation::Delete, std::move(srcFiles));
    op->run();
    return op;
}

//static
FileOperation* FileOperation::trashFiles(Fm::FilePathList srcFiles, bool prompt, QWidget* parent) {
    if(prompt) {
        int result = QMessageBox::warning(parent, tr("Confirm"),
                                          tr("Do you want to move the selected files to trash can?"),
                                          QMessageBox::Yes | QMessageBox::No,
                                          QMessageBox::No);
        if(result != QMessageBox::Yes) {
            return nullptr;
        }
    }

    FileOperation* op = new FileOperation(FileOperation::Trash, std::move(srcFiles));
    op->run();
    return op;
}

//static
FileOperation* FileOperation::unTrashFiles(Fm::FilePathList srcFiles, QWidget* parent) {
    FileOperation* op = new FileOperation(FileOperation::UnTrash, std::move(srcFiles), parent);
    op->run();
    return op;
}

// static
FileOperation* FileOperation::changeAttrFiles(Fm::FilePathList srcFiles, QWidget* parent) {
    //TODO
    FileOperation* op = new FileOperation(FileOperation::ChangeAttr, std::move(srcFiles), parent);
    op->run();
    return op;
}


} // namespace Fm

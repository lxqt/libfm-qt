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


#include "fileoperationdialog.h"
#include "fileoperation.h"
#include "renamedialog.h"
#include <QLabel>
#include <QMessageBox>
#include <libfm/fm-config.h>
#include "utilities.h"
#include "ui_file-operation-dialog.h"

#include "core/compat_p.h"


namespace Fm {

FileOperationDialog::FileOperationDialog(FileOperation* _operation):
    QDialog(nullptr),
    operation(_operation),
    defaultOption(-1),
    ignoreNonCriticalErrors_(false) {

    ui = new Ui::FileOperationDialog();
    ui->setupUi(this);

    QString title;
    QString message;
    switch(_operation->type()) {
    case FM_FILE_OP_MOVE:
        title = tr("Move files");
        message = tr("Moving the following files to destination folder:");
        break;
    case FM_FILE_OP_COPY:
        title = tr("Copy Files");
        message = tr("Copying the following files to destination folder:");
        break;
    case FM_FILE_OP_TRASH:
        title = tr("Trash Files");
        message = tr("Moving the following files to trash can:");
        break;
    case FM_FILE_OP_DELETE:
        title = tr("Delete Files");
        message = tr("Deleting the following files:");
        ui->dest->hide();
        ui->destLabel->hide();
        break;
    case FM_FILE_OP_LINK:
        title = tr("Create Symlinks");
        message = tr("Creating symlinks for the following files:");
        break;
    case FM_FILE_OP_CHANGE_ATTR:
        title = tr("Change Attributes");
        message = tr("Changing attributes of the following files:");
        ui->dest->hide();
        ui->destLabel->hide();
        break;
    case FM_FILE_OP_UNTRASH:
        title = tr("Restore Trashed Files");
        message = tr("Restoring the following files from trash can:");
        ui->dest->hide();
        ui->destLabel->hide();
        break;
    }
    ui->message->setText(message);
    setWindowTitle(title);
}


FileOperationDialog::~FileOperationDialog() {
    delete ui;
}

void FileOperationDialog::setDestPath(const Fm::FilePath &dest) {
    ui->dest->setText(dest.displayName().get());
}

void FileOperationDialog::setSourceFiles(const Fm::FilePathList &srcFiles) {
    for(auto& srcFile : srcFiles) {
        ui->sourceFiles->addItem(srcFile.displayName().get());
    }
}

int FileOperationDialog::ask(QString /*question*/, char* const* /*options*/) {
    // TODO: implement FileOperationDialog::ask()
    return 0;
}


FileOperationJob::FileExistsAction FileOperationDialog::askRename(const FileInfo &src, const FileInfo &dest, FilePath &newDest) {
    FileOperationJob::FileExistsAction ret;
    if(defaultOption == -1) { // default action is not set, ask the user
        RenameDialog dlg(src, dest, this);
        dlg.exec();
        switch(dlg.action()) {
        case RenameDialog::ActionOverwrite:
            ret = FileOperationJob::OVERWRITE;
            if(dlg.applyToAll()) {
                defaultOption = ret;
            }
            break;
        case RenameDialog::ActionRename: {
            ret = FileOperationJob::RENAME;
            auto newName = dlg.newName();
            if(!newName.isEmpty()) {
                auto destDirPath = dest.path().parent();
                newDest = destDirPath.child(newName.toUtf8().constData());
            }
            break;
        }
        case RenameDialog::ActionIgnore:
            ret = FileOperationJob::SKIP;
            if(dlg.applyToAll()) {
                defaultOption = ret;
            }
            break;
        default:
            ret = FileOperationJob::CANCEL;
            break;
        }
    }
    else {
        ret = (FileOperationJob::FileExistsAction)defaultOption;
    }
    return ret;
}

FmJobErrorAction FileOperationDialog::error(GError* err, FmJobErrorSeverity severity) {
    if(severity >= FM_JOB_ERROR_MODERATE) {
        if(severity == FM_JOB_ERROR_CRITICAL) {
            QMessageBox::critical(this, tr("Error"), QString::fromUtf8(err->message));
            return FM_JOB_ABORT;
        }
        if (ignoreNonCriticalErrors_) {
            return FM_JOB_CONTINUE;
        }
        QMessageBox::StandardButton stb = QMessageBox::critical(this, tr("Error"), QString::fromUtf8(err->message),
                                                                QMessageBox::Ok | QMessageBox::Ignore);
        if (stb == QMessageBox::Ignore) {
            ignoreNonCriticalErrors_ = true;
        }
    }
    return FM_JOB_CONTINUE;
}

void FileOperationDialog::setCurFile(QString cur_file) {
    ui->curFile->setText(cur_file);
}

void FileOperationDialog::setDataTransferred(uint64_t finishedSize, std::uint64_t totalSize) {
    ui->dataTransferred->setText(QString("%1 / %2")
                                 .arg(formatFileSize(finishedSize, fm_config->si_unit))
                                 .arg(formatFileSize(totalSize, fm_config->si_unit)));
}

void FileOperationDialog::setPercent(unsigned int percent) {
    ui->progressBar->setValue(percent);
}

void FileOperationDialog::setRemainingTime(unsigned int sec) {
    unsigned int min = 0;
    unsigned int hr = 0;
    if(sec > 60) {
        min = sec / 60;
        sec %= 60;
        if(min > 60) {
            hr = min / 60;
            min %= 60;
        }
    }
    ui->timeRemaining->setText(QString("%1:%2:%3")
                               .arg(hr, 2, 10, QChar('0'))
                               .arg(min, 2, 10, QChar('0'))
                               .arg(sec, 2, 10, QChar('0')));
}

void FileOperationDialog::setPrepared() {
}

void FileOperationDialog::reject() {
    operation->cancel();
    QDialog::reject();
}


} // namespace Fm

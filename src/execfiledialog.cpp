/*
 * Copyright (C) 2014 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#include "execfiledialog_p.h"
#include "ui_exec-file.h"
#include "core/iconinfo.h"

namespace Fm {

ExecFileDialog::ExecFileDialog(FmFileInfo* file, QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    ui(new Ui::ExecFileDialog()),
    fileInfo_(fm_file_info_ref(file)),
    result_(FM_FILE_LAUNCHER_EXEC_CANCEL) {

    ui->setupUi(this);
    // show file icon
    GIcon* gicon = G_ICON(fm_file_info_get_icon(fileInfo_));
    ui->icon->setPixmap(Fm::IconInfo::fromGIcon(gicon)->qicon().pixmap(QSize(48, 48)));

    QString msg;
    if(fm_file_info_is_desktop_entry(file)) {
        msg = tr("This file '%1' seems to be a desktop entry.\nWhat do you want to do with it?")
              .arg(QString::fromUtf8(fm_file_info_get_disp_name(file)));
        ui->exec->setDefault(true);
        ui->execTerm->hide();
    }
    else if(fm_file_info_is_text(file)) {
        msg = tr("This text file '%1' seems to be an executable script.\nWhat do you want to do with it?")
              .arg(QString::fromUtf8(fm_file_info_get_disp_name(file)));
        ui->execTerm->setDefault(true);
    }
    else {
        msg = tr("This file '%1' is executable. Do you want to execute it?")
              .arg(QString::fromUtf8(fm_file_info_get_disp_name(file)));
        ui->exec->setDefault(true);
        ui->open->hide();
    }
    ui->msg->setText(msg);
}

ExecFileDialog::~ExecFileDialog() {
    delete ui;
    if(fileInfo_) {
        fm_file_info_unref(fileInfo_);
    }
}

void ExecFileDialog::accept() {
    QObject* _sender = sender();
    if(_sender == ui->exec) {
        result_ = FM_FILE_LAUNCHER_EXEC;
    }
    else if(_sender == ui->execTerm) {
        result_ = FM_FILE_LAUNCHER_EXEC_IN_TERMINAL;
    }
    else if(_sender == ui->open) {
        result_ = FM_FILE_LAUNCHER_EXEC_OPEN;
    }
    QDialog::accept();
}

} // namespace Fm

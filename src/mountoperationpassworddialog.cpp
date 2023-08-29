/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#include "mountoperationpassworddialog_p.h"
#include "ui_mount-operation-password.h"
#include "mountoperation.h"

#include <QSettings>

namespace Fm {

MountOperationPasswordDialog::MountOperationPasswordDialog(MountOperation* op, GAskPasswordFlags flags):
    QDialog(),
    mountOperation(op),
    needPassword(flags & G_ASK_PASSWORD_NEED_PASSWORD ? true : false),
    needUserName(flags & G_ASK_PASSWORD_NEED_USERNAME ? true : false),
    needDomain(flags & G_ASK_PASSWORD_NEED_DOMAIN ? true : false),
    canSavePassword(flags & G_ASK_PASSWORD_SAVING_SUPPORTED ? true : false),
    canAnonymous(flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED ? true : false) {

    ui = new Ui::MountOperationPasswordDialog();
    ui->setupUi(this);

    // change the text of Ok button to Connect
    ui->buttonBox->buttons().constFirst()->setText(tr("&Connect"));
    connect(ui->Anonymous, &QAbstractButton::toggled, this, &MountOperationPasswordDialog::onAnonymousToggled);

    QSettings settings(QSettings::UserScope, QStringLiteral("lxqt"), QStringLiteral("mountdialog"));

    if(canAnonymous) {
        // select ananymous by default if applicable.
        if(settings.value(QStringLiteral("Anonymous"), true).toBool()) {
            ui->Anonymous->setChecked(true);
        }
        else {
            ui->asUser->setChecked(true);
        }
        connect(ui->usernameGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled),
                this, [this](QAbstractButton *btn, bool checked){
            if(checked) {
                QSettings settings(QSettings::UserScope, QStringLiteral("lxqt"), QStringLiteral("mountdialog"));
                settings.setValue(QStringLiteral("Anonymous"), btn == ui->Anonymous);
            }
        });
    }
    else {
        ui->Anonymous->setEnabled(false);
        ui->asUser->setChecked(true);
    }
    if(!needUserName) {
        ui->username->setEnabled(false);
    }
    if(needPassword) {
        if(!needUserName) {
            ui->password->setFocus();
        }
    }
    else {
        ui->password->setEnabled(false);
    }
    if(!needDomain) {
        ui->domain->hide();
        ui->domainLabel->hide();
    }
    if(canSavePassword) {
        switch(settings.value(QStringLiteral("RememberPassword"), 0).toInt()) {
        case -1:
            ui->forgetPassword->setChecked(true);
            break;
        case 1:
            ui->storePassword->setChecked(true);
            break;
        default:
            ui->sessionPassword->setChecked(true);
            break;
        }
        connect(ui->passwordGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled),
                this, [this](QAbstractButton *btn, bool checked){
            if(checked) {
                int remember = (btn == ui->forgetPassword ? -1 : btn == ui->storePassword ? 1 : 0);
                QSettings settings(QSettings::UserScope, QStringLiteral("lxqt"), QStringLiteral("mountdialog"));
                settings.setValue(QStringLiteral("RememberPassword"), remember);
            }
        });
    }
    else {
        ui->storePassword->setEnabled(false);
        ui->sessionPassword->setEnabled(false);
        ui->forgetPassword->setChecked(true);
    }
}

MountOperationPasswordDialog::~MountOperationPasswordDialog() {
    delete ui;
}

void MountOperationPasswordDialog::onAnonymousToggled(bool checked) {
    // disable username/password entries if anonymous mode is used
    bool useUserPassword = !checked;
    if(needUserName) {
        ui->username->setEnabled(useUserPassword);
    }
    if(needPassword) {
        ui->password->setEnabled(useUserPassword);
    }
    if(needDomain) {
        ui->domain->setEnabled(useUserPassword);
    }

    if(canSavePassword) {
        ui->forgetPassword->setEnabled(useUserPassword);
        ui->sessionPassword->setEnabled(useUserPassword);
        ui->storePassword->setEnabled(useUserPassword);
    }
}

void MountOperationPasswordDialog::setMessage(QString message) {
    ui->message->setText(message);
}

void MountOperationPasswordDialog::setDefaultDomain(QString domain) {
    ui->domain->setText(domain);
}

void MountOperationPasswordDialog::setDefaultUser(QString user) {
    ui->username->setText(user);
}

void MountOperationPasswordDialog::done(int r) {
    GMountOperation* gmop = mountOperation->mountOperation();

    if(r == QDialog::Accepted) {

        if(needUserName) {
            g_mount_operation_set_username(gmop, ui->username->text().toUtf8().constData());
        }
        if(needDomain) {
            g_mount_operation_set_domain(gmop, ui->domain->text().toUtf8().constData());
        }
        if(needPassword) {
            g_mount_operation_set_password(gmop, ui->password->text().toUtf8().constData());
            if(!ui->Anonymous->isChecked()) {
                g_mount_operation_set_password_save(gmop, ui->storePassword->isChecked() ? G_PASSWORD_SAVE_PERMANENTLY :
                                                          ui-> sessionPassword->isChecked() ? G_PASSWORD_SAVE_FOR_SESSION :
                                                          G_PASSWORD_SAVE_NEVER);
            }
        }
        if(canAnonymous) {
            g_mount_operation_set_anonymous(gmop, ui->Anonymous->isChecked());
        }

        g_mount_operation_reply(gmop, G_MOUNT_OPERATION_HANDLED);
    }
    else {
        g_mount_operation_reply(gmop, G_MOUNT_OPERATION_ABORTED);
    }
    QDialog::done(r);
}

} // namespace Fm

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


#include "mountoperation.h"
#include <glib/gi18n.h> // for _()
#include <glib/gstdio.h> // for g_chdir()
#include <QMessageBox>
#include <QPushButton>
#include <QEventLoop>
#include "mountoperationquestiondialog_p.h"
#include "ui_mount-operation-password.h"
#include "core/gioptrs.h"

namespace Fm {

MountOperation::MountOperation(bool interactive, QWidget* parent):
    QObject(parent),
    op(g_mount_operation_new()),
    cancellable_(g_cancellable_new()),
    running(false),
    interactive_(interactive),
    eventLoop(nullptr),
    autoDestroy_(true) {

    tmpOp_ = nullptr;
    volume_ = nullptr;
    dlg_ = nullptr;

    g_signal_connect(op, "ask-password", G_CALLBACK(onAskPassword), this);
    g_signal_connect(op, "ask-question", G_CALLBACK(onAskQuestion), this);
    // g_signal_connect(op, "reply", G_CALLBACK(onReply), this);

#if GLIB_CHECK_VERSION(2, 20, 0)
    g_signal_connect(op, "aborted", G_CALLBACK(onAbort), this);
#endif
#if GLIB_CHECK_VERSION(2, 22, 0)
    g_signal_connect(op, "show-processes", G_CALLBACK(onShowProcesses), this);
#endif
#if GLIB_CHECK_VERSION(2, 34, 0)
    g_signal_connect(op, "show-unmount-progress", G_CALLBACK(onShowUnmountProgress), this);
#endif

}

MountOperation::~MountOperation() {
    qDebug("delete MountOperation");
    if(cancellable_) {
        cancel();
        g_object_unref(cancellable_);
    }

    if(eventLoop) { // if wait() is called to block the main loop, but the event loop is still running
        // NOTE: is this possible?
        eventLoop->exit(1);
    }

    if(op) {
        g_signal_handlers_disconnect_by_func(op, (gpointer)G_CALLBACK(onAskPassword), this);
        g_signal_handlers_disconnect_by_func(op, (gpointer)G_CALLBACK(onAskQuestion), this);
#if GLIB_CHECK_VERSION(2, 20, 0)
        g_signal_handlers_disconnect_by_func(op, (gpointer)G_CALLBACK(onAbort), this);
#endif
#if GLIB_CHECK_VERSION(2, 22, 0)
        g_signal_handlers_disconnect_by_func(op, (gpointer)G_CALLBACK(onShowProcesses), this);
#endif
#if GLIB_CHECK_VERSION(2, 34, 0)
        g_signal_handlers_disconnect_by_func(op, (gpointer)G_CALLBACK(onShowUnmountProgress), this);
#endif
        g_object_unref(op);
    }

    if(volume_) {
        g_object_unref(volume_);
    }
    if(tmpOp_) {
        g_object_unref(tmpOp_);
    }
    if(dlg_) { // impossible
        delete dlg_;
    }
    // qDebug("MountOperation deleted");
}

void MountOperation::mountEnclosingVolume(const FilePath &path) {
    g_file_mount_enclosing_volume(path.gfile().get(), G_MOUNT_MOUNT_NONE, op, cancellable_,
                                  (GAsyncReadyCallback)onMountFileFinished, new QPointer<MountOperation>(this));
}

void MountOperation::mountMountable(const FilePath &mountable) {
    g_file_mount_mountable(mountable.gfile().get(), G_MOUNT_MOUNT_NONE, op, cancellable_,
                           (GAsyncReadyCallback)onMountMountableFinished, new QPointer<MountOperation>(this));
}

void MountOperation::onAbort(GMountOperation* /*_op*/, MountOperation* /*pThis*/) {

}

void MountOperation::onAskPassword(GMountOperation* /*_op*/, gchar* message, gchar* default_user, gchar* default_domain, GAskPasswordFlags flags, MountOperation* pThis) {
    //qDebug("ask password");
    if(!pThis->volume_) {
        // The mount is NOT done by g_volume_mount();
        // it is safe to show the password dialog (see below).
        MountOperationPasswordDialog dlg(pThis, flags);
        dlg.setMessage(QString::fromUtf8(message));
        dlg.setDefaultUser(QString::fromUtf8(default_user));
        dlg.setDefaultDomain(QString::fromUtf8(default_domain));
        dlg.exec();
        return;
    }

    /*
      NOTE: With g_volume_mount(), having a local event loop here (by showing a dialog) will
      result in a total freeze if a volume is added/removed while the loop is running. This
      may be a bug in GVFS or in how Qt handles GLib event loops (I, @tsujan, am not sure).

      As a workaround, only the needed info is gathered here and the password dialog is
      shown in handleFinish(). After the dialog is accepted, the volume is mounted again and
      the info provided by the dialog is used to set the password and other mount properties
      when this function is called again.
    */
    if(!pThis->tmpOp_) { // gather the needed info
        pThis->tmpOp_ = g_mount_operation_new(); // used to save mount info later
        pThis->dlg_ = new MountOperationPasswordDialog(pThis, flags);
        pThis->dlg_->setAttribute (Qt::WA_DeleteOnClose);
        pThis->dlg_->setMessage(QString::fromUtf8(message));
        pThis->dlg_->setDefaultUser(QString::fromUtf8(default_user));
        pThis->dlg_->setDefaultDomain(QString::fromUtf8(default_domain));
    }
    else { // set the password and other mount properties
        const char* userName = g_mount_operation_get_username(pThis->tmpOp_);
        const char* domain = g_mount_operation_get_domain(pThis->tmpOp_);
        const char* password = g_mount_operation_get_password(pThis->tmpOp_);

        if(userName) {
            g_mount_operation_set_username(pThis->op, userName);
        }
        if(domain) {
            g_mount_operation_set_password(pThis->op, domain);
        }
        if(password) {
            g_mount_operation_set_password(pThis->op, password);
            g_mount_operation_set_password_save(pThis->op,
                                                g_mount_operation_get_password_save(pThis->tmpOp_));
        }
        g_mount_operation_set_anonymous(pThis->op,
                                        g_mount_operation_get_anonymous(pThis->tmpOp_));

        g_object_unref(pThis->tmpOp_);
        pThis->tmpOp_ = nullptr;

        g_mount_operation_reply(pThis->op, G_MOUNT_OPERATION_HANDLED);
    }
}

void MountOperation::onAskQuestion(GMountOperation* /*_op*/, gchar* message, GStrv choices, MountOperation* pThis) {
    qDebug("ask question");
    MountOperationQuestionDialog dialog(pThis, message, choices);
    dialog.exec();
}

/*
void MountOperation::onReply(GMountOperation* _op, GMountOperationResult result, MountOperation* pThis) {
  qDebug("reply");
}
*/

void MountOperation::onShowProcesses(GMountOperation* /*_op*/, gchar* /*message*/, GArray* /*processes*/, GStrv /*choices*/, MountOperation* /*pThis*/) {
    qDebug("show processes");
}

void MountOperation::onShowUnmountProgress(GMountOperation* /*_op*/, gchar* /*message*/, gint64 /*time_left*/, gint64 /*bytes_left*/, MountOperation* /*pThis*/) {
    qDebug("show unmount progress");
}

void MountOperation::onEjectMountFinished(GMount* mount, GAsyncResult* res, QPointer< MountOperation >* pThis) {
    if(*pThis) {
        GError* error = nullptr;
        g_mount_eject_with_operation_finish(mount, res, &error);
        (*pThis)->handleFinish(error);
    }
    delete pThis;
}

void MountOperation::onEjectVolumeFinished(GVolume* volume, GAsyncResult* res, QPointer< MountOperation >* pThis) {
    if(*pThis) {
        GError* error = nullptr;
        g_volume_eject_with_operation_finish(volume, res, &error);
        (*pThis)->handleFinish(error);
    }
    delete pThis;
}

void MountOperation::onStopDriveFinished(GDrive* drive, GAsyncResult* res, QPointer<MountOperation>* pThis) {
    if(*pThis) {
        GError* error = nullptr;
        g_drive_stop_finish(drive, res, &error);
        (*pThis)->handleFinish(error);
    }
    delete pThis;
}

void MountOperation::onEjectDriveFinished(GDrive* drive, GAsyncResult* res, QPointer<MountOperation>* pThis) {
    if(*pThis) {
        GError* error = nullptr;
        g_drive_eject_with_operation_finish(drive, res, &error);
        (*pThis)->handleFinish(error);
    }
    delete pThis;
}

void MountOperation::onEjectFileFinished(GFile* file, GAsyncResult* res, QPointer<MountOperation>* pThis) {
    if(*pThis) {
        GError* error = nullptr;
        g_file_eject_mountable_with_operation_finish(file, res, &error);
        (*pThis)->handleFinish(error);
    }
    delete pThis;
}

void MountOperation::onMountFileFinished(GFile* file, GAsyncResult* res, QPointer< MountOperation >* pThis) {
    if(*pThis) {
        GError* error = nullptr;
        g_file_mount_enclosing_volume_finish(file, res, &error);
        (*pThis)->handleFinish(error);
    }
    delete pThis;
}

void MountOperation::onMountMountableFinished(GFile* file, GAsyncResult* res, QPointer<MountOperation>* pThis) {
    if(*pThis) {
        GError* error = nullptr;
        g_file_mount_mountable_finish(file, res, &error);
        (*pThis)->handleFinish(error);
    }
    delete pThis;
}

void MountOperation::onMountVolumeFinished(GVolume* volume, GAsyncResult* res, QPointer< MountOperation >* pThis) {
    if(*pThis) {
        GError* error = nullptr;
        g_volume_mount_finish(volume, res, &error);
        (*pThis)->handleFinish(error);
    }
    delete pThis;
}

void MountOperation::onUnmountMountFinished(GMount* mount, GAsyncResult* res, QPointer< MountOperation >* pThis) {
    if(*pThis) {
        GError* error = nullptr;
        g_mount_unmount_with_operation_finish(mount, res, &error);
        (*pThis)->handleFinish(error);
    }
    delete pThis;
}

void MountOperation::onUnmountFileFinished(GFile* file, GAsyncResult* res, QPointer<MountOperation>* pThis) {
    if(*pThis) {
        GError* error = nullptr;
        g_file_unmount_mountable_with_operation_finish(file, res, &error);
        (*pThis)->handleFinish(error);
    }
    delete pThis;
}

void MountOperation::handleFinish(GError* error) {
    if(volume_ && tmpOp_ && dlg_) { // see onAskPassword()
        if(error) {
            g_error_free(error);
            error = nullptr;
        }
        if(dlg_->exec()) {
            mount(volume_);
            return;
        }
    }

    qDebug("operation finished: %p", static_cast<void *>(error));
    if(error) {
        bool showError = interactive_;
        if(error->domain == G_IO_ERROR) {
            if(error->code == G_IO_ERROR_FAILED) {
                // Generate a more human-readable error message instead of using a gvfs one.
                // The original error message is something like:
                // Error unmounting: umount exited with exit code 1:
                // helper failed with: umount: only root can unmount
                // UUID=18cbf00c-e65f-445a-bccc-11964bdea05d from /media/sda4 */
                // Why they pass this back to us? This is not human-readable for the users at all.
                if(strstr(error->message, "only root can ")) {
                    g_free(error->message);
                    error->message = g_strdup(_("Only system administrators have the permission to do this."));
                }
            }
            else if(error->code == G_IO_ERROR_FAILED_HANDLED) {
                showError = false;
            }
        }
        if(showError) {
            QMessageBox::critical(nullptr, QObject::tr("Error"), QString::fromUtf8(error->message));
        }
    }

    Q_EMIT finished(error);

    if(eventLoop) { // if wait() is called to block the main loop
        eventLoop->exit(error != nullptr ? 1 : 0);
        eventLoop = nullptr;
    }

    if(error) {
        g_error_free(error);
    }

    // free ourself here!!
    if(autoDestroy_) {
        deleteLater();
    }
}

void MountOperation::prepareUnmount(GMount* mount) {
    /* ensure that CWD is not on the mounted filesystem. */
    char* cwd_str = g_get_current_dir();
    GFile* cwd = g_file_new_for_path(cwd_str);
    GFile* root = g_mount_get_root(mount);
    g_free(cwd_str);
    /* FIXME: This cannot cover 100% cases since symlinks are not checked.
      * There may be other cases that cwd is actually under mount root
      * but checking prefix is not enough. We already did our best, though. */
    if(g_file_has_prefix(cwd, root)) {
        g_chdir("/");
    }
    g_object_unref(cwd);
    g_object_unref(root);
}

// block the operation used an internal QEventLoop and returns
// only after the whole operation is finished.
bool MountOperation::wait() {
    QEventLoop loop;
    eventLoop = &loop;
    int exitCode = loop.exec();
    return exitCode == 0 ? true : false;
}

} // namespace Fm

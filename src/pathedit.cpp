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


#include "pathedit.h"
#include "pathedit_p.h"
#include <QCompleter>
#include <QStringListModel>
#include <QStringBuilder>
#include <QThread>
#include <QDebug>
#include <libfm/fm.h>

namespace Fm {

void PathEditJob::runJob() {
  GError *err = NULL;
  GFileEnumerator* enu = g_file_enumerate_children(dirName,
                                                   // G_FILE_ATTRIBUTE_STANDARD_NAME","
                                                   G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME","
                                                   G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                                   G_FILE_QUERY_INFO_NONE, cancellable,
                                                   &err);
  if(enu) {
    while(!g_cancellable_is_cancelled(cancellable)) {
      GFileInfo* inf = g_file_enumerator_next_file(enu, cancellable, &err);
      if(inf) {
        GFileType type = g_file_info_get_file_type(inf);
        if(type == G_FILE_TYPE_DIRECTORY) {
          const char* name = g_file_info_get_display_name(inf);
          // FIXME: encoding conversion here?
          subDirs.append(QString::fromUtf8(name));
        }
        g_object_unref(inf);
      }
      else {
        if(err) {
          g_error_free(err);
          err = NULL;
        }
        else /* EOF */
          break;
      }
    }
    g_file_enumerator_close(enu, cancellable, NULL);
    g_object_unref(enu);
  }
  // finished! let's update the UI in the main thread
  Q_EMIT finished();
}


PathEdit::PathEdit(QWidget* parent):
  QLineEdit(parent),
  cancellable_(NULL),
  model_(new QStringListModel()),
  completer_(new QCompleter()) {
  setCompleter(completer_);
  completer_->setModel(model_);
  connect(this, &PathEdit::textChanged, this, &PathEdit::onTextChanged);
}

PathEdit::~PathEdit() {
  delete completer_;
  if(model_)
    delete model_;
  if(cancellable_) {
    g_cancellable_cancel(cancellable_);
    g_object_unref(cancellable_);
  }
}

void PathEdit::focusInEvent(QFocusEvent* e) {
  QLineEdit::focusInEvent(e);
  // build the completion list only when we have the keyboard focus
  reloadCompleter(true);
}

void PathEdit::focusOutEvent(QFocusEvent* e) {
  QLineEdit::focusOutEvent(e);
  // free the completion list since we don't need it anymore
  freeCompleter();
}

void PathEdit::onTextChanged(const QString& text) {
  int pos = text.lastIndexOf('/');
  if(pos >= 0)
    ++pos;
  else
    pos = text.length();
  QString newPrefix = text.left(pos);
  if(currentPrefix_ != newPrefix) {
    currentPrefix_ = newPrefix;
    // only build the completion list if we have the keyboard focus
    // if we don't have the focus now, then we'll rebuild the completion list
    // when focusInEvent happens. this avoid unnecessary dir loading.
    if(hasFocus())
      reloadCompleter(false);
  }
}


void PathEdit::reloadCompleter(bool triggeredByFocusInEvent) {
  // parent dir has been changed, reload dir list
  // if(currentPrefix_[0] == "~") { // special case for home dir
  // cancel running dir-listing jobs, if there's any
  if(cancellable_) {
    g_cancellable_cancel(cancellable_);
    g_object_unref(cancellable_);
  }

  // create a new job to do dir listing
  PathEditJob* job = new PathEditJob();
  job->edit = this;
  job->triggeredByFocusInEvent = triggeredByFocusInEvent;
  // need to use fm_file_new_for_commandline_arg() rather than g_file_new_for_commandline_arg().
  // otherwise, our own vfs, such as menu://, won't be loaded.
  job->dirName = fm_file_new_for_commandline_arg(currentPrefix_.toLocal8Bit().constData());
  // qDebug("load: %s", g_file_get_uri(data->dirName));
  cancellable_ = g_cancellable_new();
  job->cancellable = (GCancellable*)g_object_ref(cancellable_);

  // launch a new worker thread to handle the job
  QThread* thread = new QThread();
  job->moveToThread(thread);
  connect(thread, &QThread::started, job, &PathEditJob::runJob);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);
  connect(thread, &QThread::finished, job, &QObject::deleteLater);
  connect(job, &PathEditJob::finished, this, &PathEdit::onJobFinished);
  thread->start(QThread::LowPriority);
}

void PathEdit::freeCompleter() {
  if(cancellable_) {
    g_cancellable_cancel(cancellable_);
    g_object_unref(cancellable_);
    cancellable_ = NULL;
  }
  model_->setStringList(QStringList());
}

// This slot is called from main thread so it's safe to access the GUI
void PathEdit::onJobFinished() {
  PathEditJob* data = static_cast<PathEditJob*>(sender());
  if(!g_cancellable_is_cancelled(data->cancellable)) {
    // update the completer only if the job is not cancelled
    QStringList::iterator it;
    for(it = data->subDirs.begin(); it != data->subDirs.end(); ++it) {
      // qDebug("%s", it->toUtf8().constData());
      *it = (currentPrefix_ % *it);
    }
    model_->setStringList(data->subDirs);
    // trigger completion manually
    if(hasFocus() && !data->triggeredByFocusInEvent)
      completer_->complete();
  }
  else
    model_->setStringList(QStringList());
  if(cancellable_) {
    g_object_unref(cancellable_);
    cancellable_ = NULL;
  }
}

} // namespace Fm

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

#ifndef __LIBFM_QT_FM_FILE_OPS_JOB_H__
#define __LIBFM_QT_FM_FILE_OPS_JOB_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"
#include "job.h"

namespace Fm {


class LIBFM_QT_API FileOpsJob: public Job {
public:


  FileOpsJob(FmFileOpType type, FmPathList* files) {
    dataPtr_ = reinterpret_cast<GObject*>(fm_file_ops_job_new(type, files));
  }


  // default constructor
  FileOpsJob() {
    dataPtr_ = nullptr;
  }


  FileOpsJob(FmFileOpsJob* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  FileOpsJob(const FileOpsJob& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  FileOpsJob(FileOpsJob&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }



  // create a wrapper for the data pointer without increasing the reference count
  static FileOpsJob wrapPtr(FmFileOpsJob* dataPtr) {
    FileOpsJob obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmFileOpsJob* takeDataPtr() {
    FmFileOpsJob* data = reinterpret_cast<FmFileOpsJob*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmFileOpsJob* dataPtr() {
    return reinterpret_cast<FmFileOpsJob*>(dataPtr_);
  }

  // automatic type casting
  operator FmFileOpsJob*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  FileOpsJob& operator=(const FileOpsJob& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  FileOpsJob& operator=(FileOpsJob&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  FmFileOpOption getOptions(void) {
    return fm_file_ops_job_get_options(dataPtr());
  }


  FmFileOpOption askRename(GFile* src, GFileInfo* src_inf, GFile* dest, GFile** new_dest) {
    return fm_file_ops_job_ask_rename(dataPtr(), src, src_inf, dest, new_dest);
  }


  void emitPercent(void) {
    fm_file_ops_job_emit_percent(dataPtr());
  }


  void emitCurFile(const char* cur_file) {
    fm_file_ops_job_emit_cur_file(dataPtr(), cur_file);
  }


  void emitPrepared(void) {
    fm_file_ops_job_emit_prepared(dataPtr());
  }


  void setTarget(const char* url) {
    fm_file_ops_job_set_target(dataPtr(), url);
  }


  void setHidden(gboolean hidden) {
    fm_file_ops_job_set_hidden(dataPtr(), hidden);
  }


  void setIcon(GIcon* icon) {
    fm_file_ops_job_set_icon(dataPtr(), icon);
  }


  void setDisplayName(const char* name) {
    fm_file_ops_job_set_display_name(dataPtr(), name);
  }


  void setChown(gint uid, gint gid) {
    fm_file_ops_job_set_chown(dataPtr(), uid, gid);
  }


  void setChmod(mode_t new_mode, mode_t new_mode_mask) {
    fm_file_ops_job_set_chmod(dataPtr(), new_mode, new_mode_mask);
  }


  void setRecursive(gboolean recursive) {
    fm_file_ops_job_set_recursive(dataPtr(), recursive);
  }


  FmPath* getDest(void) {
    return fm_file_ops_job_get_dest(dataPtr());
  }


  void setDest(FmPath* dest) {
    fm_file_ops_job_set_dest(dataPtr(), dest);
  }



};


}

#endif // __LIBFM_QT_FM_FILE_OPS_JOB_H__

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

#ifndef __LIBFM_QT_FM_DIR_LIST_JOB_H__
#define __LIBFM_QT_FM_DIR_LIST_JOB_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"
#include "job.h"

namespace Fm {


class LIBFM_QT_API DirListJob: public Job {
public:


  DirListJob(FmPath* path, gboolean dir_only) {
    dataPtr_ = reinterpret_cast<GObject*>(fm_dir_list_job_new(path, dir_only));
  }


  // default constructor
  DirListJob() {
    dataPtr_ = nullptr;
  }


  DirListJob(FmDirListJob* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  DirListJob(const DirListJob& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  DirListJob(DirListJob&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }



  // create a wrapper for the data pointer without increasing the reference count
  static DirListJob wrapPtr(FmDirListJob* dataPtr) {
    DirListJob obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmDirListJob* takeDataPtr() {
    FmDirListJob* data = reinterpret_cast<FmDirListJob*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmDirListJob* dataPtr() {
    return reinterpret_cast<FmDirListJob*>(dataPtr_);
  }

  // automatic type casting
  operator FmDirListJob*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  DirListJob& operator=(const DirListJob& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  DirListJob& operator=(DirListJob&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  void addFoundFile(FmFileInfo* file) {
    fm_dir_list_job_add_found_file(dataPtr(), file);
  }


  void setIncremental(gboolean set) {
    fm_dir_list_job_set_incremental(dataPtr(), set);
  }


  FmFileInfoList* getFiles(void) {
    return fm_dir_list_job_get_files(dataPtr());
  }


  static DirListJob newForGfile(GFile* gf) {
    return DirListJob::wrapPtr(fm_dir_list_job_new_for_gfile(gf));
  }


  static DirListJob new2(FmPath* path, FmDirListJobFlags flags) {
    return DirListJob::wrapPtr(fm_dir_list_job_new2(path, flags));
  }



};


}

#endif // __LIBFM_QT_FM_DIR_LIST_JOB_H__

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

#ifndef __LIBFM_QT_FM_FILE_INFO_JOB_H__
#define __LIBFM_QT_FM_FILE_INFO_JOB_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"
#include "job.h"

namespace Fm {


class LIBFM_QT_API FileInfoJob: public Job {
public:


  FileInfoJob(FmPathList* files_to_query, FmFileInfoJobFlags flags) {
    dataPtr_ = reinterpret_cast<GObject*>(fm_file_info_job_new(files_to_query, flags));
  }


  // default constructor
  FileInfoJob() {
    dataPtr_ = nullptr;
  }


  FileInfoJob(FmFileInfoJob* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  FileInfoJob(const FileInfoJob& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  FileInfoJob(FileInfoJob&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }



  // create a wrapper for the data pointer without increasing the reference count
  static FileInfoJob wrapPtr(FmFileInfoJob* dataPtr) {
    FileInfoJob obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmFileInfoJob* takeDataPtr() {
    FmFileInfoJob* data = reinterpret_cast<FmFileInfoJob*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmFileInfoJob* dataPtr() {
    return reinterpret_cast<FmFileInfoJob*>(dataPtr_);
  }

  // automatic type casting
  operator FmFileInfoJob*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  FileInfoJob& operator=(const FileInfoJob& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  FileInfoJob& operator=(FileInfoJob&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  FmPath* getCurrent(void) {
    return fm_file_info_job_get_current(dataPtr());
  }


  void addGfile(GFile* gf) {
    fm_file_info_job_add_gfile(dataPtr(), gf);
  }


  void add(FmPath* path) {
    fm_file_info_job_add(dataPtr(), path);
  }



};


}

#endif // __LIBFM_QT_FM_FILE_INFO_JOB_H__

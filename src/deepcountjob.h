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

#ifndef __LIBFM_QT_FM_DEEP_COUNT_JOB_H__
#define __LIBFM_QT_FM_DEEP_COUNT_JOB_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"
#include "job.h"

namespace Fm {


class LIBFM_QT_API DeepCountJob: public Job {
public:


  DeepCountJob(FmPathList* paths, FmDeepCountJobFlags flags) {
    dataPtr_ = reinterpret_cast<GObject*>(fm_deep_count_job_new(paths, flags));
  }


  // default constructor
  DeepCountJob() {
    dataPtr_ = nullptr;
  }


  DeepCountJob(FmDeepCountJob* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  DeepCountJob(const DeepCountJob& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  DeepCountJob(DeepCountJob&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }



  // create a wrapper for the data pointer without increasing the reference count
  static DeepCountJob wrapPtr(FmDeepCountJob* dataPtr) {
    DeepCountJob obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmDeepCountJob* takeDataPtr() {
    FmDeepCountJob* data = reinterpret_cast<FmDeepCountJob*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmDeepCountJob* dataPtr() {
    return reinterpret_cast<FmDeepCountJob*>(dataPtr_);
  }

  // automatic type casting
  operator FmDeepCountJob*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  DeepCountJob& operator=(const DeepCountJob& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  DeepCountJob& operator=(DeepCountJob&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  void setDest(dev_t dev, const char* fs_id) {
    fm_deep_count_job_set_dest(dataPtr(), dev, fs_id);
  }



};


}

#endif // __LIBFM_QT_FM_DEEP_COUNT_JOB_H__

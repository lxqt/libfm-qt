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

#ifndef __LIBFM_QT_FM_JOB_H__
#define __LIBFM_QT_FM_JOB_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API Job {
public:


  // default constructor
  Job() {
    dataPtr_ = nullptr;
  }


  Job(FmJob* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  Job(const Job& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  Job(Job&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }


  // destructor
  virtual ~Job() {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static Job wrapPtr(FmJob* dataPtr) {
    Job obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmJob* takeDataPtr() {
    FmJob* data = reinterpret_cast<FmJob*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmJob* dataPtr() {
    return reinterpret_cast<FmJob*>(dataPtr_);
  }

  // automatic type casting
  operator FmJob*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  Job& operator=(const Job& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  Job& operator=(Job&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  void resume(void) {
    fm_job_resume(dataPtr());
  }


  bool pause(void) {
    return fm_job_pause(dataPtr());
  }


  int askValist(const char* question, va_list options) {
    return fm_job_ask_valist(dataPtr(), question, options);
  }


  int askv(const char* question, gchar* const* options) {
    return fm_job_askv(dataPtr(), question, options);
  }


  int ask(const char* question, ... ) {
    
    int ret;
    va_list args;
    va_start (args, question);
    ret = fm_job_ask_valist(dataPtr(), question, args);
    va_end (args);
    return ret;

  }


  FmJobErrorAction emitError(GError* err, FmJobErrorSeverity severity) {
    return fm_job_emit_error(dataPtr(), err, severity);
  }


  void finish(void) {
    fm_job_finish(dataPtr());
  }


  void setCancellable(GCancellable* cancellable) {
    fm_job_set_cancellable(dataPtr(), cancellable);
  }


  GCancellable* getCancellable(void) {
    return fm_job_get_cancellable(dataPtr());
  }


  void initCancellable(void) {
    fm_job_init_cancellable(dataPtr());
  }


  gpointer callMainThread(FmJobCallMainThreadFunc func, gpointer user_data) {
    return fm_job_call_main_thread(dataPtr(), func, user_data);
  }


  void cancel(void) {
    fm_job_cancel(dataPtr());
  }


  bool runSyncWithMainloop(void) {
    return fm_job_run_sync_with_mainloop(dataPtr());
  }


  bool runSync(void) {
    return fm_job_run_sync(dataPtr());
  }


  bool runAsync(void) {
    return fm_job_run_async(dataPtr());
  }


  bool isRunning(void) {
    return fm_job_is_running(dataPtr());
  }


  bool isCancelled(void) {
    return fm_job_is_cancelled(dataPtr());
  }


  // automatic type casting for GObject
  operator GObject*() {
    return reinterpret_cast<GObject*>(dataPtr_);
  }


protected:
  GObject* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_JOB_H__

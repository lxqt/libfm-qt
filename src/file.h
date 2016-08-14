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

#ifndef __LIBFM_QT_FM_FILE_H__
#define __LIBFM_QT_FM_FILE_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API File {
public:


  // default constructor
  File() {
    dataPtr_ = nullptr;
  }


  File(FmFile* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  File(const File& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  File(File&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }


  // destructor
  virtual ~File() {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static File wrapPtr(FmFile* dataPtr) {
    File obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmFile* takeDataPtr() {
    FmFile* data = reinterpret_cast<FmFile*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmFile* dataPtr() {
    return reinterpret_cast<FmFile*>(dataPtr_);
  }

  // automatic type casting
  operator FmFile*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  File& operator=(const File& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  File& operator=(File&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  static GFile* newForCommandlineArg(const char* arg) {
    return fm_file_new_for_commandline_arg(arg);
  }


  static GFile* newForUri(const char* uri) {
    return fm_file_new_for_uri(uri);
  }


  static bool wantsIncremental(GFile* file) {
    return fm_file_wants_incremental(file);
  }


  static void addVfs(const char* name, FmFileInitTable* init) {
    fm_file_add_vfs(name, init);
  }


  // automatic type casting for GObject
  operator GObject*() {
    return reinterpret_cast<GObject*>(dataPtr_);
  }


protected:
  GObject* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_FILE_H__

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

#ifndef __LIBFM_QT_FM_THUMBNAILER_H__
#define __LIBFM_QT_FM_THUMBNAILER_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API Thumbnailer {
public:


  // default constructor
  Thumbnailer() {
    dataPtr_ = nullptr;
  }


  Thumbnailer(FmThumbnailer* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<FmThumbnailer*>(fm_thumbnailer_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  Thumbnailer(const Thumbnailer& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmThumbnailer*>(fm_thumbnailer_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  Thumbnailer(Thumbnailer&& other) {
    dataPtr_ = reinterpret_cast<FmThumbnailer*>(other.takeDataPtr());
  }


  // destructor
  ~Thumbnailer() {
    if(dataPtr_ != nullptr) {
      fm_thumbnailer_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static Thumbnailer wrapPtr(FmThumbnailer* dataPtr) {
    Thumbnailer obj;
    obj.dataPtr_ = reinterpret_cast<FmThumbnailer*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmThumbnailer* takeDataPtr() {
    FmThumbnailer* data = reinterpret_cast<FmThumbnailer*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmThumbnailer* dataPtr() {
    return reinterpret_cast<FmThumbnailer*>(dataPtr_);
  }

  // automatic type casting
  operator FmThumbnailer*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  Thumbnailer& operator=(const Thumbnailer& other) {
    if(dataPtr_ != nullptr) {
      fm_thumbnailer_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmThumbnailer*>(fm_thumbnailer_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  Thumbnailer& operator=(Thumbnailer&& other) {
    dataPtr_ = reinterpret_cast<FmThumbnailer*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  static void checkUpdate( ) {
    fm_thumbnailer_check_update();
  }


  void free(void) {
    fm_thumbnailer_free(dataPtr());
  }


  bool launchForUri(const char* uri, const char* output_file, guint size) {
    return fm_thumbnailer_launch_for_uri(dataPtr(), uri, output_file, size);
  }


  GPid launchForUriAsync(const char* uri, const char* output_file, guint size, GError** error) {
    return fm_thumbnailer_launch_for_uri_async(dataPtr(), uri, output_file, size, error);
  }


  char* commandForUri(const char* uri, const char* output_file, guint size) {
    return fm_thumbnailer_command_for_uri(dataPtr(), uri, output_file, size);
  }


  static Thumbnailer newFromKeyfile(const char* id, GKeyFile* kf) {
    return Thumbnailer::wrapPtr(fm_thumbnailer_new_from_keyfile(id, kf));
  }



private:
  FmThumbnailer* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_THUMBNAILER_H__

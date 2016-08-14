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

#ifndef __LIBFM_QT_FM_ARCHIVER_H__
#define __LIBFM_QT_FM_ARCHIVER_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API Archiver {
public:


  // default constructor
  Archiver() {
    dataPtr_ = nullptr;
  }


  // move constructor
  Archiver(Archiver&& other) {
    dataPtr_ = reinterpret_cast<FmArchiver*>(other.takeDataPtr());
  }


  // destructor
  ~Archiver() {
    if(dataPtr_ != nullptr) {
      (dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static Archiver wrapPtr(FmArchiver* dataPtr) {
    Archiver obj;
    obj.dataPtr_ = reinterpret_cast<FmArchiver*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmArchiver* takeDataPtr() {
    FmArchiver* data = reinterpret_cast<FmArchiver*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmArchiver* dataPtr() {
    return reinterpret_cast<FmArchiver*>(dataPtr_);
  }

  // automatic type casting
  operator FmArchiver*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }



  // move assignment
  Archiver& operator=(Archiver&& other) {
    dataPtr_ = reinterpret_cast<FmArchiver*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  void setDefault(void) {
    fm_archiver_set_default(dataPtr());
  }


  static Archiver getDefault( ) {
    return wrapPtr(fm_archiver_get_default());
  }


  bool extractArchivesTo(GAppLaunchContext* ctx, FmPathList* files, FmPath* dest_dir) {
    return fm_archiver_extract_archives_to(dataPtr(), ctx, files, dest_dir);
  }


  bool extractArchives(GAppLaunchContext* ctx, FmPathList* files) {
    return fm_archiver_extract_archives(dataPtr(), ctx, files);
  }


  bool createArchive(GAppLaunchContext* ctx, FmPathList* files) {
    return fm_archiver_create_archive(dataPtr(), ctx, files);
  }


  bool isMimeTypeSupported(const char* type) {
    return fm_archiver_is_mime_type_supported(dataPtr(), type);
  }


// the wrapped object cannot be copied.
private:
  Archiver(const Archiver& other) = delete;
  Archiver& operator=(const Archiver& other) = delete;


private:
  FmArchiver* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_ARCHIVER_H__

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

#ifndef __LIBFM_QT_FM_MIME_TYPE_H__
#define __LIBFM_QT_FM_MIME_TYPE_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API MimeType {
public:


  // default constructor
  MimeType() {
    dataPtr_ = nullptr;
  }


  MimeType(FmMimeType* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<FmMimeType*>(fm_mime_type_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  MimeType(const MimeType& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmMimeType*>(fm_mime_type_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  MimeType(MimeType&& other) {
    dataPtr_ = reinterpret_cast<FmMimeType*>(other.takeDataPtr());
  }


  // destructor
  ~MimeType() {
    if(dataPtr_ != nullptr) {
      fm_mime_type_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static MimeType wrapPtr(FmMimeType* dataPtr) {
    MimeType obj;
    obj.dataPtr_ = reinterpret_cast<FmMimeType*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmMimeType* takeDataPtr() {
    FmMimeType* data = reinterpret_cast<FmMimeType*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmMimeType* dataPtr() {
    return reinterpret_cast<FmMimeType*>(dataPtr_);
  }

  // automatic type casting
  operator FmMimeType*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  MimeType& operator=(const MimeType& other) {
    if(dataPtr_ != nullptr) {
      fm_mime_type_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmMimeType*>(fm_mime_type_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  MimeType& operator=(MimeType&& other) {
    dataPtr_ = reinterpret_cast<FmMimeType*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  void removeThumbnailer(gpointer thumbnailer) {
    fm_mime_type_remove_thumbnailer(dataPtr(), thumbnailer);
  }


  void addThumbnailer(gpointer thumbnailer) {
    fm_mime_type_add_thumbnailer(dataPtr(), thumbnailer);
  }


  GList* getThumbnailersList(void) {
    return fm_mime_type_get_thumbnailers_list(dataPtr());
  }


  FmIcon* getIcon(void) {
    return fm_mime_type_get_icon(dataPtr());
  }


  static MimeType fromName(const char* type) {
    return MimeType::wrapPtr(fm_mime_type_from_name(type));
  }


  static MimeType fromNativeFile(const char* file_path, const char* base_name, struct stat* pstat) {
    return MimeType::wrapPtr(fm_mime_type_from_native_file(file_path, base_name, pstat));
  }


  static MimeType fromFileName(const char* ufile_name) {
    return MimeType::wrapPtr(fm_mime_type_from_file_name(ufile_name));
  }



private:
  FmMimeType* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_MIME_TYPE_H__

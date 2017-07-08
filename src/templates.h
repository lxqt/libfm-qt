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

#ifndef __LIBFM_QT_FM_TEMPLATES_H__
#define __LIBFM_QT_FM_TEMPLATES_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API Template {
public:


  // default constructor
  Template() {
    dataPtr_ = nullptr;
  }


  Template(FmTemplate* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  Template(const Template& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  Template(Template&& other) noexcept {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }


  // destructor
  virtual ~Template() {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static Template wrapPtr(FmTemplate* dataPtr) {
    Template obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmTemplate* takeDataPtr() {
    FmTemplate* data = reinterpret_cast<FmTemplate*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmTemplate* dataPtr() {
    return reinterpret_cast<FmTemplate*>(dataPtr_);
  }

  // automatic type casting
  operator FmTemplate*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  Template& operator=(const Template& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  Template& operator=(Template&& other) noexcept {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  bool createFile(GFile* path, GError** error, gboolean run_default) {
    return fm_template_create_file(dataPtr(), path, error, run_default);
  }


  bool isDirectory(void) {
    return fm_template_is_directory(dataPtr());
  }


  FmIcon* getIcon(void) {
    return fm_template_get_icon(dataPtr());
  }


  FmMimeType* getMimeType(void) {
    return fm_template_get_mime_type(dataPtr());
  }


  static GList* listAll(gboolean user_only) {
    return fm_template_list_all(user_only);
  }


  // automatic type casting for GObject
  operator GObject*() {
    return reinterpret_cast<GObject*>(dataPtr_);
  }


protected:
  GObject* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_TEMPLATES_H__

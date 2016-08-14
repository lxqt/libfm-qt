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

#ifndef __LIBFM_QT_FM_TERMINAL_H__
#define __LIBFM_QT_FM_TERMINAL_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API Terminal {
public:


  // default constructor
  Terminal() {
    dataPtr_ = nullptr;
  }


  Terminal(FmTerminal* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  Terminal(const Terminal& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  Terminal(Terminal&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }


  // destructor
  virtual ~Terminal() {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static Terminal wrapPtr(FmTerminal* dataPtr) {
    Terminal obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmTerminal* takeDataPtr() {
    FmTerminal* data = reinterpret_cast<FmTerminal*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmTerminal* dataPtr() {
    return reinterpret_cast<FmTerminal*>(dataPtr_);
  }

  // automatic type casting
  operator FmTerminal*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  Terminal& operator=(const Terminal& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  Terminal& operator=(Terminal&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  static bool launch(const gchar* dir, GError** error) {
    return fm_terminal_launch(dir, error);
  }


  static Terminal dupDefault(GError** error) {
    return Terminal::wrapPtr(fm_terminal_dup_default(error));
  }


  // automatic type casting for GObject
  operator GObject*() {
    return reinterpret_cast<GObject*>(dataPtr_);
  }


protected:
  GObject* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_TERMINAL_H__

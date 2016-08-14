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

#ifndef __LIBFM_QT_FM_ICON_H__
#define __LIBFM_QT_FM_ICON_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API Icon {
public:


  // default constructor
  Icon() {
    dataPtr_ = nullptr;
  }


  Icon(FmIcon* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<FmIcon*>(fm_icon_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  Icon(const Icon& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmIcon*>(fm_icon_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  Icon(Icon&& other) {
    dataPtr_ = reinterpret_cast<FmIcon*>(other.takeDataPtr());
  }


  // destructor
  ~Icon() {
    if(dataPtr_ != nullptr) {
      fm_icon_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static Icon wrapPtr(FmIcon* dataPtr) {
    Icon obj;
    obj.dataPtr_ = reinterpret_cast<FmIcon*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmIcon* takeDataPtr() {
    FmIcon* data = reinterpret_cast<FmIcon*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmIcon* dataPtr() {
    return reinterpret_cast<FmIcon*>(dataPtr_);
  }

  // automatic type casting
  operator FmIcon*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  Icon& operator=(const Icon& other) {
    if(dataPtr_ != nullptr) {
      fm_icon_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmIcon*>(fm_icon_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  Icon& operator=(Icon&& other) {
    dataPtr_ = reinterpret_cast<FmIcon*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  static void unloadCache( ) {
    fm_icon_unload_cache();
  }


  static void resetUserDataCache(GQuark quark) {
    fm_icon_reset_user_data_cache(quark);
  }


  static void unloadUserDataCache( ) {
    fm_icon_unload_user_data_cache();
  }


  static void setUserDataDestroy(GDestroyNotify func) {
    fm_icon_set_user_data_destroy(func);
  }


  void setUserData(gpointer user_data) {
    fm_icon_set_user_data(dataPtr(), user_data);
  }


  gpointer getUserData(void) {
    return fm_icon_get_user_data(dataPtr());
  }


  static Icon fromName(const char* name) {
    return Icon::wrapPtr(fm_icon_from_name(name));
  }


  static Icon fromGicon(GIcon* gicon) {
    return Icon::wrapPtr(fm_icon_from_gicon(gicon));
  }



private:
  FmIcon* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_ICON_H__

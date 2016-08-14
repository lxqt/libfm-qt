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

#ifndef __LIBFM_QT_FM_LIST_H__
#define __LIBFM_QT_FM_LIST_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API List {
public:


  List(FmListFuncs* funcs) {
    dataPtr_ = reinterpret_cast<FmList*>(fm_list_new(funcs));
  }


  // default constructor
  List() {
    dataPtr_ = nullptr;
  }


  List(FmList* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<FmList*>(fm_list_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  List(const List& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmList*>(fm_list_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  List(List&& other) {
    dataPtr_ = reinterpret_cast<FmList*>(other.takeDataPtr());
  }


  // destructor
  ~List() {
    if(dataPtr_ != nullptr) {
      fm_list_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static List wrapPtr(FmList* dataPtr) {
    List obj;
    obj.dataPtr_ = reinterpret_cast<FmList*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmList* takeDataPtr() {
    FmList* data = reinterpret_cast<FmList*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmList* dataPtr() {
    return reinterpret_cast<FmList*>(dataPtr_);
  }

  // automatic type casting
  operator FmList*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  List& operator=(const List& other) {
    if(dataPtr_ != nullptr) {
      fm_list_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmList*>(fm_list_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  List& operator=(List&& other) {
    dataPtr_ = reinterpret_cast<FmList*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  void deleteLink(GList* l_) {
    fm_list_delete_link(dataPtr(), l_);
  }


  void removeAll(gpointer data) {
    fm_list_remove_all(dataPtr(), data);
  }


  void remove(gpointer data) {
    fm_list_remove(dataPtr(), data);
  }


  void clear(void) {
    fm_list_clear(dataPtr());
  }



private:
  FmList* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_LIST_H__

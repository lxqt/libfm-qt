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

#ifndef __LIBFM_QT_FM_BOOKMARKS_H__
#define __LIBFM_QT_FM_BOOKMARKS_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API Bookmarks {
public:


  // default constructor
  Bookmarks() {
    dataPtr_ = nullptr;
  }


  Bookmarks(FmBookmarks* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  Bookmarks(const Bookmarks& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  Bookmarks(Bookmarks&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }


  // destructor
  virtual ~Bookmarks() {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static Bookmarks wrapPtr(FmBookmarks* dataPtr) {
    Bookmarks obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmBookmarks* takeDataPtr() {
    FmBookmarks* data = reinterpret_cast<FmBookmarks*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmBookmarks* dataPtr() {
    return reinterpret_cast<FmBookmarks*>(dataPtr_);
  }

  // automatic type casting
  operator FmBookmarks*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  Bookmarks& operator=(const Bookmarks& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  Bookmarks& operator=(Bookmarks&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  GList* getAll(void) {
    return fm_bookmarks_get_all(dataPtr());
  }


  void rename(FmBookmarkItem* item, const char* new_name) {
    fm_bookmarks_rename(dataPtr(), item, new_name);
  }


  void reorder(FmBookmarkItem* item, int pos) {
    fm_bookmarks_reorder(dataPtr(), item, pos);
  }


  void remove(FmBookmarkItem* item) {
    fm_bookmarks_remove(dataPtr(), item);
  }


  FmBookmarkItem* insert(FmPath* path, const char* name, int pos) {
    return fm_bookmarks_insert(dataPtr(), path, name, pos);
  }


  static Bookmarks dup(void ) {
    return Bookmarks::wrapPtr(fm_bookmarks_dup());
  }


  // automatic type casting for GObject
  operator GObject*() {
    return reinterpret_cast<GObject*>(dataPtr_);
  }


protected:
  GObject* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_BOOKMARKS_H__

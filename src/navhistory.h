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

#ifndef __LIBFM_QT_FM_NAV_HISTORY_H__
#define __LIBFM_QT_FM_NAV_HISTORY_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API NavHistory {
public:


  NavHistory(void ) {
    dataPtr_ = reinterpret_cast<GObject*>(fm_nav_history_new());
  }


  NavHistory(FmNavHistory* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  NavHistory(const NavHistory& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  NavHistory(NavHistory&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }


  // destructor
  virtual ~NavHistory() {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static NavHistory wrapPtr(FmNavHistory* dataPtr) {
    NavHistory obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmNavHistory* takeDataPtr() {
    FmNavHistory* data = reinterpret_cast<FmNavHistory*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmNavHistory* dataPtr() {
    return reinterpret_cast<FmNavHistory*>(dataPtr_);
  }

  // automatic type casting
  operator FmNavHistory*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  NavHistory& operator=(const NavHistory& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  NavHistory& operator=(NavHistory&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  void setMax(guint num) {
    fm_nav_history_set_max(dataPtr(), num);
  }


  void clear(void) {
    fm_nav_history_clear(dataPtr());
  }


  void chdir(FmPath* path, gint old_scroll_pos) {
    fm_nav_history_chdir(dataPtr(), path, old_scroll_pos);
  }


  bool canBack(void) {
    return fm_nav_history_can_back(dataPtr());
  }


  int getScrollPos(void) {
    return fm_nav_history_get_scroll_pos(dataPtr());
  }


  FmPath* goTo(guint n, gint old_scroll_pos) {
    return fm_nav_history_go_to(dataPtr(), n, old_scroll_pos);
  }


  FmPath* getNthPath(guint n) {
    return fm_nav_history_get_nth_path(dataPtr(), n);
  }


  unsigned int getCurIndex(void) {
    return fm_nav_history_get_cur_index(dataPtr());
  }


  void jump(GList* l, int old_scroll_pos) {
    fm_nav_history_jump(dataPtr(), l, old_scroll_pos);
  }


  void forward(int old_scroll_pos) {
    fm_nav_history_forward(dataPtr(), old_scroll_pos);
  }


  bool canForward(void) {
    return fm_nav_history_can_forward(dataPtr());
  }


  void back(int old_scroll_pos) {
    fm_nav_history_back(dataPtr(), old_scroll_pos);
  }


  // automatic type casting for GObject
  operator GObject*() {
    return reinterpret_cast<GObject*>(dataPtr_);
  }


protected:
  GObject* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_NAV_HISTORY_H__

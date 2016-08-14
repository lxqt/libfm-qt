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

#ifndef __LIBFM_QT_FM_FOLDER_H__
#define __LIBFM_QT_FM_FOLDER_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API Folder {
public:


  // default constructor
  Folder() {
    dataPtr_ = nullptr;
  }


  Folder(FmFolder* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<GObject*>(g_object_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  Folder(const Folder& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  Folder(Folder&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
  }


  // destructor
  virtual ~Folder() {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static Folder wrapPtr(FmFolder* dataPtr) {
    Folder obj;
    obj.dataPtr_ = reinterpret_cast<GObject*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmFolder* takeDataPtr() {
    FmFolder* data = reinterpret_cast<FmFolder*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmFolder* dataPtr() {
    return reinterpret_cast<FmFolder*>(dataPtr_);
  }

  // automatic type casting
  operator FmFolder*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  Folder& operator=(const Folder& other) {
    if(dataPtr_ != nullptr) {
      g_object_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<GObject*>(g_object_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  Folder& operator=(Folder&& other) {
    dataPtr_ = reinterpret_cast<GObject*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  bool makeDirectory(const char* name, GError** error) {
    return fm_folder_make_directory(dataPtr(), name, error);
  }


  void queryFilesystemInfo(void) {
    fm_folder_query_filesystem_info(dataPtr());
  }


  bool getFilesystemInfo(guint64* total_size, guint64* free_size) {
    return fm_folder_get_filesystem_info(dataPtr(), total_size, free_size);
  }


  void reload(void) {
    fm_folder_reload(dataPtr());
  }


  bool isIncremental(void) {
    return fm_folder_is_incremental(dataPtr());
  }


  bool isValid(void) {
    return fm_folder_is_valid(dataPtr());
  }


  bool isLoaded(void) {
    return fm_folder_is_loaded(dataPtr());
  }


  FmFileInfo* getFileByName(const char* name) {
    return fm_folder_get_file_by_name(dataPtr(), name);
  }


  bool isEmpty(void) {
    return fm_folder_is_empty(dataPtr());
  }


  FmFileInfoList* getFiles(void) {
    return fm_folder_get_files(dataPtr());
  }


  FmPath* getPath(void) {
    return fm_folder_get_path(dataPtr());
  }


  FmFileInfo* getInfo(void) {
    return fm_folder_get_info(dataPtr());
  }


  void unblockUpdates(void) {
    fm_folder_unblock_updates(dataPtr());
  }


  void blockUpdates(void) {
    fm_folder_block_updates(dataPtr());
  }


  static Folder findByPath(FmPath* path) {
    return Folder::wrapPtr(fm_folder_find_by_path(path));
  }


  static Folder fromUri(const char* uri) {
    return Folder::wrapPtr(fm_folder_from_uri(uri));
  }


  static Folder fromPathName(const char* path) {
    return Folder::wrapPtr(fm_folder_from_path_name(path));
  }


  static Folder fromGfile(GFile* gf) {
    return Folder::wrapPtr(fm_folder_from_gfile(gf));
  }


  static Folder fromPath(FmPath* path) {
    return Folder::wrapPtr(fm_folder_from_path(path));
  }


  // automatic type casting for GObject
  operator GObject*() {
    return reinterpret_cast<GObject*>(dataPtr_);
  }


protected:
  GObject* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_FOLDER_H__

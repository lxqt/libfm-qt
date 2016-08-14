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

#ifndef __LIBFM_QT_FM_PATH_H__
#define __LIBFM_QT_FM_PATH_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include <QMetaType>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API PathList {
public:


  PathList(void ) {
    dataPtr_ = reinterpret_cast<FmPathList*>(fm_path_list_new());
  }


  PathList(FmPathList* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<FmPathList*>(fm_list_ref(FM_LIST(dataPtr))) : nullptr;
  }


  // copy constructor
  PathList(const PathList& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmPathList*>(fm_list_ref(FM_LIST(other.dataPtr_))) : nullptr;
  }


  // move constructor
  PathList(PathList&& other) {
    dataPtr_ = reinterpret_cast<FmPathList*>(other.takeDataPtr());
  }


  // destructor
  ~PathList() {
    if(dataPtr_ != nullptr) {
      fm_list_unref(FM_LIST(dataPtr_));
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static PathList wrapPtr(FmPathList* dataPtr) {
    PathList obj;
    obj.dataPtr_ = reinterpret_cast<FmPathList*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmPathList* takeDataPtr() {
    FmPathList* data = reinterpret_cast<FmPathList*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmPathList* dataPtr() {
    return reinterpret_cast<FmPathList*>(dataPtr_);
  }

  // automatic type casting
  operator FmPathList*() {
    return dataPtr();
  }

  // copy assignment
  PathList& operator=(const PathList& other) {
    if(dataPtr_ != nullptr) {
      fm_list_unref(FM_LIST(dataPtr_));
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmPathList*>(fm_list_ref(FM_LIST(other.dataPtr_))) : nullptr;
    return *this;
  }


  // move assignment
  PathList& operator=(PathList&& other) {
    dataPtr_ = reinterpret_cast<FmPathList*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  void writeUriList(GString* buf) {
    fm_path_list_write_uri_list(dataPtr(), buf);
  }

  char* toUriList(void) {
    return fm_path_list_to_uri_list(dataPtr());
  }

  unsigned int getLength() {
    return fm_path_list_get_length(dataPtr());
  }

  bool isEmpty() {
    return fm_path_list_is_empty(dataPtr());
  }

  FmPath* peekHead() {
    return fm_path_list_peek_head(dataPtr());
  }

  GList* peekHeadLink() {
    return fm_path_list_peek_head_link(dataPtr());
  }

  void pushTail(FmPath* path) {
    fm_path_list_push_tail(dataPtr(), path);
  }

  static PathList newFromFileInfoGslist(GSList* fis) {
    return PathList::wrapPtr(fm_path_list_new_from_file_info_gslist(fis));
  }


  static PathList newFromFileInfoGlist(GList* fis) {
    return PathList::wrapPtr(fm_path_list_new_from_file_info_glist(fis));
  }


  static PathList newFromFileInfoList(FmFileInfoList* fis) {
    return PathList::wrapPtr(fm_path_list_new_from_file_info_list(fis));
  }


  static PathList newFromUris(char* const* uris) {
    return PathList::wrapPtr(fm_path_list_new_from_uris(uris));
  }


  static PathList newFromUriList(const char* uri_list) {
    return PathList::wrapPtr(fm_path_list_new_from_uri_list(uri_list));
  }



private:
  FmPathList* dataPtr_; // data pointer for the underlying C struct

};



class LIBFM_QT_API Path {
public:


  // default constructor
  Path() {
    dataPtr_ = nullptr;
  }


  Path(FmPath* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<FmPath*>(fm_path_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  Path(const Path& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmPath*>(fm_path_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  Path(Path&& other) {
    dataPtr_ = reinterpret_cast<FmPath*>(other.takeDataPtr());
  }


  // destructor
  ~Path() {
    if(dataPtr_ != nullptr) {
      fm_path_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static Path wrapPtr(FmPath* dataPtr) {
    Path obj;
    obj.dataPtr_ = reinterpret_cast<FmPath*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmPath* takeDataPtr() {
    FmPath* data = reinterpret_cast<FmPath*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmPath* dataPtr() {
    return reinterpret_cast<FmPath*>(dataPtr_);
  }

  // automatic type casting
  operator FmPath*() {
    return dataPtr();
  }

  // copy assignment
  Path& operator=(const Path& other) {
    if(dataPtr_ != nullptr) {
      fm_path_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmPath*>(fm_path_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  Path& operator=(Path&& other) {
    dataPtr_ = reinterpret_cast<FmPath*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods
  bool isNative() {
    return fm_path_is_native(dataPtr());
  }

  bool isTrash() {
    return fm_path_is_trash(dataPtr());
  }

  bool isTrashRoot() {
    return fm_path_is_trash_root(dataPtr());
  }

  bool isNativeOrTrash() {
    return fm_path_is_native_or_trash(dataPtr());
  }

  int depth(void) {
    return fm_path_depth(dataPtr());
  }


  bool equalStr(const gchar* str, int n) {
    return fm_path_equal_str(dataPtr(), str, n);
  }


  int compare(FmPath* p2) {
    return fm_path_compare(dataPtr(), p2);
  }

  int compare(Path& p2) {
    return fm_path_compare(dataPtr(), p2.dataPtr());
  }

  bool equal(FmPath* p2) {
    return fm_path_equal(dataPtr(), p2);
  }

  bool equal(Path& p2) {
    return fm_path_equal(dataPtr(), p2.dataPtr());
  }

  bool operator == (Path& other) {
    return fm_path_equal(dataPtr(), other.dataPtr());
  }

  bool operator != (Path& other) {
    return !fm_path_equal(dataPtr(), other.dataPtr());
  }

  bool operator < (Path& other) {
    return compare(other);
  }

  bool operator > (Path& other) {
    return (other < *this);
  }

  unsigned int hash(void) {
    return fm_path_hash(dataPtr());
  }


  char* displayBasename(void) {
    return fm_path_display_basename(dataPtr());
  }

  char* displayName(gboolean human_readable) {
    return fm_path_display_name(dataPtr(), human_readable);
  }


  GFile* toGfile(void) {
    return fm_path_to_gfile(dataPtr());
  }


  char* toUri(void) {
    return fm_path_to_uri(dataPtr());
  }


  char* toStr(void) {
    return fm_path_to_str(dataPtr());
  }


  Path getSchemePath(void) {
    return Path(fm_path_get_scheme_path(dataPtr()));
  }


  bool hasPrefix(FmPath* prefix) {
    return fm_path_has_prefix(dataPtr(), prefix);
  }


  FmPathFlags getFlags(void) {
    return fm_path_get_flags(dataPtr());
  }


  Path getParent(void) {
    return Path(fm_path_get_parent(dataPtr()));
  }


  static Path getAppsMenu(void ) {
    return Path(fm_path_get_apps_menu());
  }


  static Path getTrash(void ) {
    return Path(fm_path_get_trash());
  }


  static Path getDesktop(void ) {
    return Path(fm_path_get_desktop());
  }


  static Path getHome(void ) {
    return Path(fm_path_get_home());
  }


  static Path getRoot(void ) {
    return Path(fm_path_get_root());
  }


  static Path newForGfile(GFile* gf) {
    return Path::wrapPtr(fm_path_new_for_gfile(gf));
  }


  Path newRelative(const char* rel) {
    return Path::wrapPtr(fm_path_new_relative(dataPtr(), rel));
  }


  Path newChildLen(const char* basename, int name_len) {
    return Path::wrapPtr(fm_path_new_child_len(dataPtr(), basename, name_len));
  }


  Path newChild(const char* basename) {
    return Path::wrapPtr(fm_path_new_child(dataPtr(), basename));
  }


  static Path newForCommandlineArg(const char* arg) {
    return Path::wrapPtr(fm_path_new_for_commandline_arg(arg));
  }


  static Path newForStr(const char* path_str) {
    return Path::wrapPtr(fm_path_new_for_str(path_str));
  }


  static Path newForDisplayName(const char* path_name) {
    return Path::wrapPtr(fm_path_new_for_display_name(path_name));
  }


  static Path newForUri(const char* uri) {
    return Path::wrapPtr(fm_path_new_for_uri(uri));
  }


  static Path newForPath(const char* path_name) {
    return Path::wrapPtr(fm_path_new_for_path(path_name));
  }



private:
  FmPath* dataPtr_; // data pointer for the underlying C struct

};

}

Q_DECLARE_OPAQUE_POINTER(FmPath*)

#endif // __LIBFM_QT_FM_PATH_H__

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

#ifndef __LIBFM_QT_FM_FILE_INFO_H__
#define __LIBFM_QT_FM_FILE_INFO_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"


namespace Fm {


class LIBFM_QT_API FileInfoList {
public:


  FileInfoList( ) {
    dataPtr_ = reinterpret_cast<FmFileInfoList*>(fm_file_info_list_new());
  }


  FileInfoList(FmFileInfoList* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<FmFileInfoList*>(fm_list_ref(FM_LIST(dataPtr))) : nullptr;
  }


  // copy constructor
  FileInfoList(const FileInfoList& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmFileInfoList*>(fm_list_ref(FM_LIST(other.dataPtr_))) : nullptr;
  }


  // move constructor
  FileInfoList(FileInfoList&& other) {
    dataPtr_ = reinterpret_cast<FmFileInfoList*>(other.takeDataPtr());
  }


  // destructor
  ~FileInfoList() {
    if(dataPtr_ != nullptr) {
      fm_list_unref(FM_LIST(dataPtr_));
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static FileInfoList wrapPtr(FmFileInfoList* dataPtr) {
    FileInfoList obj;
    obj.dataPtr_ = reinterpret_cast<FmFileInfoList*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmFileInfoList* takeDataPtr() {
    FmFileInfoList* data = reinterpret_cast<FmFileInfoList*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmFileInfoList* dataPtr() {
    return reinterpret_cast<FmFileInfoList*>(dataPtr_);
  }
  
  // automatic type casting
  operator FmFileInfoList*() {
    return dataPtr();
  }

  // automatic type casting
  operator void*() {
    return dataPtr();
  }


  // copy assignment
  FileInfoList& operator=(const FileInfoList& other) {
    if(dataPtr_ != nullptr) {
      fm_list_unref(FM_LIST(dataPtr_));
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmFileInfoList*>(fm_list_ref(FM_LIST(other.dataPtr_))) : nullptr;
    return *this;
  }


  // move assignment
  FileInfoList& operator=(FileInfoList&& other) {
    dataPtr_ = reinterpret_cast<FmFileInfoList*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  bool isSameFs(void) {
    return fm_file_info_list_is_same_fs(dataPtr());
  }


  bool isSameType(void) {
    return fm_file_info_list_is_same_type(dataPtr());
  }


  bool isEmpty() {
    return fm_file_info_list_is_empty(dataPtr());
  }

  unsigned int getLength() {
    return fm_file_info_list_get_length(dataPtr());
  }

  FmFileInfo* peekHead() {
    return fm_file_info_list_peek_head(dataPtr());
  }

  GList* peekHeadLink() {
    return fm_file_info_list_peek_head_link(dataPtr());
  }

  void pushTail(FmFileInfo* d) {
    fm_file_info_list_push_tail(dataPtr(), d);
  }

  void pushTailLink(GList* d) {
    fm_file_info_list_push_tail_link(dataPtr(), d);
  }

  FmFileInfo* popHead(){
    return fm_file_info_list_pop_head(dataPtr());
  }

  void deleteLink(GList* _l) {
    fm_file_info_list_delete_link(dataPtr(), _l);
  }

  void clear() {
    fm_file_info_list_clear(dataPtr());
  }


private:
  FmFileInfoList* dataPtr_; // data pointer for the underlying C struct

};



class LIBFM_QT_API FileInfo {
public:


  FileInfo( ) {
    dataPtr_ = reinterpret_cast<FmFileInfo*>(fm_file_info_new());
  }


  FileInfo(FmFileInfo* dataPtr){
    dataPtr_ = dataPtr != nullptr ? reinterpret_cast<FmFileInfo*>(fm_file_info_ref(dataPtr)) : nullptr;
  }


  // copy constructor
  FileInfo(const FileInfo& other) {
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmFileInfo*>(fm_file_info_ref(other.dataPtr_)) : nullptr;
  }


  // move constructor
  FileInfo(FileInfo&& other) {
    dataPtr_ = reinterpret_cast<FmFileInfo*>(other.takeDataPtr());
  }


  // destructor
  ~FileInfo() {
    if(dataPtr_ != nullptr) {
      fm_file_info_unref(dataPtr_);
    }
  }


  // create a wrapper for the data pointer without increasing the reference count
  static FileInfo wrapPtr(FmFileInfo* dataPtr) {
    FileInfo obj;
    obj.dataPtr_ = reinterpret_cast<FmFileInfo*>(dataPtr);
    return obj;
  }

  // disown the managed data pointer
  FmFileInfo* takeDataPtr() {
    FmFileInfo* data = reinterpret_cast<FmFileInfo*>(dataPtr_);
    dataPtr_ = nullptr;
    return data;
  }

  // get the raw pointer wrapped
  FmFileInfo* dataPtr() {
    return reinterpret_cast<FmFileInfo*>(dataPtr_);
  }

  // automatic type casting
  operator FmFileInfo*() {
    return dataPtr();
  }


  // copy assignment
  FileInfo& operator=(const FileInfo& other) {
    if(dataPtr_ != nullptr) {
      fm_file_info_unref(dataPtr_);
    }
    dataPtr_ = other.dataPtr_ != nullptr ? reinterpret_cast<FmFileInfo*>(fm_file_info_ref(other.dataPtr_)) : nullptr;
    return *this;
  }


  // move assignment
  FileInfo& operator=(FileInfo&& other) {
    dataPtr_ = reinterpret_cast<FmFileInfo*>(other.takeDataPtr());
    return *this;
  }

  bool isNull() {
    return (dataPtr_ == nullptr);
  }

  // methods

  bool canSetHidden(void) {
    return fm_file_info_can_set_hidden(dataPtr());
  }


  bool canSetIcon(void) {
    return fm_file_info_can_set_icon(dataPtr());
  }


  bool canSetName(void) {
    return fm_file_info_can_set_name(dataPtr());
  }


  bool canThumbnail(void) {
    return fm_file_info_can_thumbnail(dataPtr());
  }


  dev_t getDev(void) {
    return fm_file_info_get_dev(dataPtr());
  }


  gid_t getGid(void) {
    return fm_file_info_get_gid(dataPtr());
  }


  uid_t getUid(void) {
    return fm_file_info_get_uid(dataPtr());
  }

  const char* getDispGroup() {
    return fm_file_info_get_disp_group(dataPtr());
  }

  const char* getFsId() {
    return fm_file_info_get_fs_id(dataPtr());
  }

  FmIcon* getIcon(void) {
    return fm_file_info_get_icon(dataPtr());
  }


  time_t getCtime(void) {
    return fm_file_info_get_ctime(dataPtr());
  }


  time_t getAtime(void) {
    return fm_file_info_get_atime(dataPtr());
  }


  time_t getMtime(void) {
    return fm_file_info_get_mtime(dataPtr());
  }


  const char* getTarget() {
    return fm_file_info_get_target(dataPtr());
  }

  const char* getCollateKey() {
    return fm_file_info_get_collate_key(dataPtr());
  }

  const char* getCollateKeyNoCaseFold() {
    return fm_file_info_get_collate_key_nocasefold(dataPtr());
  }

  const char* getDesc() {
    return fm_file_info_get_desc(dataPtr());
  }

  const char* getDispMtime() {
    return fm_file_info_get_disp_mtime(dataPtr());
  }

  bool isWritableDirectory(void) {
    return fm_file_info_is_writable_directory(dataPtr());
  }


  bool isAccessible(void) {
    return fm_file_info_is_accessible(dataPtr());
  }


  bool isExecutableType(void) {
    return fm_file_info_is_executable_type(dataPtr());
  }


  bool isBackup(void) {
    return fm_file_info_is_backup(dataPtr());
  }


  bool isHidden(void) {
    return fm_file_info_is_hidden(dataPtr());
  }


  bool isUnknownType(void) {
    return fm_file_info_is_unknown_type(dataPtr());
  }


  bool isDesktopEntry(void) {
    return fm_file_info_is_desktop_entry(dataPtr());
  }


  bool isText(void) {
    return fm_file_info_is_text(dataPtr());
  }


  bool isImage(void) {
    return fm_file_info_is_image(dataPtr());
  }


  bool isMountable(void) {
    return fm_file_info_is_mountable(dataPtr());
  }


  bool isShortcut(void) {
    return fm_file_info_is_shortcut(dataPtr());
  }


  bool isSymlink(void) {
    return fm_file_info_is_symlink(dataPtr());
  }


  bool isDir(void) {
    return fm_file_info_is_dir(dataPtr());
  }


  FmMimeType* getMimeType(void) {
    return fm_file_info_get_mime_type(dataPtr());
  }


  bool isNative(void) {
    return fm_file_info_is_native(dataPtr());
  }


  mode_t getMode(void) {
    return fm_file_info_get_mode(dataPtr());
  }


  goffset getBlocks(void) {
    return fm_file_info_get_blocks(dataPtr());
  }


  goffset getSize(void) {
    return fm_file_info_get_size(dataPtr());
  }

  const char* getDispSize() {
    return fm_file_info_get_disp_size(dataPtr());
  }

  void setIcon(GIcon* icon) {
    fm_file_info_set_icon(dataPtr(), icon);
  }


  void setDispName(const char* name) {
    fm_file_info_set_disp_name(dataPtr(), name);
  }


  void setPath(FmPath* path) {
    fm_file_info_set_path(dataPtr(), path);
  }


  const char* getName() {
    return fm_file_info_get_name(dataPtr());
  }


  const char* getDispName() {
    return fm_file_info_get_disp_name(dataPtr());
  }


  FmPath* getPath(void) {
    return fm_file_info_get_path(dataPtr());
  }


  void update(FmFileInfo* src) {
    fm_file_info_update(dataPtr(), src);
  }


  static FileInfo newFromNativeFile(FmPath* path, const char* path_str, GError** err) {
    return FileInfo::wrapPtr(fm_file_info_new_from_native_file(path, path_str, err));
  }


  bool setFromNativeFile(const char* path, GError** err) {
    return fm_file_info_set_from_native_file(dataPtr(), path, err);
  }


  void setFromMenuCacheItem(struct _MenuCacheItem* item) {
    fm_file_info_set_from_menu_cache_item(dataPtr(), item);
  }


  static FileInfo newFromMenuCacheItem(FmPath* path, struct _MenuCacheItem* item) {
    return FileInfo::wrapPtr(fm_file_info_new_from_menu_cache_item(path, item));
  }


  void setFromGFileData(GFile* gf, GFileInfo* inf) {
    fm_file_info_set_from_g_file_data(dataPtr(), gf, inf);
  }


  static FileInfo newFromGFileData(GFile* gf, GFileInfo* inf, FmPath* path) {
    return FileInfo::wrapPtr(fm_file_info_new_from_g_file_data(gf, inf, path));
  }


  void setFromGfileinfo(GFileInfo* inf) {
    fm_file_info_set_from_gfileinfo(dataPtr(), inf);
  }


  static FileInfo newFromGfileinfo(FmPath* path, GFileInfo* inf) {
    return FileInfo::wrapPtr(fm_file_info_new_from_gfileinfo(path, inf));
  }


private:
  FmFileInfo* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_FILE_INFO_H__

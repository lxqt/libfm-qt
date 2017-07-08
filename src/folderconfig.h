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

#ifndef __LIBFM_QT_FM_FOLDER_CONFIG_H__
#define __LIBFM_QT_FM_FOLDER_CONFIG_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include "libfmqtglobals.h"

#include "core/filepath.h"

namespace Fm {

// FIXME: port to the new API and drop libfm dependency

class LIBFM_QT_API FolderConfig {
public:

    FolderConfig(const Fm::FilePath& path) {
        FmPath* fmpath = fm_path_new_for_gfile(path.gfile().get());
        dataPtr_ = reinterpret_cast<FmFolderConfig*>(fm_folder_config_open(fmpath));
        fm_path_unref(fmpath);
    }


    // default constructor
    FolderConfig() {
        dataPtr_ = nullptr;
    }


    // move constructor
    FolderConfig(FolderConfig&& other) noexcept {
        dataPtr_ = reinterpret_cast<FmFolderConfig*>(other.takeDataPtr());
    }


    // destructor
    ~FolderConfig() {
        if(dataPtr_ != nullptr) {
            fm_folder_config_close(dataPtr_, nullptr);
        }
    }


    // create a wrapper for the data pointer without increasing the reference count
    static FolderConfig wrapPtr(FmFolderConfig* dataPtr) {
        FolderConfig obj;
        obj.dataPtr_ = reinterpret_cast<FmFolderConfig*>(dataPtr);
        return obj;
    }

    // disown the managed data pointer
    FmFolderConfig* takeDataPtr() {
        FmFolderConfig* data = reinterpret_cast<FmFolderConfig*>(dataPtr_);
        dataPtr_ = nullptr;
        return data;
    }

    // get the raw pointer wrapped
    FmFolderConfig* dataPtr() {
        return reinterpret_cast<FmFolderConfig*>(dataPtr_);
    }

    // automatic type casting
    operator FmFolderConfig* () {
        return dataPtr();
    }

    // automatic type casting
    operator void* () {
        return dataPtr();
    }



    // move assignment
    FolderConfig& operator=(FolderConfig&& other) noexcept {
        dataPtr_ = reinterpret_cast<FmFolderConfig*>(other.takeDataPtr());
        return *this;
    }

    bool isNull() {
        return (dataPtr_ == nullptr);
    }

    // methods

    static void saveCache(void) {
        fm_folder_config_save_cache();
    }


    void purge(void) {
        fm_folder_config_purge(dataPtr());
    }


    void removeKey(const char* key) {
        fm_folder_config_remove_key(dataPtr(), key);
    }


    void setStringList(const char* key, const gchar* const list[], gsize length) {
        fm_folder_config_set_string_list(dataPtr(), key, list, length);
    }


    void setString(const char* key, const char* string) {
        fm_folder_config_set_string(dataPtr(), key, string);
    }


    void setBoolean(const char* key, gboolean val) {
        fm_folder_config_set_boolean(dataPtr(), key, val);
    }


    void setDouble(const char* key, gdouble val) {
        fm_folder_config_set_double(dataPtr(), key, val);
    }


    void setUint64(const char* key, guint64 val) {
        fm_folder_config_set_uint64(dataPtr(), key, val);
    }


    void setInteger(const char* key, gint val) {
        fm_folder_config_set_integer(dataPtr(), key, val);
    }


    char** getStringList(const char* key, gsize* length) {
        return fm_folder_config_get_string_list(dataPtr(), key, length);
    }


    char* getString(const char* key) {
        return fm_folder_config_get_string(dataPtr(), key);
    }


    bool getBoolean(const char* key, gboolean* val) {
        return fm_folder_config_get_boolean(dataPtr(), key, val);
    }


    bool getDouble(const char* key, gdouble* val) {
        return fm_folder_config_get_double(dataPtr(), key, val);
    }


    bool getUint64(const char* key, guint64* val) {
        return fm_folder_config_get_uint64(dataPtr(), key, val);
    }


    bool getInteger(const char* key, gint* val) {
        return fm_folder_config_get_integer(dataPtr(), key, val);
    }


    bool isEmpty(void) {
        return fm_folder_config_is_empty(dataPtr());
    }


// the wrapped object cannot be copied.
private:
    FolderConfig(const FolderConfig& other) = delete;
    FolderConfig& operator=(const FolderConfig& other) = delete;


private:
    FmFolderConfig* dataPtr_; // data pointer for the underlying C struct

};


}

#endif // __LIBFM_QT_FM_FOLDER_CONFIG_H__

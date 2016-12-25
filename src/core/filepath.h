#ifndef FM2_FILEPATH_H
#define FM2_FILEPATH_H

#include "gobjectptr.h"
#include "cstrptr.h"
#include <gio/gio.h>


namespace Fm2 {

class FilePath {
public:

    FilePath() {
    }

    FilePath(const char *path_str): gfile_{g_file_new_for_commandline_arg(path_str), false} {
    }

    FilePath(GFile* gfile, bool add_ref): gfile_{gfile, add_ref} {
    }

    FilePath(const FilePath& other) = default;

    FilePath(FilePath&& other) = default;

    static FilePath fromUri(const char* uri) {
        return FilePath{g_file_new_for_uri(uri), false};
    }

    static FilePath fromLocalPath(const char* path) {
        return FilePath{g_file_new_for_path(path), false};
    }

    bool isValid() const {
        return gfile_ != nullptr;
    }

    unsigned int hash() const {
        return g_file_hash(gfile_.get());
    }

    CStrPtr baseName() const {
        return CStrPtr{g_file_get_basename(gfile_.get()), g_free};
    }

    CStrPtr localPath() const {
        return CStrPtr{g_file_get_path(gfile_.get()), g_free};
    }

    CStrPtr uri() const {
        return CStrPtr{g_file_get_uri(gfile_.get()), g_free};
    }

    CStrPtr toString() const {
        if(isNative()) {
            return localPath();
        }
        return uri();
    }

    FilePath parent() const {
        return FilePath{g_file_get_parent(gfile_.get()), false};
    }

    bool isParentOf(const FilePath& other) {
        return g_file_has_parent(other.gfile_.get(), gfile_.get());
    }

    bool isPrefixOf(const FilePath& other) {
        return g_file_has_prefix(other.gfile_.get(), gfile_.get());
    }

    FilePath child(const char* name) const {
        return FilePath{g_file_get_child(gfile_.get(), name), false};
    }

    CStrPtr relativePathStr(const FilePath& descendant) const {
        return CStrPtr{g_file_get_relative_path(gfile_.get(), descendant.gfile_.get()), g_free};
    }

    FilePath relativePath(const char* relPath) const {
        return FilePath{g_file_resolve_relative_path(gfile_.get(), relPath), false};
    }

    bool isNative() const {
        return g_file_is_native(gfile_.get());
    }

    bool hasUriScheme(const char* scheme) const {
        return g_file_has_uri_scheme(gfile_.get(), scheme);
    }

    CStrPtr uriScheme() const {
        return CStrPtr{g_file_get_uri_scheme(gfile_.get()), g_free};
    }

    const GObjectPtr<GFile>& gfile() const {
        return gfile_;
    }

private:
    GObjectPtr<GFile> gfile_;
};

} // namespace Fm2

#endif // FM2_FILEPATH_H

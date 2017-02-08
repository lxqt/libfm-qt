#ifndef FM2_CSTRPTR_H
#define FM2_CSTRPTR_H

#include <memory>
#include <glib.h>

namespace Fm {

struct CStrDeleter {
    void operator()(char* ptr) {
        g_free(ptr);
    }
};

typedef std::unique_ptr<char, CStrDeleter> CStrPtr;

struct CStrHash {
    std::size_t operator()(const char* str) const {
        return g_str_hash(str);
    }
};

struct CStrEqual {
    bool operator()(const char* str1, const char* str2) const {
        return g_str_equal(str1, str2);
    }
};

} // namespace Fm

#endif // FM2_CSTRPTR_H

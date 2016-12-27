#ifndef FM2_CSTRPTR_H
#define FM2_CSTRPTR_H

#include <memory>
#include <glib.h>

namespace Fm2 {

typedef std::unique_ptr<char, decltype(&g_free)> CStrPtr;

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

} // namespace Fm2

#endif // FM2_CSTRPTR_H
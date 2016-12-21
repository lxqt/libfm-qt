#ifndef FM2_CSTRPTR_H
#define FM2_CSTRPTR_H

#include <memory>
#include <glib.h>

namespace Fm2 {

typedef std::unique_ptr<char, decltype(&g_free)> CStrPtr;

} // namespace Fm2

#endif // FM2_CSTRPTR_H

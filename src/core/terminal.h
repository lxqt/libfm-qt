#ifndef TERMINAL_H
#define TERMINAL_H

#include "libfmqtglobals.h"
#include "gioptrs.h"
#include "filepath.h"
#include <vector>

namespace Fm2 {

LIBFM_QT_API bool launchTerminal(const char* programName, const FilePath& workingDir, GErrorPtr& error);

LIBFM_QT_API std::vector<CStrPtr> allKnownTerminals();

} // namespace Fm2

#endif // TERMINAL_H

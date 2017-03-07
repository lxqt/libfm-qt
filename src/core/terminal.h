#ifndef TERMINAL_H
#define TERMINAL_H

#include "../libfmqtglobals.h"
#include "gioptrs.h"
#include "filepath.h"
#include <vector>

namespace Fm {

LIBFM_QT_API bool launchTerminal(const char* programName, const FilePath& workingDir, GErrorPtr& error);

LIBFM_QT_API std::vector<CStrPtr> allKnownTerminals();

} // namespace Fm

#endif // TERMINAL_H

#ifndef FM2_FILEMONITOR_H
#define FM2_FILEMONITOR_H

#include "libfmqtglobals.h"
#include <QObject>
#include "gioptrs.h"
#include "filepath.h"

namespace Fm2 {

class LIBFM_QT_API FileMonitor: public QObject {
    Q_OBJECT
public:

    FileMonitor();

Q_SIGNALS:


private:
    GFileMonitorPtr monitor_;
};

} // namespace Fm2

#endif // FM2_FILEMONITOR_H

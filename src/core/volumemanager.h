#ifndef FM2_VOLUMEMANAGER_H
#define FM2_VOLUMEMANAGER_H

#include <QObject>

namespace Fm2 {

class VolumeManager : public QObject {
    Q_OBJECT
public:
    explicit VolumeManager();

Q_SIGNALS:

public Q_SLOTS:
};

} // namespace Fm2

#endif // FM2_VOLUMEMANAGER_H

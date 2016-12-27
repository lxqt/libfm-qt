#ifndef FM2_VOLUMEMANAGER_H
#define FM2_VOLUMEMANAGER_H

#include <QObject>
#include <gio/gio.h>
#include "gobjectptr.h"
#include "gsignalhandler.h"

namespace Fm2 {

class Volume: public GObjectPtr<GVolume> {
public:
};

class Mount: public GObjectPtr<GMount> {
public:

};

class VolumeManager : public QObject {
    Q_OBJECT
public:
    explicit VolumeManager();

Q_SIGNALS:
    void VolumeAdded(const Volume& vol);
    void VolumeRemoved(const Volume& vol);
    void VolumeChanged(const Volume& vol);

    void MountAdded(const Mount& mnt);
    void MountRemoved(const Mount& mnt);
    void MountChanged(const Mount& mnt);

public Q_SLOTS:

private:
    void onGVolumeAdded(GVolumeMonitor* mon, GVolume* vol);
    void onGVolumeRemoved(GVolumeMonitor* mon, GVolume* vol);
    void onGVolumeChanged(GVolumeMonitor* mon, GVolume* vol);

    void onGMountAdded(GVolumeMonitor* mon, GMount* mnt);
    void onGMountRemoved(GVolumeMonitor* mon, GMount* mnt);
    void onGMountChanged(GVolumeMonitor* mon, GMount* mnt);

private:
    GObjectPtr<GVolumeMonitor> monitor_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GVolume*> volAdded_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GVolume*> volRemoved_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GVolume*> volChanged_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GMount*> mntAdded_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GMount*> mntRemoved_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GMount*> mntChanged_;
};

} // namespace Fm2

#endif // FM2_VOLUMEMANAGER_H

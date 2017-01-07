#include "volumemanager.h"

namespace Fm2 {

VolumeManager::VolumeManager():
    QObject(),
    monitor_{g_volume_monitor_get(), false},
    volAdded_{this, &VolumeManager::onGVolumeAdded},
    volRemoved_{this, &VolumeManager::onGVolumeRemoved},
    volChanged_{this, &VolumeManager::onGVolumeChanged},
    mntAdded_{this, &VolumeManager::onGMountAdded},
    mntRemoved_{this, &VolumeManager::onGMountRemoved},
    mntChanged_{this, &VolumeManager::onGMountChanged} {

    // connect gobject signal handlers
    volAdded_.connect(monitor_.get(), "volume-added");
    volRemoved_.connect(monitor_.get(), "volume-removd");
    volChanged_.connect(monitor_.get(), "volume-changed");

    mntAdded_.connect(monitor_.get(), "mount-added");
    mntRemoved_.connect(monitor_.get(), "mount-removd");
    mntChanged_.connect(monitor_.get(), "mount-changed");

}

void VolumeManager::onGVolumeAdded(GVolumeMonitor* mon, GVolume* vol) {

}

void VolumeManager::onGVolumeRemoved(GVolumeMonitor* mon, GVolume* vol) {

}

void VolumeManager::onGVolumeChanged(GVolumeMonitor* mon, GVolume* vol) {

}

void VolumeManager::onGMountAdded(GVolumeMonitor* mon, GMount* mnt) {

}

void VolumeManager::onGMountRemoved(GVolumeMonitor* mon, GMount* mnt) {

}

void VolumeManager::onGMountChanged(GVolumeMonitor* mon, GMount* mnt) {

}


} // namespace Fm2

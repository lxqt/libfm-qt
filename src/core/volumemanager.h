#ifndef FM2_VOLUMEMANAGER_H
#define FM2_VOLUMEMANAGER_H

#include "libfmqtglobals.h"
#include <QObject>
#include <gio/gio.h>
#include "gioptrs.h"
#include "gsignalhandler.h"
#include "filepath.h"
#include "icon.h"
#include <vector>

namespace Fm2 {

class LIBFM_QT_API Volume: public GVolumePtr {
public:

    Volume(GVolume* gvol, bool addRef): GVolumePtr{gvol, addRef} {
    }

    CStrPtr name() const {
        return CStrPtr{g_volume_get_name(get())};
    }

    CStrPtr uuid() const {
        return CStrPtr{g_volume_get_uuid(get())};
    }

    Icon icon() {
        return Icon{GIconPtr{g_volume_get_icon(get()), false}};
    }

    // GDrive *	g_volume_get_drive(get());
    GMountPtr mount() const {
        return GMountPtr{g_volume_get_mount(get()), false};
    }

    bool canMount() const {
        return g_volume_can_mount(get());
    }

    bool shouldAutoMount() const {
        return g_volume_should_automount(get());
    }

    FilePath activationRoot() const {
        return FilePath{g_volume_get_activation_root(get()), false};
    }

    /*
    void	g_volume_mount(get());
    gboolean	g_volume_mount_finish(get());
    */
    bool canEject() const {
        return g_volume_can_eject(get());
    }

    /*
    void	g_volume_eject(get());
    gboolean	g_volume_eject_finish(get());
    void	g_volume_eject_with_operation(get());
    gboolean	g_volume_eject_with_operation_finish(get());
    char **	g_volume_enumerate_identifiers(get());
    char *	g_volume_get_identifier(get());
    const gchar *	g_volume_get_sort_key(get());
    */

    CStrPtr device() const {
        return CStrPtr{g_volume_get_identifier(get(), G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE)};
    }

    CStrPtr label() const {
        return CStrPtr{g_volume_get_identifier(get(), G_VOLUME_IDENTIFIER_KIND_LABEL)};
    }

};


class LIBFM_QT_API Mount: public GMountPtr {
public:

    CStrPtr name() const {
        return CStrPtr{g_mount_get_name(get())};
    }

    CStrPtr uuid() const {
        return CStrPtr{g_mount_get_uuid(get())};
    }

    Icon icon() const {
        return Icon{GIconPtr{g_mount_get_icon(get()), false}};
    }

    // GIcon *	g_mount_get_symbolic_icon(get());
    // GDrive *	g_mount_get_drive(get());
    FilePath root() const {
        return FilePath{g_mount_get_root(get()), false};
    }

    Volume volume() const {
        Volume{g_mount_get_volume(get()), false};
    }

    FilePath defaultLocation() const {
        return FilePath{g_mount_get_default_location(get()), false};
    }

    bool canUnmount() const {
        return g_mount_can_unmount(get());
    }

/*
    void	g_mount_unmount(get());
    gboolean	g_mount_unmount_finish(get());
    void	g_mount_unmount_with_operation(get());
    gboolean	g_mount_unmount_with_operation_finish(get());
    void	g_mount_remount(get());
    gboolean	g_mount_remount_finish(get());
*/
    bool canEject() const {
        return g_mount_can_eject(get());
    }

/*
    void	g_mount_eject(get());
    gboolean	g_mount_eject_finish(get());
    void	g_mount_eject_with_operation(get());
    gboolean	g_mount_eject_with_operation_finish(get());
*/
    // void	g_mount_guess_content_type(get());
    // gchar **	g_mount_guess_content_type_finish(get());
    // gchar **	g_mount_guess_content_type_sync(get());

    bool isShadowed() const {
        return g_mount_is_shadowed(get());
    }

    // void	g_mount_shadow(get());
    // void	g_mount_unshadow(get());
    // const gchar *	g_mount_get_sort_key(get());
};


class LIBFM_QT_API VolumeManager : public QObject {
    Q_OBJECT
public:
    explicit VolumeManager();

    const std::vector<Volume>& volumes() const {
        return volumes_;
    }

    const std::vector<Mount>& mounts() const {
        return mounts_;
    }

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
    GVolumeMonitorPtr monitor_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GVolume*> volAdded_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GVolume*> volRemoved_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GVolume*> volChanged_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GMount*> mntAdded_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GMount*> mntRemoved_;
    GSignalHandler<VolumeManager, GVolumeMonitor, void, GMount*> mntChanged_;

    std::vector<Volume> volumes_;
    std::vector<Mount> mounts_;
};

} // namespace Fm2

#endif // FM2_VOLUMEMANAGER_H

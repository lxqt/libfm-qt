#ifndef FM2_VOLUMEMANAGER_H
#define FM2_VOLUMEMANAGER_H

#include "libfmqtglobals.h"
#include <QObject>
#include <gio/gio.h>
#include "gioptrs.h"
#include "gsignalhandler.h"
#include <vector>

namespace Fm2 {

class LIBFM_QT_API Volume: public GVolumePtr {
public:
#if 0
    char *	g_volume_get_name ()
    char *	g_volume_get_uuid ()
    GIcon *	g_volume_get_icon ()
    GIcon *	g_volume_get_symbolic_icon ()
    GDrive *	g_volume_get_drive ()
    GMount *	g_volume_get_mount ()
    gboolean	g_volume_can_mount ()
    gboolean	g_volume_should_automount ()
    GFile *	g_volume_get_activation_root ()
    void	g_volume_mount ()
    gboolean	g_volume_mount_finish ()
    gboolean	g_volume_can_eject ()
    void	g_volume_eject ()
    gboolean	g_volume_eject_finish ()
    void	g_volume_eject_with_operation ()
    gboolean	g_volume_eject_with_operation_finish ()
    char **	g_volume_enumerate_identifiers ()
    char *	g_volume_get_identifier ()
    const gchar *	g_volume_get_sort_key ()
#endif
};

class LIBFM_QT_API Mount: public GMountPtr {
public:
#if 0
    char *	g_mount_get_name ()
    char *	g_mount_get_uuid ()
    GIcon *	g_mount_get_icon ()
    GIcon *	g_mount_get_symbolic_icon ()
    GDrive *	g_mount_get_drive ()
    GFile *	g_mount_get_root ()
    GVolume *	g_mount_get_volume ()
    GFile *	g_mount_get_default_location ()
    gboolean	g_mount_can_unmount ()
    void	g_mount_unmount ()
    gboolean	g_mount_unmount_finish ()
    void	g_mount_unmount_with_operation ()
    gboolean	g_mount_unmount_with_operation_finish ()
    void	g_mount_remount ()
    gboolean	g_mount_remount_finish ()
    gboolean	g_mount_can_eject ()
    void	g_mount_eject ()
    gboolean	g_mount_eject_finish ()
    void	g_mount_eject_with_operation ()
    gboolean	g_mount_eject_with_operation_finish ()
    void	g_mount_guess_content_type ()
    gchar **	g_mount_guess_content_type_finish ()
    gchar **	g_mount_guess_content_type_sync ()
    gboolean	g_mount_is_shadowed ()
    void	g_mount_shadow ()
    void	g_mount_unshadow ()
    const gchar *	g_mount_get_sort_key ()
#endif
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

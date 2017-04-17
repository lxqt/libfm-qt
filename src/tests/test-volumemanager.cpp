#include <QApplication>
#include <QDir>
#include <QDebug>
#include "../core/volumemanager.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    auto vm = Fm::VolumeManager::globalInstance();

    QObject::connect(vm.get(), &Fm::VolumeManager::volumeAdded, [=](const Fm::Volume& vol) {
        qDebug() << "volume added:" << vol.name().get();
    });
    QObject::connect(vm.get(), &Fm::VolumeManager::volumeRemoved, [=](const Fm::Volume& vol) {
        qDebug() << "volume removed:" << vol.name().get();
    });

    for(auto& item: vm->volumes()) {
        auto name = item.name();
        qDebug() << "list volume:" << name.get();
    }

    return app.exec();
}

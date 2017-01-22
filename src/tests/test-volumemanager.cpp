#include <QApplication>
#include <QDir>
#include <QDebug>
#include "../core/volumemanager.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    auto vm = Fm2::VolumeManager::globalInstance();

    QObject::connect(vm.get(), &Fm2::VolumeManager::volumeAdded, [=](const Fm2::Volume& vol) {
        qDebug() << "volume added:" << vol.name().get();
    });
    QObject::connect(vm.get(), &Fm2::VolumeManager::volumeRemoved, [=](const Fm2::Volume& vol) {
        qDebug() << "volume removed:" << vol.name().get();
    });

    for(auto& item: vm->volumes()) {
        auto name = item.name();
        qDebug() << "list volume:" << name.get();
    }

    return app.exec();
}

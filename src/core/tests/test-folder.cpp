#include <QApplication>
#include <QDir>
#include <QDebug>
#include "../folder.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    auto home = Fm2::FilePath::fromLocalPath(QDir().homePath().toLocal8Bit().constData());
    auto folder = Fm2::Folder::fromPath(home);

    QObject::connect(folder.get(), &Fm2::Folder::startLoading, [=]() {
        qDebug("start loading");
    });
    QObject::connect(folder.get(), &Fm2::Folder::finishLoading, [=]() {
        qDebug("finish loading");
    });

    QObject::connect(folder.get(), &Fm2::Folder::filesAdded, [=](Fm2::FileInfoList& files) {
        qDebug("files added");
        for(auto& item: files) {
            qDebug() << item->getName().c_str();
            // qDebug() << item->getDispName();
        }
    });

    for(auto& item: folder->getFiles()) {
        qDebug() << item->getDispName();
    }
    qDebug() << "here";

    return app.exec();
}

#include <QApplication>
#include <QDebug>
#include "../core/folder.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    auto home = Fm::FilePath::homeDir();
    auto folder = Fm::Folder::fromPath(home);

    QObject::connect(folder.get(), &Fm::Folder::startLoading, [=]() {
        qDebug("start loading");
    });
    QObject::connect(folder.get(), &Fm::Folder::finishLoading, [=]() {
        qDebug("finish loading");
    });

    QObject::connect(folder.get(), &Fm::Folder::filesAdded, [=](Fm::FileInfoList& files) {
        qDebug("files added");
        for(auto& item: files) {
            qDebug() << item->displayName();
        }
    });
    QObject::connect(folder.get(), &Fm::Folder::filesRemoved, [=](Fm::FileInfoList& files) {
        qDebug("files removed");
        for(auto& item: files) {
            qDebug() << item->displayName();
        }
    });
    QObject::connect(folder.get(), &Fm::Folder::filesChanged, [=](std::vector<Fm::FileInfoPair>& file_pairs) {
        qDebug("files changed");
        for(auto& pair: file_pairs) {
            auto& item = pair.second;
            qDebug() << item->displayName();
        }
    });

    for(auto& item: folder->files()) {
        qDebug() << item->displayName();
    }
    qDebug() << "here";

    return app.exec();
}

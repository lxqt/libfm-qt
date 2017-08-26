#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QDebug>
#include "../core/folder.h"
#include "../foldermodel.h"
#include "../cachedfoldermodel.h"
#include "../proxyfoldermodel.h"
#include "../pathedit.h"
#include "libfmqt.h"

#include <QTemporaryDir>
#include <QFile>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Fm::LibFmQt contex;

    QTemporaryDir tmp_dir;
    for (int i = 0; i < 3000; ++i) {
        QString path = tmp_dir.path();
        path += "/empty_file_created_for_test";
        path += QString::number(i);
        path += ".txt";
        QFile f{path};
        f.open(QIODevice::WriteOnly);
    }
    qDebug() << "test dir:" << tmp_dir.path() << " created";

    auto home = Fm::FilePath::fromLocalPath(tmp_dir.path().toLocal8Bit().constData());
    Fm::CachedFolderModel* model = Fm::CachedFolderModel::modelFromPath(home);
    auto proxy_model = new Fm::ProxyFolderModel();
    proxy_model->sort(Fm::FolderModel::ColumnFileName, Qt::AscendingOrder);
    proxy_model->setSourceModel(model);

    proxy_model->setThumbnailSize(64);
    proxy_model->setShowThumbnails(true);

    QObject::connect(model->folder().get(), &Fm::Folder::finishLoading,
        [&]() {
            auto n_files = proxy_model->rowCount();
            auto folder = model->folder();
            if(n_files > 0) {
                for(int i = 0; i < n_files; ++i) {
                    std::string name = "empty_file_created_for_test";
                    name += std::to_string(i);
                    name += ".txt";
                    auto file = folder->fileByName(name.c_str());
                    //qDebug() << "found:" << file.get();
                    if(file) {
                        auto src_idx = model->indexFromFileInfo(file.get());
                        if(src_idx.isValid()) {
                            auto idx = proxy_model->mapFromSource(src_idx);
                            qDebug() << idx.row();
                        }
                    }

                    // auto idx = proxy_model->index(i, 0);
                    // auto fpath = home.child(name.c_str());
                    // auto file = proxy_model->fileInfoFromPath(fpath);
                }
            }
            app.quit();
        }
    );

    return app.exec();
}

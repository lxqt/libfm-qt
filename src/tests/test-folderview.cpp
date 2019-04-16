#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QDebug>
#include "../core/folder.h"
#include "../foldermodel.h"
#include "../folderview.h"
#include "../cachedfoldermodel.h"
#include "../proxyfoldermodel.h"
#include "../pathedit.h"
#include "libfmqt.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Fm::LibFmQt contex;
    QMainWindow win;

    Fm::FolderView folder_view;
    win.setCentralWidget(&folder_view);

    auto home = Fm::FilePath::homeDir();
    Fm::CachedFolderModel* model = Fm::CachedFolderModel::modelFromPath(home);
    auto proxy_model = new Fm::ProxyFolderModel();
    proxy_model->sort(Fm::FolderModel::ColumnFileName, Qt::AscendingOrder);
    proxy_model->setSourceModel(model);

    proxy_model->setThumbnailSize(64);
    proxy_model->setShowThumbnails(true);

    folder_view.setModel(proxy_model);

    QToolBar toolbar;
    win.addToolBar(Qt::TopToolBarArea, &toolbar);
    Fm::PathEdit edit;
    edit.setText(home.toString().get());
    toolbar.addWidget(&edit);
    auto action = new QAction(QStringLiteral("Go"), nullptr);
    toolbar.addAction(action);
    QObject::connect(action, &QAction::triggered, [&]() {
        auto path = Fm::FilePath::fromPathStr(edit.text().toLocal8Bit().constData());
        auto new_model = Fm::CachedFolderModel::modelFromPath(path);
        proxy_model->setSourceModel(new_model);
    });

    win.show();
    return app.exec();
}

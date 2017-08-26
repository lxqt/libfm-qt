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
#include "../filedialog.h"
#include "libfmqt.h"


int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Fm::LibFmQt contex;

    /*
    QFileDialog dlg0;
    dlg0.setFileMode(QFileDialog::ExistingFiles);

    dlg0.setNameFilters(QStringList() << "Txt (*.txt)");
    QObject::connect(&dlg0, &QFileDialog::currentChanged, [](const QString& path) {
        qDebug() << "currentChanged:" << path;
    });
    QObject::connect(&dlg0, &QFileDialog::fileSelected, [](const QString& path) {
        qDebug() << "fileSelected:" << path;
    });
    QObject::connect(&dlg0, &QFileDialog::filesSelected, [](const QStringList& paths) {
        qDebug() << "filesSelected:" << paths;
    });

    dlg0.exec();
    */

    Fm::FileDialog dlg;
    // dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setFileMode(QFileDialog::ExistingFiles);
    // dlg.setFileMode(QFileDialog::Directory);
    dlg.setNameFilters(QStringList() << "All (*)" << "Text (*.txt)" << "Images (*.gif *.jpeg *.jpg)");

    dlg.exec();
    qDebug() << "selected files:" << dlg.selectedFiles();

    return 0;
}

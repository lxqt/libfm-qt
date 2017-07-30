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
    dlg0.setNameFilters(QStringList() << "Txt (*.txt)");
    dlg0.exec();
*/
    Fm::FileDialog dlg;
    dlg.setNameFilters(QStringList() << "All (*)" << "Text (*.txt)" << "Images (*.gif *.jpeg *.jpg)");

    dlg.exec();

    return 0;
}

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

    Fm::FileDialog dlg;

    dlg.exec();

    return 0;
}

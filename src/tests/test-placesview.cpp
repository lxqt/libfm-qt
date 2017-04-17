#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QDir>
#include <QDebug>
#include "../placesview.h"
#include "libfmqt.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Fm::LibFmQt contex;
    QMainWindow win;

    Fm::PlacesView view;
    win.setCentralWidget(&view);

    win.show();
    return app.exec();
}

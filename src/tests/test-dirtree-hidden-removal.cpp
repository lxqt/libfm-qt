// Self-check: deleted hidden directories must not reappear when showing hidden files.
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QThread>
#include <functional>
#include "dirtreemodel.h"
#include "dirtreeview.h"
#include "libfmqt.h"
#include "core/folder.h"

static void check(bool ok, const char* message) {
    if(!ok) {
        qFatal("%s", message);
    }
}

static void waitFor(const std::function<bool()>& ready) {
    QElapsedTimer timer;
    timer.start();
    while(!ready() && timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    check(ready(), "Timed out waiting for directory monitor/model");
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    Fm::LibFmQt context;
    QTemporaryDir scratch;
    check(scratch.isValid(), "Cannot create scratch directory");
    QDir directory(scratch.path());
    check(directory.mkdir("visible-anchor"), "Cannot create visible anchor");
    check(directory.mkdir(".keep"), "Cannot create surviving hidden directory");

    Fm::DirTreeModel model(nullptr);
    Fm::DirTreeView view(nullptr);
    view.setModel(&model);
    const auto path = Fm::FilePath::fromLocalPath(scratch.path().toUtf8().constData());
    model.addRoots({path});
    waitFor([&]() { return model.rowCount(QModelIndex()) == 1; });
    const auto root = model.index(0, 0, QModelIndex());
    view.expand(root);
    waitFor([&]() { return model.isLoaded(root); });

    auto count = [&](const QString& name) {
        int found = 0;
        for(int row = 0; row < model.rowCount(root); ++row) {
            if(model.dispName(model.index(row, 0, root)) == name) {
                ++found;
            }
        }
        return found;
    };
    auto folder = Fm::Folder::fromPath(path);
    int additions = 0;
    int removals = 0;
    QObject::connect(folder.get(), &Fm::Folder::filesAdded, &model,
                     [&](Fm::FileInfoList) { ++additions; });
    QObject::connect(folder.get(), &Fm::Folder::filesRemoved, &model,
                     [&](Fm::FileInfoList) { ++removals; });

    auto churn = [&](const QString& name) {
        const int oldAdditions = additions;
        const int oldRemovals = removals;
        check(directory.mkdir(name), "Cannot create test directory");
        waitFor([&]() { return additions > oldAdditions; });
        check(directory.rmdir(name), "Cannot delete test directory");
        waitFor([&]() { return removals > oldRemovals; });
    };

    for(int cycle = 0; cycle < 3; ++cycle) {
        churn(".fictional-config.lock");
    }
    model.setShowHidden(true);
    check(count(".fictional-config.lock") == 0, "Deleted hidden directories became ghost rows");
    check(count(".keep") == 1 && count("visible-anchor") == 1,
          "Unrelated surviving directories were lost");

    // Exercise both the existing visible deletion path and previously visible hidden items.
    churn("visible-temporary");
    check(count("visible-temporary") == 0, "Visible deletion regressed");
    model.setShowHidden(false);
    const int oldRemovals = removals;
    check(directory.rmdir(".keep"), "Cannot delete previously visible hidden directory");
    waitFor([&]() { return removals > oldRemovals; });
    model.setShowHidden(true);
    check(count(".keep") == 0, "Previously visible hidden directory became a ghost");
    check(count("visible-anchor") == 1, "Visible anchor disappeared");
    qInfo("PASS: hidden churn, surviving rows, visible deletion, hide/delete/show");
    return 0;
}

#include "filedialoghelper.h"

#include "libfmqt.h"
#include "filedialog.h"

#include <QWindow>
#include <QDebug>
#include <QTimer>
#include <QSettings>
#include <QtGlobal>

#include <memory>

namespace Fm {

inline static const QString viewModeToString(Fm::FolderView::ViewMode value);
inline static Fm::FolderView::ViewMode viewModeFromString(const QString& str);

FileDialogHelper::FileDialogHelper() {
    // can only be used after libfm-qt initialization
    dlg_ = std::unique_ptr<Fm::FileDialog>(new Fm::FileDialog());
    connect(dlg_.get(), &Fm::FileDialog::accepted, [this]() {
        saveSettings();
        accept();
    });
    connect(dlg_.get(), &Fm::FileDialog::rejected, [this]() {
        saveSettings();
        reject();
    });

    connect(dlg_.get(), &Fm::FileDialog::fileSelected, this, &FileDialogHelper::fileSelected);
    connect(dlg_.get(), &Fm::FileDialog::filesSelected, this, &FileDialogHelper::filesSelected);
    connect(dlg_.get(), &Fm::FileDialog::currentChanged, this, &FileDialogHelper::currentChanged);
    connect(dlg_.get(), &Fm::FileDialog::directoryEntered, this, &FileDialogHelper::directoryEntered);
    connect(dlg_.get(), &Fm::FileDialog::filterSelected, this, &FileDialogHelper::filterSelected);
}

FileDialogHelper::~FileDialogHelper() {
}

void FileDialogHelper::exec() {
    dlg_->exec();
}

bool FileDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow* parent) {
    dlg_->setAttribute(Qt::WA_NativeWindow, true); // without this, sometimes windowHandle() will return nullptr

    dlg_->setWindowFlags(windowFlags);
    dlg_->setWindowModality(windowModality);

    // Reference: KDE implementation
    // https://github.com/KDE/plasma-integration/blob/master/src/platformtheme/kdeplatformfiledialoghelper.cpp
    dlg_->windowHandle()->setTransientParent(parent);

    applyOptions();

    loadSettings();
    // central positioning with respect to the parent window
    if(parent && parent->isVisible()) {
        dlg_->move(parent->x() + (parent->width() - dlg_->width()) / 2,
                   parent->y() + (parent->height() - dlg_->height()) / 2);
    }

    // NOTE: the timer here is required as a workaround borrowed from KDE. Without this, the dialog UI will be blocked.
    // QFileDialog calls our platform plugin to show our own native file dialog instead of showing its widget.
    // However, it still creates a hidden dialog internally, and then make it modal.
    // So user input from all other windows that are not the children of the QFileDialog widget will be blocked.
    // This includes our own dialog. After the return of this show() method, QFileDialog creates its own window and
    // then make it modal, which blocks our UI. The timer schedule a delayed popup of our file dialog, so we can
    // show again after QFileDialog and override the modal state. Then our UI can be unblocked.
    QTimer::singleShot(0, dlg_.get(), &QDialog::show);
    dlg_->setFocus();
    return true;
}

void FileDialogHelper::hide() {
    dlg_->hide();
}

bool FileDialogHelper::defaultNameFilterDisables() const {
    return false;
}

void FileDialogHelper::setDirectory(const QUrl& directory) {
    dlg_->setDirectory(directory);
}

QUrl FileDialogHelper::directory() const {
    return dlg_->directory();
}

void FileDialogHelper::selectFile(const QUrl& filename) {
    dlg_->selectFile(filename);
}

QList<QUrl> FileDialogHelper::selectedFiles() const {
    return dlg_->selectedFiles();
}

void FileDialogHelper::setFilter() {
    // FIXME: what's this?
    // The gtk+ 3 file dialog helper in Qt5 update options in this method.
    applyOptions();
}

void FileDialogHelper::selectNameFilter(const QString& filter) {
    dlg_->selectNameFilter(filter);
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
QString FileDialogHelper::selectedMimeTypeFilter() const {
    return dlg_->selectedMimeTypeFilter();
}

void FileDialogHelper::selectMimeTypeFilter(const QString& filter) {
    dlg_->selectMimeTypeFilter(filter);
}
#endif

QString FileDialogHelper::selectedNameFilter() const {
    return dlg_->selectedNameFilter();
}

bool FileDialogHelper::isSupportedUrl(const QUrl& url) const {
    return dlg_->isSupportedUrl(url);
}

void FileDialogHelper::applyOptions() {
    auto& opt = options();

    // set title
    if(opt->windowTitle().isEmpty()) {
        dlg_->setWindowTitle(opt->acceptMode() == QFileDialogOptions::AcceptOpen ? tr("Open File")
                                                                                 : tr("Save File"));
    }
    else {
        dlg_->setWindowTitle(opt->windowTitle());
    }

    dlg_->setFilter(opt->filter());
    dlg_->setFileMode(QFileDialog::FileMode(opt->fileMode()));
    dlg_->setAcceptMode(QFileDialog::AcceptMode(opt->acceptMode())); // also sets a default label for accept button
    // bool useDefaultNameFilters() const;
    dlg_->setNameFilters(opt->nameFilters());
    if(!opt->mimeTypeFilters().empty()) {
        dlg_->setMimeTypeFilters(opt->mimeTypeFilters());
    }

    dlg_->setDefaultSuffix(opt->defaultSuffix());
    // QStringList history() const;

    // explicitly set labels
    for(int i = 0; i < QFileDialogOptions::DialogLabelCount; ++i) {
        auto label = static_cast<QFileDialogOptions::DialogLabel>(i);
        if(opt->isLabelExplicitlySet(label)) {
            dlg_->setLabelText(static_cast<QFileDialog::DialogLabel>(label), opt->labelText(label));
        }
    }

    auto url = opt->initialDirectory();
    if(url.isValid()) {
        dlg_->setDirectory(url);
    }


#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    auto filter = opt->initiallySelectedMimeTypeFilter();
    if(!filter.isEmpty()) {
        selectMimeTypeFilter(filter);
    }
    else {
        filter = opt->initiallySelectedNameFilter();
        if(!filter.isEmpty()) {
            selectNameFilter(opt->initiallySelectedNameFilter());
        }
    }
#else
    auto filter = opt->initiallySelectedNameFilter();
    if(!filter.isEmpty()) {
        selectNameFilter(filter);
    }
#endif

    auto selectedFiles = opt->initiallySelectedFiles();
    for(const auto& selectedFile: selectedFiles) {
        selectFile(selectedFile);
    }
    // QStringList supportedSchemes() const;
}

static const QString viewModeToString(Fm::FolderView::ViewMode value) {
    QString ret;
    switch(value) {
    case Fm::FolderView::DetailedListMode:
    default:
        ret = QLatin1String("Detailed");
        break;
    case Fm::FolderView::CompactMode:
        ret = QLatin1String("Compact");
        break;
    case Fm::FolderView::IconMode:
        ret = QLatin1String("Icon");
        break;
    case Fm::FolderView::ThumbnailMode:
        ret = QLatin1String("Thumbnail");
        break;
    }
    return ret;
}

Fm::FolderView::ViewMode viewModeFromString(const QString& str) {
    Fm::FolderView::ViewMode ret;
    if(str == QLatin1String("Detailed")) {
        ret = Fm::FolderView::DetailedListMode;
    }
    else if(str == QLatin1String("Compact")) {
        ret = Fm::FolderView::CompactMode;
    }
    else if(str == QLatin1String("Icon")) {
        ret = Fm::FolderView::IconMode;
    }
    else if(str == QLatin1String("Thumbnail")) {
        ret = Fm::FolderView::ThumbnailMode;
    }
    else {
        ret = Fm::FolderView::DetailedListMode;
    }
    return ret;
}

void FileDialogHelper::loadSettings() {
    QSettings settings(QSettings::UserScope, QStringLiteral("lxqt"), QStringLiteral("filedialog"));
    settings.beginGroup (QStringLiteral("Sizes"));
    dlg_->resize(settings.value(QStringLiteral("WindowSize"), QSize(700, 500)).toSize());
    dlg_->setSplitterPos(settings.value(QStringLiteral("SplitterPos"), 200).toInt());
    settings.endGroup();

   settings.beginGroup (QStringLiteral("View"));
   dlg_->setViewMode(viewModeFromString(settings.value(QStringLiteral("Mode"), QStringLiteral("Detailed")).toString()));
   settings.endGroup();
}

void FileDialogHelper::saveSettings() {
    QSettings settings(QSettings::UserScope, QStringLiteral("lxqt"), QStringLiteral("filedialog"));
    settings.beginGroup (QStringLiteral("Sizes"));
    QSize windowSize = dlg_->size();
    if(settings.value(QStringLiteral("WindowSize")) != windowSize) { // no redundant write
        settings.setValue(QStringLiteral("WindowSize"), windowSize);
    }
    int splitterPos = dlg_->splitterPos();
    if(settings.value(QStringLiteral("SplitterPos")) != splitterPos) {
        settings.setValue(QStringLiteral("SplitterPos"), splitterPos);
    }
    settings.endGroup();

    settings.beginGroup (QStringLiteral("View"));
    QString mode = viewModeToString(dlg_->viewMode());
    if(settings.value(QStringLiteral("Mode")) != mode) {
        settings.setValue(QStringLiteral("Mode"), mode);
    }
    settings.endGroup();
}

/*
FileDialogPlugin::FileDialogPlugin() {

}

QPlatformFileDialogHelper *FileDialogPlugin::createHelper() {
    return new FileDialogHelper();
}
*/

} // namespace Fm


QPlatformFileDialogHelper *createFileDialogHelper() {
    // When a process has this environment set, that means glib event loop integration is disabled.
    // In this case, libfm just won't work. So let's disable the file dialog helper and return nullptr.
    if(qgetenv("QT_NO_GLIB") == "1") {
        return nullptr;
    }

    static std::unique_ptr<Fm::LibFmQt> libfmQtContext_;
    if(!libfmQtContext_) {
        // initialize libfm-qt only once
        libfmQtContext_ = std::unique_ptr<Fm::LibFmQt>{new Fm::LibFmQt()};
    }
    return new Fm::FileDialogHelper{};
}

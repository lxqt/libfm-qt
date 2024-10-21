#include "filedialoghelper.h"

#include "libfmqt.h"
#include "filedialog.h"

#include <QCoreApplication>
#include <QWindow>
#include <QDebug>
#include <QTimer>
#include <QSettings>
#include <QtGlobal>

#include <memory>

using namespace Qt::Literals::StringLiterals;

namespace Fm {

inline static const QString viewModeToString(Fm::FolderView::ViewMode value);
inline static Fm::FolderView::ViewMode viewModeFromString(const QString& str);

inline static const QString sortColumnToString(FolderModel::ColumnId value);
inline static FolderModel::ColumnId sortColumnFromString(const QString str);

inline static const QString sortOrderToString(Qt::SortOrder order);
inline static Qt::SortOrder sortOrderFromString(const QString str);

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

QString FileDialogHelper::selectedMimeTypeFilter() const {
    return dlg_->selectedMimeTypeFilter();
}

void FileDialogHelper::selectMimeTypeFilter(const QString& filter) {
    dlg_->selectMimeTypeFilter(filter);
}

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

    const auto selectedFiles = opt->initiallySelectedFiles();
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
        ret = "Detailed"_L1;
        break;
    case Fm::FolderView::CompactMode:
        ret = "Compact"_L1;
        break;
    case Fm::FolderView::IconMode:
        ret = "Icon"_L1;
        break;
    case Fm::FolderView::ThumbnailMode:
        ret = "Thumbnail"_L1;
        break;
    }
    return ret;
}

Fm::FolderView::ViewMode viewModeFromString(const QString& str) {
    Fm::FolderView::ViewMode ret;
    if(str == "Detailed"_L1) {
        ret = Fm::FolderView::DetailedListMode;
    }
    else if(str == "Compact"_L1) {
        ret = Fm::FolderView::CompactMode;
    }
    else if(str == "Icon"_L1) {
        ret = Fm::FolderView::IconMode;
    }
    else if(str == "Thumbnail"_L1) {
        ret = Fm::FolderView::ThumbnailMode;
    }
    else {
        ret = Fm::FolderView::DetailedListMode;
    }
    return ret;
}

static Fm::FolderModel::ColumnId sortColumnFromString(const QString str) {
    Fm::FolderModel::ColumnId ret;
    if(str == "name"_L1) {
        ret = Fm::FolderModel::ColumnFileName;
    }
    else if(str == "type"_L1) {
        ret = Fm::FolderModel::ColumnFileType;
    }
    else if(str == "size"_L1) {
        ret = Fm::FolderModel::ColumnFileSize;
    }
    else if(str == "mtime"_L1) {
        ret = Fm::FolderModel::ColumnFileMTime;
    }
    else if(str == "crtime"_L1) {
        ret = Fm::FolderModel::ColumnFileCrTime;
    }
    else if(str == "dtime"_L1) {
        ret = Fm::FolderModel::ColumnFileDTime;
    }
    else if(str == "owner"_L1) {
        ret = Fm::FolderModel::ColumnFileOwner;
    }
    else if(str == "group"_L1) {
        ret = Fm::FolderModel::ColumnFileGroup;
    }
    else {
        ret = Fm::FolderModel::ColumnFileName;
    }
    return ret;
}

static const QString sortColumnToString(Fm::FolderModel::ColumnId value) {
    QString ret;
    switch(value) {
    case Fm::FolderModel::ColumnFileName:
    default:
        ret = "name"_L1;
        break;
    case Fm::FolderModel::ColumnFileType:
        ret = "type"_L1;
        break;
    case Fm::FolderModel::ColumnFileSize:
        ret = "size"_L1;
        break;
    case Fm::FolderModel::ColumnFileMTime:
        ret = "mtime"_L1;
        break;
    case Fm::FolderModel::ColumnFileCrTime:
        ret = "crtime"_L1;
        break;
    case Fm::FolderModel::ColumnFileDTime:
        ret = "dtime"_L1;
        break;
    case Fm::FolderModel::ColumnFileOwner:
        ret = "owner"_L1;
        break;
    case Fm::FolderModel::ColumnFileGroup:
        ret = "group"_L1;
        break;
    }
    return ret;
}

static const QString sortOrderToString(Qt::SortOrder order) {
    return (order == Qt::DescendingOrder ? "descending"_L1 : "ascending"_L1);
}

static Qt::SortOrder sortOrderFromString(const QString str) {
    return (str == "descending"_L1 ? Qt::DescendingOrder : Qt::AscendingOrder);
}

void FileDialogHelper::loadSettings() {
    QSettings settings(QSettings::UserScope, "lxqt"_L1, "filedialog"_L1);
    settings.beginGroup ("Sizes"_L1);
    dlg_->resize(settings.value("WindowSize"_L1, QSize(700, 500)).toSize());
    dlg_->setSplitterPos(settings.value("SplitterPos"_L1, 200).toInt());
    settings.endGroup();

    settings.beginGroup ("View"_L1);
    dlg_->setViewMode(viewModeFromString(settings.value("Mode"_L1, "Detailed"_L1).toString()));
    dlg_->sort(sortColumnFromString(settings.value("SortColumn"_L1).toString()), sortOrderFromString(settings.value("SortOrder"_L1).toString()));
    dlg_->setSortFolderFirst(settings.value("SortFolderFirst"_L1, true).toBool());
    dlg_->setSortHiddenLast(settings.value("SortHiddenLast"_L1, false).toBool());
    dlg_->setSortCaseSensitive(settings.value("SortCaseSensitive"_L1, false).toBool());
    dlg_->setShowHidden(settings.value("ShowHidden"_L1, false).toBool());

    dlg_->setShowThumbnails(settings.value("ShowThumbnails"_L1, true).toBool());
    dlg_->setNoItemTooltip(settings.value("NoItemTooltip"_L1, false).toBool());
    dlg_->setScrollPerPixel(settings.value("ScrollPerPixel"_L1, true).toBool());

    dlg_->setBigIconSize(settings.value("BigIconSize"_L1, 48).toInt());
    dlg_->setSmallIconSize(settings.value("SmallIconSize"_L1, 24).toInt());
    dlg_->setThumbnailIconSize(settings.value("ThumbnailIconSize"_L1, 128).toInt());

    const QList<QVariant> hiddenColumns = settings.value("HiddenColumns"_L1).toList();
    QList<int> l;
    for(auto width : hiddenColumns) {
        l << width.toInt();
    }
    dlg_->setHiddenColumns(l);
    settings.endGroup();

    settings.beginGroup("Places"_L1);
    QStringList hiddenPlacesList = settings.value("HiddenPlaces"_L1).toStringList();
    QSet<QString> hiddenPlacesSet = QSet<QString>(hiddenPlacesList.begin(), hiddenPlacesList.end());
    dlg_->setHiddenPlaces(hiddenPlacesSet);
    settings.endGroup();
}

// This also prevents redundant writings whenever a file dialog is closed without a change in its settings.
void FileDialogHelper::saveSettings() {
    QSettings settings(QSettings::UserScope, "lxqt"_L1, "filedialog"_L1);
    settings.beginGroup ("Sizes"_L1);
    QSize windowSize = dlg_->size();
    if(settings.value("WindowSize"_L1) != windowSize) { // no redundant write
        settings.setValue("WindowSize"_L1, windowSize);
    }
    int splitterPos = dlg_->splitterPos();
    if(settings.value("SplitterPos"_L1) != splitterPos) {
        settings.setValue("SplitterPos"_L1, splitterPos);
    }
    settings.endGroup();

    settings.beginGroup ("View"_L1);
    QString mode = viewModeToString(dlg_->viewMode());
    if(settings.value("Mode"_L1) != mode) {
        settings.setValue("Mode"_L1, mode);
    }
    QString sortColumn = sortColumnToString(static_cast<Fm::FolderModel::ColumnId>(dlg_->sortColumn()));
    if(settings.value("SortColumn"_L1) != sortColumn) {
        settings.setValue("SortColumn"_L1, sortColumn);
    }
    QString sortOrder = sortOrderToString(dlg_->sortOrder());
    if(settings.value("SortOrder"_L1) != sortOrder) {
        settings.setValue("SortOrder"_L1, sortOrder);
    }
    bool sortFolderFirst = dlg_->sortFolderFirst();
    if(settings.value("SortFolderFirst"_L1).toBool() != sortFolderFirst) {
        settings.setValue("SortFolderFirst"_L1, sortFolderFirst);
    }
    bool sortHiddenLast = dlg_->sortHiddenLast();
    if(settings.value("SortHiddenLast"_L1).toBool() != sortHiddenLast) {
        settings.setValue("SortHiddenLast"_L1, sortHiddenLast);
    }
    bool sortCaseSensitive = dlg_->sortCaseSensitive();
    if(settings.value("SortCaseSensitive"_L1).toBool() != sortCaseSensitive) {
        settings.setValue("SortCaseSensitive"_L1, sortCaseSensitive);
    }
    bool showHidden = dlg_->showHidden();
    if(settings.value("ShowHidden"_L1).toBool() != showHidden) {
        settings.setValue("ShowHidden"_L1, showHidden);
    }

    bool showThumbnails = dlg_->showThumbnails();
    if(settings.value("ShowThumbnails"_L1).toBool() != showThumbnails) {
        settings.setValue("ShowThumbnails"_L1, showThumbnails);
    }
    bool noItemTooltip = dlg_->noItemTooltip();
    if(settings.value("NoItemTooltip"_L1).toBool() != noItemTooltip) {
        settings.setValue("NoItemTooltip"_L1, noItemTooltip);
    }
    bool scrollPerPixel = dlg_->scrollPerPixel();
    if(settings.value("ScrollPerPixel"_L1).toBool() != scrollPerPixel) {
        settings.setValue("ScrollPerPixel"_L1, scrollPerPixel);
    }

    int size = dlg_->bigIconSize();
    if(settings.value("BigIconSize"_L1).toInt() != size) {
        settings.setValue("BigIconSize"_L1, size);
    }
    size = dlg_->smallIconSize();
    if(settings.value("SmallIconSize"_L1).toInt() != size) {
        settings.setValue("SmallIconSize"_L1, size);
    }
    size = dlg_->thumbnailIconSize();
    if(settings.value("ThumbnailIconSize"_L1).toInt() != size) {
        settings.setValue("ThumbnailIconSize"_L1, size);
    }

    QList<int> columns = dlg_->getHiddenColumns();
    std::sort(columns.begin(), columns.end());
    QList<QVariant> hiddenColumns;
    for(int i = 0; i < columns.size(); ++i) {
        hiddenColumns << QVariant(columns.at(i));
    }
    if(settings.value("HiddenColumns"_L1).toList() != hiddenColumns) {
        settings.setValue("HiddenColumns"_L1, hiddenColumns);
    }
    settings.endGroup();

    settings.beginGroup("Places"_L1);
    QSet<QString> hiddenPlaces = dlg_->getHiddenPlaces();
    if(hiddenPlaces.isEmpty()) { // don't save "@Invalid()"
        settings.remove("HiddenPlaces"_L1);
    }
    else {
        QStringList hiddenPlacesList = settings.value("HiddenPlaces"_L1).toStringList();
        QSet<QString> hiddenPlacesSet = QSet<QString>(hiddenPlacesList.begin(), hiddenPlacesList.end());
        if (hiddenPlaces != hiddenPlacesSet) {
            QStringList sl(hiddenPlaces.begin(), hiddenPlaces.end());
            settings.setValue("HiddenPlaces"_L1, sl);
        }
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
        // add translations
        QCoreApplication::installTranslator(libfmQtContext_.get()->translator());
    }
    return new Fm::FileDialogHelper{};
}

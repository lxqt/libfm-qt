#include "filedialog.h"
#include "cachedfoldermodel.h"
#include "proxyfoldermodel.h"
#include "utilities.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QMimeType>
#include <QMimeDatabase>
#include <QDebug>

namespace Fm {


FileDialog::FileDialog(QWidget* parent, FilePath path) :
    QDialog(parent),
    folderModel_{nullptr},
    proxyModel_{nullptr},
    options_{0},
    isLabelExplicitlySetMask_{0},
    modelFilter_{this} {

    ui.setupUi(this);

    // path bar
    connect(ui.location, &PathBar::chdir, this, &FileDialog::setDirectoryPath);

    // side pane
    ui.sidePane->setMode(Fm::SidePane::ModePlaces);
    connect(ui.sidePane, &SidePane::chdirRequested, [this](int type, const FilePath &path) {
        setDirectoryPath(path);
    });

    // folder view
    proxyModel_ = new ProxyFolderModel(this);
    proxyModel_->sort(FolderModel::ColumnFileName, Qt::AscendingOrder);
    proxyModel_->setThumbnailSize(64);
    proxyModel_->setShowThumbnails(true);

    proxyModel_->addFilter(&modelFilter_);

    ui.folderView->setViewMode(Fm::FolderView::DetailedListMode);
    connect(ui.folderView, &FolderView::clicked, [this](int type, const std::shared_ptr<const Fm::FileInfo>& file){
        if(file && type == FolderView::ActivatedClick) {
            setDirectoryPath(file->path());
        }
    });
    ui.folderView->setModel(proxyModel_);

    // file type
    connect(ui.fileTypeCombo, &QComboBox::currentTextChanged, [this](const QString& text) {
        selectNameFilter(text);
    });
    // default filename pattern
    setNameFilters(QStringList() << tr("All Files (*)"));
    ui.fileTypeCombo->setCurrentIndex(0);

    if(path.isValid()) {
        setDirectoryPath(path);
        directoryPath_ = std::move(path);
    }
}

void FileDialog::setDirectory(const QUrl &directory) {
    auto path = Fm::FilePath::fromUri(directory.toEncoded().constData());
    setDirectoryPath(path);
}

// interface for QPlatformFileDialogHelper

void FileDialog::setDirectoryPath(FilePath directory) {
    ui.location->setPath(directory);
    ui.sidePane->chdir(directory);
    ui.folderView->model();

    auto oldModel = folderModel_;
    folderModel_ = Fm::CachedFolderModel::modelFromPath(directory);
    proxyModel_->setSourceModel(folderModel_);

    if(oldModel != nullptr) {
        // FIXME: is this correct?
        oldModel->unref();
    }
    directoryPath_ = std::move(directory);
}

QUrl FileDialog::directory() const {
    QUrl url{directoryPath_.uri().get()};
    return url;
}

void FileDialog::selectFile(const QUrl& filename) {
    auto urlStr = filename.toEncoded();
    auto path = FilePath::fromUri(urlStr.constData());
    auto idx = proxyModel_->indexFromPath(path);

    // FIXME: add a method to Fm::FolderView to select files

    // FIXME: need to add this for detailed list
    //int flags = QItemSelectionModel::Rows;
    ui.folderView->selectionModel()->select(idx, QItemSelectionModel::Select);
}

QList<QUrl> FileDialog::selectedFiles() {
    QList<QUrl> urls;
    auto selFiles = ui.folderView->selectedFilePaths();
    for(auto& path: selFiles) {
        urls.append(QUrl(path.uri().get()));
    }
    return urls;
}

void FileDialog::setFilter() {
    // FIXME: what's this?
}

void FileDialog::selectNameFilter(const QString& filter) {
    if(filter != currentNameFilter_) {
        currentNameFilter_ = filter;
        ui.fileTypeCombo->setCurrentText(filter);

        modelFilter_.update();
        proxyModel_->invalidate();
    }
}


bool FileDialog::isSupportedUrl(const QUrl& url) {
    auto scheme = url.scheme().toLocal8Bit();
    // FIXME: this is not reliable due to the bug of gvfs.
    return Fm::isUriSchemeSupported(scheme.constData());
}


// options

void FileDialog::setFilter(QDir::Filters filters) {
    filters_ = filters;
    // TODO:
}

void FileDialog::setViewMode(QFileDialog::ViewMode mode) {
    viewMode_ = mode;

    switch(mode) {
    case QFileDialog::Detail:
        ui.folderView->setViewMode(FolderView::DetailedListMode);
        break;
    case QFileDialog::List:
        ui.folderView->setViewMode(FolderView::CompactMode);
        break;
    default:
        break;
    }
}


void FileDialog::setFileMode(QFileDialog::FileMode mode) {
    fileMode_ = mode;
    // TODO:
}


void FileDialog::setAcceptMode(QFileDialog::AcceptMode mode) {
    acceptMode_ = mode;
    // TODO:
}


void FileDialog::setNameFilters(const QStringList& filters) {
    nameFilters_ = filters;
    ui.fileTypeCombo->clear();
    ui.fileTypeCombo->addItems(filters);
}


void FileDialog::setMimeTypeFilters(const QStringList& filters) {
    mimeTypeFilters_ = filters;

    QStringList nameFilters;
    QMimeDatabase db;
    for(const auto& filter: filters) {
        auto mimeType = db.mimeTypeForName(filter);
        auto nameFilter = mimeType.comment();
        if(!mimeType.suffixes().empty()) {
            nameFilter + " (";
            for(const auto& suffix: mimeType.suffixes()) {
                nameFilter += "*.";
                nameFilter += suffix;
                nameFilter += ' ';
            }
            nameFilter[nameFilter.length() - 1] = ')';
        }
        nameFilters << nameFilter;
    }
    setNameFilters(nameFilters);
}


void FileDialog::setLabelText(QFileDialog::DialogLabel label, const QString& text) {
    switch(label) {
    case QFileDialog::LookIn:
        ui.lookInLabel->setText(text);
        break;
    case QFileDialog::FileName:
        ui.fileNameLabel->setText(text);
        break;
    case QFileDialog::FileType:
        ui.fileTypeLabel->setText(text);
        break;
    case QFileDialog::Accept:
        ui.buttonBox->button(QDialogButtonBox::Ok)->setText(text);
        break;
    case QFileDialog::Reject:
        ui.buttonBox->button(QDialogButtonBox::Cancel)->setText(text);
        break;
    default:
        break;
    }
    isLabelExplicitlySetMask_ |= label;
}

QString FileDialog::labelText(QFileDialog::DialogLabel label) const {
    QString text;
    switch(label) {
    case QFileDialog::LookIn:
        text = ui.lookInLabel->text();
        break;
    case QFileDialog::FileName:
        text = ui.fileNameLabel->text();
        break;
    case QFileDialog::FileType:
        text = ui.fileTypeLabel->text();
        break;
    case QFileDialog::Accept:
        ui.buttonBox->button(QDialogButtonBox::Ok)->text();
        break;
    case QFileDialog::Reject:
        ui.buttonBox->button(QDialogButtonBox::Cancel)->text();
        break;
    default:
        break;
    }
    return text;
}

bool FileDialog::isLabelExplicitlySet(QFileDialog::DialogLabel label) {
    return (isLabelExplicitlySetMask_ & label) != 0;
}

bool FileDialog::FileDialogFilter::filterAcceptsRow(const ProxyFolderModel *model, const std::shared_ptr<const FileInfo> &info) const {
    if(dlg_->fileMode_ & QFileDialog::Directory) {
        // we only want to select directories
        if(!info->isDir() && dlg_->options_& QFileDialog::ShowDirsOnly) { // not a dir
            return false;
        }
    }
    else {
        // we want to select files, so all directories can be shown regardless of their names
        if(info->isDir()) {
            return true;
        }
    }

    bool nameMatched = false;
    auto& name = info->displayName();
    for(const auto& pattern: patterns_) {
        if(pattern.exactMatch(name)) {
            nameMatched = true;
            break;
        }
    }
    return nameMatched;
}

void FileDialog::FileDialogFilter::update() {
    // update filename pattersn
    patterns_.clear();
    QString nameFilter = dlg_->currentNameFilter_;
    // if the filter contains (...), only get the part between the parenthesis.
    auto left = nameFilter.indexOf('(');
    if(left != -1) {
        ++left;
        auto right = nameFilter.indexOf(')', left);
        if(right == -1) {
            right = nameFilter.length();
        }
        nameFilter = nameFilter.mid(left, right - left);
    }
    // parse the "*.ext1 *.ext2 *.ext3 ..." list into QRegExp objects
    auto globs = nameFilter.simplified().split(' ');
    for(const auto& glob: globs) {
        patterns_.emplace_back(QRegExp(glob, Qt::CaseInsensitive, QRegExp::Wildcard));
    }
}

#if 0

QUrl FileDialog::initialDirectory() const {
    return QUrl();
}

void FileDialog::setInitialDirectory(const QUrl& directory) {

}

QString FileDialog::initiallySelectedNameFilter() const {

}

void FileDialog::setInitiallySelectedNameFilter(const QString&filter) {

}

QList<QUrl> FileDialog::initiallySelectedFiles() const {

}

void FileDialog::setInitiallySelectedFiles(const QList<QUrl>& fileUrls) {

}

#endif

} // namespace Fm

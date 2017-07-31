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
    viewMode_{QFileDialog::Detail},
    fileMode_{QFileDialog::AnyFile},
    acceptMode_{QFileDialog::AcceptOpen},
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
    connect(ui.folderView, &FolderView::clicked, this, &FileDialog::onFileClicked);
    ui.folderView->setModel(proxyModel_);
    // update selection mode for the view
    updateSelectionMode();

    // selection changes
    connect(ui.folderView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &FileDialog::onCurrentRowChanged);
    connect(ui.folderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FileDialog::onSelectionChanged);

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

void FileDialog::accept() {
    // handle selected filenames
    selectedFiles_.clear();
    // parse the file names from the text entry
    QStringList parsedNames;
    auto fileNames = ui.fileName->text();
    // check if there are multiple file names (containing ")
    auto firstQuote = fileNames.indexOf('\"');
    auto lastQuote = fileNames.lastIndexOf('\"');
    if(firstQuote != -1 && lastQuote != -1) {
        // split the names
        QRegExp sep{"\"\\s+\""};  // separated with " "
        parsedNames = fileNames.mid(firstQuote, lastQuote - firstQuote).split(sep);
    }
    else {
        parsedNames << fileNames;
    }

    // get full paths for the filenames and convert them to URLs
    for(auto& name: parsedNames) {
        // add default filename extension as needed
        if(!defaultSuffix_.isEmpty() && name.lastIndexOf('.') == -1) {
            name += '.';
            name += defaultSuffix_;
        }
        auto fullPath = directoryPath_.child(name.toLocal8Bit().constData());
        selectedFiles_.append(QUrl::fromEncoded(fullPath.uri().get()));
    }

    Q_EMIT filesSelected(selectedFiles_);

    if(selectedFiles_.size() == 1) {
        Q_EMIT fileSelected(selectedFiles_[0]);
    }

    QDialog::accept();
}

void FileDialog::reject() {
    QDialog::reject();
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

    QUrl uri = QUrl::fromEncoded(directory.uri().get());
    Q_EMIT directoryEntered(uri);
}

void FileDialog::selectFilePath(const FilePath &path) {
    auto idx = proxyModel_->indexFromPath(path);

    // FIXME: add a method to Fm::FolderView to select files

    // FIXME: need to add this for detailed list
    QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Select;
    if(viewMode_ == QFileDialog::Detail) {
        flags |= QItemSelectionModel::Rows;
    }
    ui.folderView->selectionModel()->select(idx, flags);
}

void FileDialog::onCurrentRowChanged(const QModelIndex &current, const QModelIndex &previous) {
    // emit currentChanged signal
    QUrl currentUrl;
    if(current.isValid()) {
        // emit changed siangl for newly selected items
        auto fi = proxyModel_->fileInfoFromIndex(current);
        if(fi) {
            currentUrl = QUrl::fromEncoded(fi->path().uri().get());
        }
    }
    Q_EMIT currentChanged(currentUrl);
}

void FileDialog::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QString fileNames;
    auto selFiles = ui.folderView->selectedFiles();
    for(auto& fileInfo: selFiles) {
        if(fileMode_ == QFileDialog::Directory && !fileInfo->isDir()) {
            // if we want to select dir, ignore selected files
            continue;
        }
        else if(fileInfo->isDir()) {
            // if we want to select files, ignore selected dirs
            continue;
        }

        auto baseName = fileInfo->path().baseName();
        if(fileMode_ == QFileDialog::ExistingFiles) {
            // support multiple selection
            if(!fileNames.isEmpty()) {
                fileNames += ' ';
            }
            // FIXME: use a more reliable way to quote file names.
            // otherwise names with embedded " will break.
            fileNames += '\"';
            fileNames += baseName.get();
            fileNames += '\"';
        }
        else {
            // support single selection only
            fileNames = baseName.get();
            break;
        }
    }
    ui.fileName->setText(fileNames);
}

void FileDialog::onFileClicked(int type, const std::shared_ptr<const FileInfo> &file) {
    bool canAccept = false;
    if(file && type == FolderView::ActivatedClick) {
        if(file->isDir()) {
            if(fileMode_ == QFileDialog::Directory) {
                // select directory and a dir item is activated
                canAccept = true;
            }
            else {
                setDirectoryPath(file->path());
            }
        }
        else if(fileMode_ != QFileDialog::Directory) {
            // select file(s) and a file item is activated
            canAccept = true;
        }
    }

    if(canAccept) {
        selectFilePath(file->path());
        accept();
    }
}

void FileDialog::updateSelectionMode() {
    // enable multiple selection?
    ui.folderView->childView()->setSelectionMode(fileMode_ == QFileDialog::ExistingFiles ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
}

QUrl FileDialog::directory() const {
    QUrl url{directoryPath_.uri().get()};
    return url;
}

void FileDialog::selectFile(const QUrl& filename) {
    auto urlStr = filename.toEncoded();
    auto path = FilePath::fromUri(urlStr.constData());
    selectFilePath(path);
}

QList<QUrl> FileDialog::selectedFiles() {
    return selectedFiles_;
}


void FileDialog::selectNameFilter(const QString& filter) {
    if(filter != currentNameFilter_) {
        currentNameFilter_ = filter;
        ui.fileTypeCombo->setCurrentText(filter);

        modelFilter_.update();
        proxyModel_->invalidate();
        Q_EMIT filterSelected(filter);
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

    // update selection mode for the view
    updateSelectionMode();
}


void FileDialog::setFileMode(QFileDialog::FileMode mode) {
    fileMode_ = mode;

    // enable multiple selection?
    updateSelectionMode();
}


void FileDialog::setAcceptMode(QFileDialog::AcceptMode mode) {
    acceptMode_ = mode;
    // TODO: open or save (default window title)
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


bool FileDialog::FileDialogFilter::filterAcceptsRow(const ProxyFolderModel *model, const std::shared_ptr<const FileInfo> &info) const {
    if(dlg_->fileMode_ & QFileDialog::Directory) {
        // we only want to select directories
        if(!info->isDir()) { // not a dir
            // NOTE: here we ignore dlg_->options_& QFileDialog::ShowDirsOnly option.
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

} // namespace Fm

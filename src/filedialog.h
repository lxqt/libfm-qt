#ifndef FM_FILEDIALOG_H
#define FM_FILEDIALOG_H

#include "libfmqtglobals.h"
#include "core/filepath.h"

#include <QFileDialog>
#include <QRegExp>
#include <vector>
#include <memory>
#include "folderview.h"

namespace Ui {
class FileDialog;
}

namespace Fm {

class CachedFolderModel;
class ProxyFolderModel;

class LIBFM_QT_API FileDialog : public QDialog {
    Q_OBJECT
public:
    explicit FileDialog(QWidget *parent = 0, FilePath path = FilePath::homeDir());

    ~FileDialog();

    // Some QFileDialog compatible interface
    void accept() override;

    void reject() override;

    QFileDialog::Options options() const {
        return options_;
    }

    void setOptions(QFileDialog::Options options) {
        options_ = options;
    }

    // interface for QPlatformFileDialogHelper

    void setDirectory(const QUrl &directory);

    QUrl directory() const;

    void selectFile(const QUrl &filename);

    QList<QUrl> selectedFiles();

    void selectNameFilter(const QString &filter);

    QString selectedNameFilter() const {
        return currentNameFilter_;
    }

    bool isSupportedUrl(const QUrl &url);

    // options

    // not yet supported
    QDir::Filters filter() const {
        return filters_;
    }
    // not yet supported
    void setFilter(QDir::Filters filters);

    void setViewMode(FolderView::ViewMode mode);
    FolderView::ViewMode viewMode() const {
        return viewMode_;
    }

    void setFileMode(QFileDialog::FileMode mode);
    QFileDialog::FileMode fileMode() const {
        return fileMode_;
    }

    void setAcceptMode(QFileDialog::AcceptMode mode);
    QFileDialog::AcceptMode acceptMode() const {
        return acceptMode_;
    }

    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const {
        return nameFilters_;
    }

    void setMimeTypeFilters(const QStringList &filters);
    QStringList mimeTypeFilters() const {
        return mimeTypeFilters_;
    }

    void setDefaultSuffix(const QString &suffix) {
        if(!suffix.isEmpty() && suffix[0] == '.') {
            // if the first char is dot, remove it.
            defaultSuffix_ = suffix.mid(1);
        }
        else {
            defaultSuffix_ = suffix;
        }
    }
    QString defaultSuffix() const {
        return defaultSuffix_;
    }

    void setLabelText(QFileDialog::DialogLabel label, const QString &text);
    QString labelText(QFileDialog::DialogLabel label) const;

private Q_SLOTS:
    void onCurrentRowChanged(const QModelIndex &current, const QModelIndex& /*previous*/);
    void onSelectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/);
    void onFileClicked(int type, const std::shared_ptr<const Fm::FileInfo>& file);
    void onNewFolder();
    void onViewModeToggled(bool active);

Q_SIGNALS:
    // emitted when the dialog is accepted and some files are selected
    void fileSelected(const QUrl &file);
    void filesSelected(const QList<QUrl> &files);

    // emitted whenever selection changes (including no selected files)
    void currentChanged(const QUrl &path);

    void directoryEntered(const QUrl &directory);
    void filterSelected(const QString &filter);

private:

    class FileDialogFilter: public ProxyFolderModelFilter {
    public:
        FileDialogFilter(FileDialog* dlg): dlg_{dlg} {}
        virtual bool filterAcceptsRow(const ProxyFolderModel* /*model*/, const std::shared_ptr<const Fm::FileInfo>& info) const override;
        void update();

        FileDialog* dlg_;
        std::vector<QRegExp> patterns_;

    };

    void selectFilePath(const FilePath& path, bool singleSelection = false);
    void setDirectoryPath(FilePath directory, FilePath selectedPath = FilePath());
    void updateSelectionMode();
    void doAccept();
    void onFileInfoJobFinished();
    void freeFolder();

private:
    std::unique_ptr<Ui::FileDialog> ui;
    CachedFolderModel* folderModel_;
    ProxyFolderModel* proxyModel_;
    FilePath directoryPath_;
    std::shared_ptr<Fm::Folder> folder_;

    QFileDialog::Options options_;
    QDir::Filters filters_;
    FolderView::ViewMode viewMode_;
    QFileDialog::FileMode fileMode_;
    QFileDialog::AcceptMode acceptMode_;
    QStringList nameFilters_;
    QStringList mimeTypeFilters_;
    QString defaultSuffix_;
    FileDialogFilter modelFilter_;
    QString currentNameFilter_;
    QList<QUrl> selectedFiles_;
    // view modes
    QAction* iconViewAction_;
    QAction* thumbnailViewAction_;
    QAction* compactViewAction_;
    QAction* detailedViewAction_;
};


} // namespace Fm
#endif // FM_FILEDIALOG_H

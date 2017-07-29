#ifndef FM_FILEDIALOG_H
#define FM_FILEDIALOG_H

#include "libfmqtglobals.h"
#include "core/filepath.h"
#include "ui_filedialog.h"

#include <QFileDialog>

namespace Fm {


class LIBFM_QT_API FileDialog : public QDialog {
    Q_OBJECT

public:
    explicit FileDialog(QWidget *parent = 0, FilePath path = FilePath::homeDir());

    ~FileDialog() = default;

    // interface for QPlatformFileDialogHelper
    bool defaultNameFilterDisables() const;

    void setDirectory(const QUrl &directory);

    QUrl directory() const;

    void selectFile(const QUrl &filename);

    QList<QUrl> selectedFiles();

    void setFilter();

    void selectNameFilter(const QString &filter);

    QString selectedNameFilter() const;

    bool isSupportedUrl(const QUrl &url);

    // options
    QDir::Filters filter() const;
    void setFilter(QDir::Filters filters);

    void setViewMode(QFileDialog::ViewMode mode);
    QFileDialog::ViewMode viewMode() const;

    void setFileMode(QFileDialog::FileMode mode);
    QFileDialog::FileMode fileMode() const;

    void setAcceptMode(QFileDialog::AcceptMode mode);
    QFileDialog::AcceptMode acceptMode() const;

    void setSidebarUrls(const QList<QUrl> &urls);
    QList<QUrl> sidebarUrls() const;

    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;

    void setMimeTypeFilters(const QStringList &filters);
    QStringList mimeTypeFilters() const;

    void setDefaultSuffix(const QString &suffix);
    QString defaultSuffix() const;

    void setHistory(const QStringList &paths);
    QStringList history() const;

    void setLabelText(QFileDialog::DialogLabel label, const QString &text);
    QString labelText(QFileDialog::DialogLabel label) const;

    bool isLabelExplicitlySet(QFileDialog::DialogLabel label);

    QUrl initialDirectory() const;
    void setInitialDirectory(const QUrl &);

    QString initiallySelectedNameFilter() const;
    void setInitiallySelectedNameFilter(const QString &);

    QList<QUrl> initiallySelectedFiles() const;
    void setInitiallySelectedFiles(const QList<QUrl> &);

Q_SIGNALS:
    void fileSelected(const QUrl &file);
    void filesSelected(const QList<QUrl> &files);
    void currentChanged(const QUrl &path);
    void directoryEntered(const QUrl &directory);
    void filterSelected(const QString &filter);

private:
    Ui::FileDialog ui;
};


} // namespace Fm
#endif // FM_FILEDIALOG_H

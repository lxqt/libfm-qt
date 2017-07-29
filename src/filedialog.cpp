#include "filedialog.h"
#include "cachedfoldermodel.h"

namespace Fm {

FileDialog::FileDialog(QWidget *parent, FilePath path) :
    QDialog(parent) {
    ui.setupUi(this);

    // toolbar
    ui.location->setPath(path);

    // side pane
    ui.sidePane->setMode(Fm::SidePane::ModePlaces);

    // folder view
    Fm::CachedFolderModel* model = Fm::CachedFolderModel::modelFromPath(path);
    auto proxy_model = new Fm::ProxyFolderModel();
    proxy_model->sort(Fm::FolderModel::ColumnFileName, Qt::AscendingOrder);
    proxy_model->setSourceModel(model);

    proxy_model->setThumbnailSize(64);
    proxy_model->setShowThumbnails(true);

    ui.folderView->setViewMode(Fm::FolderView::DetailedListMode);
    ui.folderView->setModel(proxy_model);
}

} // namespace Fm

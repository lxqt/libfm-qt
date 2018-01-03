/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include "editbookmarksdialog.h"
#include "ui_edit-bookmarks.h"
#include <QByteArray>
#include <QUrl>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDir>

namespace Fm {

EditBookmarksDialog::EditBookmarksDialog(FmBookmarks* bookmarks, QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    ui(new Ui::EditBookmarksDialog()),
    bookmarks_(FM_BOOKMARKS(g_object_ref(bookmarks))) {

    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose); // auto delete on close

    // load bookmarks
    GList* allBookmarks = fm_bookmarks_get_all(bookmarks_);
    for(GList* l = allBookmarks; l; l = l->next) {
        FmBookmarkItem* bookmark = reinterpret_cast<FmBookmarkItem*>(l->data);
        QTreeWidgetItem* item = new QTreeWidgetItem();
        char* path_str = fm_path_display_name(bookmark->path, false);
        item->setData(0, Qt::DisplayRole, QString::fromUtf8(bookmark->name));
        item->setData(1, Qt::DisplayRole, QString::fromUtf8(path_str));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
        g_free(path_str);
        ui->treeWidget->addTopLevelItem(item);
    }
    g_list_free_full(allBookmarks, (GDestroyNotify)fm_bookmark_item_unref);

    connect(ui->addItem, &QPushButton::clicked, this, &EditBookmarksDialog::onAddItem);
    connect(ui->removeItem, &QPushButton::clicked, this, &EditBookmarksDialog::onRemoveItem);
}

EditBookmarksDialog::~EditBookmarksDialog() {
    g_object_unref(bookmarks_);
    delete ui;
}

void EditBookmarksDialog::accept() {
    // save bookmarks
    // it's easier to recreate the whole bookmark file than
    // to manipulate FmBookmarks object. So here we generate the file directly.
    // FIXME: maybe in the future we should add a libfm API to easily replace all FmBookmarks.
    // Here we use gtk+ 3.0 bookmarks rather than the gtk+ 2.0 one.
    // Since gtk+ 2.24.12, gtk+2 reads gtk+3 bookmarks file if it exists.
    // So it's safe to only save gtk+3 bookmarks file.
    QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    path += QLatin1String("/gtk-3.0");
    if(!QDir().mkpath(path)) {
        return;    // fail to create ~/.config/gtk-3.0 dir
    }
    path += QLatin1String("/bookmarks");
    QSaveFile file(path); // use QSaveFile for atomic file operation
    if(file.open(QIODevice::WriteOnly)) {
        for(int row = 0; ; ++row) {
            QTreeWidgetItem* item = ui->treeWidget->topLevelItem(row);
            if(!item) {
                break;
            }
            QString name = item->data(0, Qt::DisplayRole).toString();
            QUrl url = QUrl::fromUserInput(item->data(1, Qt::DisplayRole).toString());
            file.write(url.toEncoded());
            file.write(" ");
            file.write(name.toUtf8());
            file.write("\n");
        }
        // FIXME: should we support Qt or KDE specific bookmarks in the future?
        file.commit();
    }
    QDialog::accept();
}

void EditBookmarksDialog::onAddItem() {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setData(0, Qt::DisplayRole, tr("New bookmark"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
    ui->treeWidget->addTopLevelItem(item);
    ui->treeWidget->editItem(item);
}

void EditBookmarksDialog::onRemoveItem() {
    const QList<QTreeWidgetItem*> sels = ui->treeWidget->selectedItems();
    for(QTreeWidgetItem* const item : sels) {
        delete item;
    }
}


} // namespace Fm

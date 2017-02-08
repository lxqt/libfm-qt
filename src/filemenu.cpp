/*
 * Copyright (C) 2012 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#include "filemenu.h"
#include "createnewmenu.h"
#include "icontheme.h"
#include "filepropsdialog.h"
#include "utilities.h"
#include "fileoperation.h"
#include "filelauncher.h"
#include "appchooserdialog.h"
#ifdef CUSTOM_ACTIONS
#include <libfm/fm-actions.h>
#include "customaction_p.h"
#endif
#include <QMessageBox>
#include <QDebug>
#include "filemenu_p.h"

#include "core/compat_p.h"

namespace Fm {

FileMenu::FileMenu(Fm::FileInfoList files, std::shared_ptr<const Fm::FileInfo> info, Fm::FilePath cwd, const QString& title, QWidget* parent):
    QMenu(title, parent),
    files_{std::move(files)},
    info_{std::move(info)},
    cwd_{std::move(cwd)},
    unTrashAction_(nullptr),
    fileLauncher_(nullptr) {

    useTrash_ = true;
    confirmDelete_ = true;
    confirmTrash_ = false; // Confirm before moving files into "trash can"

    openAction_ = nullptr;
    openWithMenuAction_ = nullptr;
    openWithAction_ = nullptr;
    separator1_ = nullptr;
    cutAction_ = nullptr;
    copyAction_ = nullptr;
    pasteAction_ = nullptr;
    deleteAction_ = nullptr;
    unTrashAction_ = nullptr;
    renameAction_ = nullptr;
    separator2_ = nullptr;
    propertiesAction_ = nullptr;

    auto mime_type = info_->mimeType();
    Fm::FilePath path = info_->path();

    // check if the files are of the same type
    sameType_ = files_.isSameType();
    // check if the files are on the same filesystem
    sameFilesystem_ = files_.isSameFilesystem();
    // check if the files are all virtual

    // FIXME: allVirtual_ = sameFilesystem_ && fm_path_is_virtual(path);
    allVirtual_ = false;

    // check if the files are all in the trash can
    allTrash_ =  sameFilesystem_ && path.hasUriScheme("trash");

    openAction_ = new QAction(QIcon::fromTheme("document-open"), tr("Open"), this);
    connect(openAction_, &QAction::triggered, this, &FileMenu::onOpenTriggered);
    addAction(openAction_);

    openWithMenuAction_ = new QAction(tr("Open With..."), this);
    addAction(openWithMenuAction_);
    // create the "Open with..." sub menu
    QMenu* menu = new QMenu(this);
    openWithMenuAction_->setMenu(menu);

    if(sameType_) { /* add specific menu items for this mime type */
        if(mime_type && !allVirtual_) { /* the file has a valid mime-type and its not virtual */
            GList* apps = g_app_info_get_all_for_type(mime_type->name());
            GList* l;
            for(l = apps; l; l = l->next) {
                Fm::GAppInfoPtr app{G_APP_INFO(l->data), false};
                // check if the command really exists
                gchar* program_path = g_find_program_in_path(g_app_info_get_executable(app.get()));
                if(!program_path) {
                    continue;
                }
                g_free(program_path);

                // create a QAction for the application.
                AppInfoAction* action = new AppInfoAction(std::move(app), menu);
                connect(action, &QAction::triggered, this, &FileMenu::onApplicationTriggered);
                menu->addAction(action);
            }
            g_list_free(apps);
        }
    }
    menu->addSeparator();
    openWithAction_ = new QAction(tr("Other Applications"), this);
    connect(openWithAction_, &QAction::triggered, this, &FileMenu::onOpenWithTriggered);
    menu->addAction(openWithAction_);

    separator1_ = addSeparator();

    createAction_ = new QAction(tr("Create &New"), this);
    Fm::FilePath dirPath = files_.size() == 1 && info_->isDir() ? path : cwd_;
    createAction_->setMenu(new CreateNewMenu(nullptr, dirPath, this));
    addAction(createAction_);

    separator2_ = addSeparator();

    if(allTrash_) { // all selected files are in trash:///
        bool can_restore = true;
        /* only immediate children of trash:/// can be restored. */
        auto trash_root = Fm::FilePath::fromUri("trash:///");
        for(auto& file: files_) {
            Fm::FilePath trash_path = file->path();
            if(!trash_root.isParentOf(trash_path)) {
                can_restore = false;
                break;
            }
        }
        if(can_restore) {
            unTrashAction_ = new QAction(tr("&Restore"), this);
            connect(unTrashAction_, &QAction::triggered, this, &FileMenu::onUnTrashTriggered);
            addAction(unTrashAction_);
        }
    }
    else { // ordinary files
        cutAction_ = new QAction(QIcon::fromTheme("edit-cut"), tr("Cut"), this);
        connect(cutAction_, &QAction::triggered, this, &FileMenu::onCutTriggered);
        addAction(cutAction_);

        copyAction_ = new QAction(QIcon::fromTheme("edit-copy"), tr("Copy"), this);
        connect(copyAction_, &QAction::triggered, this, &FileMenu::onCopyTriggered);
        addAction(copyAction_);

        pasteAction_ = new QAction(QIcon::fromTheme("edit-paste"), tr("Paste"), this);
        connect(pasteAction_, &QAction::triggered, this, &FileMenu::onPasteTriggered);
        addAction(pasteAction_);

        deleteAction_ = new QAction(QIcon::fromTheme("user-trash"), tr("&Move to Trash"), this);
        connect(deleteAction_, &QAction::triggered, this, &FileMenu::onDeleteTriggered);
        addAction(deleteAction_);

        renameAction_ = new QAction(tr("Rename"), this);
        connect(renameAction_, &QAction::triggered, this, &FileMenu::onRenameTriggered);
        addAction(renameAction_);
    }

    // FIXME: port these parts to Fm API
#ifdef CUSTOM_ACTIONS
    // DES-EMA custom actions integration
    GList* files_list = nullptr;
    for(auto it = files_.crbegin(); it != files_.crend(); ++it) {
        FmFileInfo* fm_info = Fm::_convertFileInfo(*it);
        files_list = g_list_prepend(files_list, fm_info);
        qDebug() << "fi:" << fm_info;
    }
    GList* items = fm_get_actions_for_files(files_list);
    g_list_foreach(files_list, (GFunc)fm_file_info_unref, nullptr);
    g_list_free(files_list);

    if(items) {
        GList* l;
        for(l = items; l; l = l->next) {
            FmFileActionItem* item = FM_FILE_ACTION_ITEM(l->data);
            if(l == items && item
                    && !(fm_file_action_item_is_action(item)
                         && !(fm_file_action_item_get_target(item) & FM_FILE_ACTION_TARGET_CONTEXT))) {
                addSeparator(); // before all custom actions
            }
            addCustomActionItem(this, item);
        }
        g_list_foreach(items, (GFunc)fm_file_action_item_unref, nullptr);
        g_list_free(items);
    }
#endif

    // archiver integration
    // FIXME: we need to modify upstream libfm to include some Qt-based archiver programs.
    if(!allVirtual_) {
        if(sameType_) {
            // FIXME: port these parts to Fm API
            FmArchiver* archiver = fm_archiver_get_default();
            if(archiver) {
                if(fm_archiver_is_mime_type_supported(archiver, mime_type->name())) {
                    if(cwd_ && archiver->extract_to_cmd) {
                        QAction* action = new QAction(tr("Extract to..."), this);
                        connect(action, &QAction::triggered, this, &FileMenu::onExtract);
                        addAction(action);
                    }
                    if(archiver->extract_cmd) {
                        QAction* action = new QAction(tr("Extract Here"), this);
                        connect(action, &QAction::triggered, this, &FileMenu::onExtractHere);
                        addAction(action);
                    }
                }
                else {
                    QAction* action = new QAction(tr("Compress"), this);
                    connect(action, &QAction::triggered, this, &FileMenu::onCompress);
                    addAction(action);
                }
            }
        }
    }

    separator3_ = addSeparator();

    propertiesAction_ = new QAction(QIcon::fromTheme("document-properties"), tr("Properties"), this);
    connect(propertiesAction_, &QAction::triggered, this, &FileMenu::onFilePropertiesTriggered);
    addAction(propertiesAction_);
}

FileMenu::~FileMenu() {
}


#ifdef CUSTOM_ACTIONS
void FileMenu::addCustomActionItem(QMenu* menu, FmFileActionItem* item) {
    if(!item) { // separator
        addSeparator();
        return;
    }

    // this action is not for context menu
    if(fm_file_action_item_is_action(item) && !(fm_file_action_item_get_target(item) & FM_FILE_ACTION_TARGET_CONTEXT)) {
        return;
    }

    CustomAction* action = new CustomAction(item, menu);
    menu->addAction(action);
    if(fm_file_action_item_is_menu(item)) {
        GList* subitems = fm_file_action_item_get_sub_items(item);
        if(subitems != nullptr) {
            QMenu* submenu = new QMenu(menu);
            for(GList* l = subitems; l; l = l->next) {
                FmFileActionItem* subitem = FM_FILE_ACTION_ITEM(l->data);
                addCustomActionItem(submenu, subitem);
            }
            action->setMenu(submenu);
        }
    }
    else if(fm_file_action_item_is_action(item)) {
        connect(action, &QAction::triggered, this, &FileMenu::onCustomActionTrigerred);
    }
}
#endif

void FileMenu::onOpenTriggered() {
    if(fileLauncher_) {
        fileLauncher_->launchFiles(nullptr, files_);
    }
    else { // use the default launcher
        Fm::FileLauncher launcher;
        launcher.launchFiles(nullptr, files_);
    }
}

void FileMenu::onOpenWithTriggered() {
    AppChooserDialog dlg(nullptr);
    if(sameType_) {
        dlg.setMimeType(info_->mimeType());
    }
    else { // we can only set the selected app as default if all files are of the same type
        dlg.setCanSetDefault(false);
    }

    if(execModelessDialog(&dlg) == QDialog::Accepted) {
        auto app = dlg.selectedApp();
        if(app) {
            openFilesWithApp(app.get());
        }
    }
}

void FileMenu::openFilesWithApp(GAppInfo* app) {
    GList* uris = nullptr;
    for(auto& file: files_) {
        auto uri = file->path().uri();
        uris = g_list_prepend(uris, uri.release());
    }
    fm_app_info_launch_uris(app, uris, nullptr, nullptr);
    g_list_foreach(uris, (GFunc)g_free, nullptr);
    g_list_free(uris);
}

void FileMenu::onApplicationTriggered() {
    AppInfoAction* action = static_cast<AppInfoAction*>(sender());
    openFilesWithApp(action->appInfo().get());
}

#ifdef CUSTOM_ACTIONS
void FileMenu::onCustomActionTrigerred() {
    CustomAction* action = static_cast<CustomAction*>(sender());
    FmFileActionItem* item = action->item();

    GList* files_list = nullptr;
    for(auto it = files_.crbegin(); it != files_.crend(); ++it) {
        FmFileInfo* fm_info = Fm::_convertFileInfo(*it);
        files_list = g_list_prepend(files_list, fm_info);
    }
    char* output = nullptr;
    /* g_debug("item: %s is activated, id:%s", fm_file_action_item_get_name(item),
        fm_file_action_item_get_id(item)); */
    fm_file_action_item_launch(item, nullptr, files_list, &output);
    g_list_foreach(files_list, (GFunc)fm_file_info_unref, nullptr);
    g_list_free(files_list);

    if(output) {
        QMessageBox::information(this, tr("Output"), QString::fromUtf8(output));
        g_free(output);
    }
}
#endif

void FileMenu::onFilePropertiesTriggered() {
    FilePropsDialog::showForFiles(files_);
}

void FileMenu::onCopyTriggered() {
    Fm::copyFilesToClipboard(files_.paths());
}

void FileMenu::onCutTriggered() {
    Fm::cutFilesToClipboard(files_.paths());
}

void FileMenu::onDeleteTriggered() {
    auto paths = files_.paths();
    if(useTrash_) {
        FileOperation::trashFiles(paths, confirmTrash_);
    }
    else {
        FileOperation::deleteFiles(paths, confirmDelete_);
    }
}

void FileMenu::onUnTrashTriggered() {
    FileOperation::unTrashFiles(files_.paths());
}

void FileMenu::onPasteTriggered() {
    Fm::pasteFilesFromClipboard(cwd_);
}

void FileMenu::onRenameTriggered() {
    for(auto& info: files_) {
        Fm::renameFile(info, nullptr);
    }
}

void FileMenu::setUseTrash(bool trash) {
    if(useTrash_ != trash) {
        useTrash_ = trash;
        if(deleteAction_) {
            deleteAction_->setText(useTrash_ ? tr("&Move to Trash") : tr("&Delete"));
            deleteAction_->setIcon(useTrash_ ? QIcon::fromTheme("user-trash") : QIcon::fromTheme("edit-delete"));
        }
    }
}

void FileMenu::onCompress() {
    FmArchiver* archiver = fm_archiver_get_default();
    if(archiver) {
        auto paths = Fm::_convertPathList(files_.paths());
        fm_archiver_create_archive(archiver, nullptr, paths.dataPtr());
    }
}

void FileMenu::onExtract() {
    FmArchiver* archiver = fm_archiver_get_default();
    if(archiver) {
        auto paths = Fm::_convertPathList(files_.paths());
        fm_archiver_extract_archives(archiver, nullptr, paths.dataPtr());
    }
}

void FileMenu::onExtractHere() {
    FmArchiver* archiver = fm_archiver_get_default();
    if(archiver) {
        auto paths = Fm::_convertPathList(files_.paths());
        auto cwd = Fm::_convertPath(cwd_);
        fm_archiver_extract_archives_to(archiver, nullptr, paths.dataPtr(), cwd);
    }
}

} // namespace Fm

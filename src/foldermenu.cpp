/*
 * Copyright (C) 2012 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * Copyright (C) 2012 - 2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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


#include "foldermenu.h"
#include "createnewmenu.h"
#include "filepropsdialog.h"
#include "folderview.h"
#include "utilities.h"
#include <cstring> // for memset
#include <QDebug>
#ifdef CUSTOM_ACTIONS
#include "customaction_p.h"
#include <QMessageBox>
#endif

#include "core/compat_p.h"

namespace Fm {

FolderMenu::FolderMenu(FolderView* view, QWidget* parent):
    QMenu(parent),
    view_(view) {

    ProxyFolderModel* model = view_->model();

    createAction_ = new QAction(tr("Create &New"), this);
    addAction(createAction_);

    createAction_->setMenu(new CreateNewMenu(view_, view_->path(), this));

    separator1_ = addSeparator();

    pasteAction_ = new QAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), this);
    addAction(pasteAction_);
    connect(pasteAction_, &QAction::triggered, this, &FolderMenu::onPasteActionTriggered);

    separator2_ = addSeparator();

    selectAllAction_ = new QAction(tr("Select &All"), this);
    addAction(selectAllAction_);
    connect(selectAllAction_, &QAction::triggered, this, &FolderMenu::onSelectAllActionTriggered);

    invertSelectionAction_ = new QAction(tr("Invert Selection"), this);
    addAction(invertSelectionAction_);
    connect(invertSelectionAction_, &QAction::triggered, this, &FolderMenu::onInvertSelectionActionTriggered);

    separator3_ = addSeparator();

    sortAction_ = new QAction(tr("Sorting"), this);
    addAction(sortAction_);
    createSortMenu();
    sortAction_->setMenu(sortMenu_);

    showHiddenAction_ = new QAction(tr("Show Hidden"), this);
    addAction(showHiddenAction_);
    showHiddenAction_->setCheckable(true);
    showHiddenAction_->setChecked(model->showHidden());
    connect(showHiddenAction_, &QAction::triggered, this, &FolderMenu::onShowHiddenActionTriggered);

    // FIXME: port custom actions to Fm2 API
#ifdef CUSTOM_ACTIONS
    auto folderInfo = view_->folderInfo();
    if(folderInfo) {
        GList* single_list = nullptr;
        FmFileInfo* fm_info = Fm2::_convertFileInfo(folderInfo);
        single_list = g_list_prepend(single_list, fm_info);
        GList* items = fm_get_actions_for_files(single_list);
        g_list_foreach(single_list, (GFunc)fm_file_info_unref, nullptr);
        g_list_free(single_list);

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
        }
        g_list_foreach(items, (GFunc)fm_file_action_item_unref, nullptr);
        g_list_free(items);
    }
#endif

    separator4_ = addSeparator();

    propertiesAction_ = new QAction(tr("Folder Pr&operties"), this);
    addAction(propertiesAction_);
    connect(propertiesAction_, &QAction::triggered, this, &FolderMenu::onPropertiesActionTriggered);
}

FolderMenu::~FolderMenu() {
}

#ifdef CUSTOM_ACTIONS
void FolderMenu::addCustomActionItem(QMenu* menu, FmFileActionItem* item) {
    if(!item) {
        return;
    }
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
        connect(action, &QAction::triggered, this, &FolderMenu::onCustomActionTrigerred);
    }
}

void FolderMenu::onCustomActionTrigerred() {
    // FIXME: port to Fm2
    CustomAction* action = static_cast<CustomAction*>(sender());
    FmFileActionItem* item = action->item();
    auto folderInfo = view_->folderInfo();
    if(folderInfo) {
        GList* single_list = nullptr;
        single_list = g_list_prepend(single_list, Fm2::_convertFileInfo(folderInfo));
        char* output = nullptr;
        fm_file_action_item_launch(item, nullptr, single_list, &output);
        g_list_foreach(single_list, (GFunc)fm_file_info_unref, nullptr);
        g_list_free(single_list);
        if(output) {
            QMessageBox::information(this, tr("Output"), QString::fromUtf8(output));
            g_free(output);
        }
    }
}
#endif

void FolderMenu::addSortMenuItem(QString title, int id) {
    QAction* action = new QAction(title, this);
    sortMenu_->addAction(action);
    action->setCheckable(true);
    sortActionGroup_->addAction(action);
    connect(action, &QAction::triggered, this, &FolderMenu::onSortActionTriggered);
    sortActions_[id] = action;
}

void FolderMenu::createSortMenu() {
    ProxyFolderModel* model = view_->model();

    sortMenu_ = new QMenu(this);
    sortActionGroup_ = new QActionGroup(sortMenu_);
    sortActionGroup_->setExclusive(true);

    std::memset(sortActions_, 0, sizeof(sortActions_));

    addSortMenuItem(tr("By File Name"), FolderModel::ColumnFileName);
    addSortMenuItem(tr("By Modification Time"), FolderModel::ColumnFileMTime);
    addSortMenuItem(tr("By File Size"), FolderModel::ColumnFileSize);
    addSortMenuItem(tr("By File Type"), FolderModel::ColumnFileType);
    addSortMenuItem(tr("By File Owner"), FolderModel::ColumnFileOwner);

    int col = model->sortColumn();

    if(col >= 0 && col < FolderModel::NumOfColumns) {
        sortActions_[col]->setChecked(true);;
    }

    sortMenu_->addSeparator();

    QActionGroup* group = new QActionGroup(this);
    group->setExclusive(true);
    actionAscending_ = new QAction(tr("Ascending"), this);
    actionAscending_->setCheckable(true);
    sortMenu_->addAction(actionAscending_);
    group->addAction(actionAscending_);

    actionDescending_ = new QAction(tr("Descending"), this);
    actionDescending_->setCheckable(true);
    sortMenu_->addAction(actionDescending_);
    group->addAction(actionDescending_);

    if(model->sortOrder() == Qt::AscendingOrder) {
        actionAscending_->setChecked(true);
    }
    else {
        actionDescending_->setChecked(true);
    }

    connect(actionAscending_, &QAction::triggered, this, &FolderMenu::onSortOrderActionTriggered);
    connect(actionDescending_, &QAction::triggered, this, &FolderMenu::onSortOrderActionTriggered);

    sortMenu_->addSeparator();

    QAction* actionFolderFirst = new QAction(tr("Folder First"), this);
    sortMenu_->addAction(actionFolderFirst);
    actionFolderFirst->setCheckable(true);

    if(model->folderFirst()) {
        actionFolderFirst->setChecked(true);
    }

    connect(actionFolderFirst, &QAction::triggered, this, &FolderMenu::onFolderFirstActionTriggered);

    QAction* actionCaseSensitive = new QAction(tr("Case Sensitive"), this);
    sortMenu_->addAction(actionCaseSensitive);
    actionCaseSensitive->setCheckable(true);

    if(model->sortCaseSensitivity() == Qt::CaseSensitive) {
        actionCaseSensitive->setChecked(true);
    }

    connect(actionCaseSensitive, &QAction::triggered, this, &FolderMenu::onCaseSensitiveActionTriggered);
}

void FolderMenu::onPasteActionTriggered() {
    auto folderPath = view_->path();
    if(folderPath) {
        pasteFilesFromClipboard(folderPath);
    }
}

void FolderMenu::onSelectAllActionTriggered() {
    view_->selectAll();
}

void FolderMenu::onInvertSelectionActionTriggered() {
    view_->invertSelection();
}

void FolderMenu::onSortActionTriggered(bool checked) {
    ProxyFolderModel* model = view_->model();

    if(model) {
        QAction* action = static_cast<QAction*>(sender());

        for(int col = 0; col < FolderModel::NumOfColumns; ++col) {
            if(action == sortActions_[col]) {
                model->sort(col, model->sortOrder());
                break;
            }
        }
    }
}

void FolderMenu::onSortOrderActionTriggered(bool checked) {
    ProxyFolderModel* model = view_->model();

    if(model) {
        QAction* action = static_cast<QAction*>(sender());
        Qt::SortOrder order;

        if(action == actionAscending_) {
            order = Qt::AscendingOrder;
        }
        else {
            order = Qt::DescendingOrder;
        }

        model->sort(model->sortColumn(), order);
    }
}

void FolderMenu::onShowHiddenActionTriggered(bool checked) {
    ProxyFolderModel* model = view_->model();

    if(model) {
        qDebug("show hidden: %d", checked);
        model->setShowHidden(checked);
    }
}

void FolderMenu::onCaseSensitiveActionTriggered(bool checked) {
    ProxyFolderModel* model = view_->model();

    if(model) {
        model->setSortCaseSensitivity(checked ? Qt::CaseSensitive : Qt::CaseInsensitive);
    }
}

void FolderMenu::onFolderFirstActionTriggered(bool checked) {
    ProxyFolderModel* model = view_->model();

    if(model) {
        model->setFolderFirst(checked);
    }
}

void FolderMenu::onPropertiesActionTriggered() {
    auto folderInfo = view_->folderInfo();
    if(folderInfo) {
        FilePropsDialog::showForFile(folderInfo);
    }
}

} // namespace Fm

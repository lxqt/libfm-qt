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

#include "createnewmenu.h"
#include "folderview.h"
#include "icontheme.h"
#include "utilities.h"
#include "core/iconinfo.h"
#include "core/templates.h"

namespace Fm {


class TemplateAction: public QAction {
public:
    TemplateAction(std::shared_ptr<const TemplateItem> item, QObject* parent): templateItem_{std::move(item)} {
        auto mimeType = templateItem_->mimeType();
        setText(QString("%1 (%2)").arg(templateItem_->displayName()).arg(mimeType->desc()));
        setIcon(templateItem_->icon()->qicon());
    }

    std::shared_ptr<const TemplateItem> templateItem_;
};


CreateNewMenu::CreateNewMenu(QWidget* dialogParent, Fm::FilePath dirPath, QWidget* parent):
    QMenu(parent), dialogParent_(dialogParent), dirPath_(std::move(dirPath)) {
    QAction* action = new QAction(QIcon::fromTheme("folder-new"), tr("Folder"), this);
    connect(action, &QAction::triggered, this, &CreateNewMenu::onCreateNewFolder);
    addAction(action);

    action = new QAction(QIcon::fromTheme("document-new"), tr("Blank File"), this);
    connect(action, &QAction::triggered, this, &CreateNewMenu::onCreateNewFile);
    addAction(action);

    // add more items to "Create New" menu from templates
    auto templates = Templates::globalInstance();
    if(templates->hasTemplates()) {
        addSeparator();
        templates->forEachItem([this](const std::shared_ptr<const TemplateItem>& item) {
            auto mimeType = item->mimeType();
            /* we support directories differently */
            if(mimeType->isDir()) {
                return;
            }
            QAction* action = new TemplateAction{item, this};
            connect(action, &QAction::triggered, this, &CreateNewMenu::onCreateNew);
            addAction(action);
        });
    }
}

CreateNewMenu::~CreateNewMenu() {
}

void CreateNewMenu::onCreateNewFile() {
    if(dirPath_) {
        createFileOrFolder(CreateNewTextFile, dirPath_);
    }
}

void CreateNewMenu::onCreateNewFolder() {
    if(dirPath_) {
        createFileOrFolder(CreateNewFolder, dirPath_);
    }
}

void CreateNewMenu::onCreateNew() {
    TemplateAction* action = static_cast<TemplateAction*>(sender());
    if(dirPath_) {
        createFileOrFolder(CreateWithTemplate, dirPath_, action->templateItem_.get(), dialogParent_);
    }
}

} // namespace Fm

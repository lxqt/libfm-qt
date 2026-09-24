/*
 * Copyright (C) 2014 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef FM_APPMENUVIEW_P_H
#define FM_APPMENUVIEW_P_H

#include <QStandardItem>
#include "core/vfs/desktop-menu.h"
#include "core/iconinfo.h"

namespace Fm {

class AppMenuViewItem : public QStandardItem {
public:
    explicit AppMenuViewItem(DesktopMenuItem* item):
        item_(desktop_menu_item_ref(item)) {
        std::shared_ptr<const Fm::IconInfo> icon;
        if(const char* iconName = desktop_menu_item_get_icon(item_)) {
            icon = Fm::IconInfo::fromName(iconName);
        }
        setText(QString::fromUtf8(desktop_menu_item_get_name(item_)));
        setEditable(false);
        setDragEnabled(false);
        if(icon) {
            setIcon(icon->qicon());
        }
    }

    ~AppMenuViewItem() override {
        desktop_menu_item_unref(item_);
    }

    DesktopMenuItem* item() const {
        return item_;
    }

    bool isApp() const {
        return desktop_menu_item_get_item_type(item_) == DESKTOP_MENU_TYPE_APP;
    }

    bool isDir() const {
        return desktop_menu_item_get_item_type(item_) == DESKTOP_MENU_TYPE_DIR;
    }

private:
    DesktopMenuItem* item_;
};

}

#endif // FM_APPMENUVIEW_P_H

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

#ifndef FM_CUSTOMACTION_P_H
#define FM_CUSTOMACTION_P_H

namespace Fm {

class CustomAction : public QAction {
public:
  explicit CustomAction(FmFileActionItem* item, QObject* parent = NULL):
    QAction(QString::fromUtf8(fm_file_action_item_get_name(item)), parent),
    item_(reinterpret_cast<FmFileActionItem*>(fm_file_action_item_ref(item))) {
    const char* icon_name = fm_file_action_item_get_icon(item);
    if(icon_name)
      setIcon(QIcon::fromTheme(icon_name));
  }

  virtual ~CustomAction() {
    fm_file_action_item_unref(item_);
  }

  FmFileActionItem* item() {
    return item_;
  }

private:
  FmFileActionItem* item_;
};

} // namespace Fm

#endif

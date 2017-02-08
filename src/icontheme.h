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


#ifndef FM_ICONTHEME_H
#define FM_ICONTHEME_H

#include "libfmqtglobals.h"
#include <QIcon>
#include <QString>
#include "libfm/fm.h"

namespace Fm {

class LIBFM_QT_API IconTheme: public QObject {
    Q_OBJECT
public:
    IconTheme();
    ~IconTheme();

    static IconTheme* instance();

    static void checkChanged(); // check if current icon theme name is changed

Q_SIGNALS:
    void changed(); // emitted when the name of current icon theme is changed

protected:
    bool eventFilter(QObject* obj, QEvent* event);

private:
    QString currentThemeName_;
};

}

#endif // FM_ICONTHEME_H

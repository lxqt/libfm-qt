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


#ifndef FM_SIDEPANE_H
#define FM_SIDEPANE_H

#include "libfmqtglobals.h"
#include <libfm/fm.h>
#include <QWidget>

#include "core/filepath.h"

class QComboBox;
class QVBoxLayout;
class QWidget;

namespace Fm {

class FileMenu;

class LIBFM_QT_API SidePane : public QWidget {
    Q_OBJECT

public:
    enum Mode {
        ModeNone = -1,
        ModePlaces = 0,
        ModeDirTree,
        NumModes
    };

public:
    explicit SidePane(QWidget* parent = 0);
    virtual ~SidePane();

    QSize iconSize() const {
        return iconSize_;
    }

    void setIconSize(QSize size);

    const Fm2::FilePath& currentPath() const {
        return currentPath_;
    }

    void setCurrentPath(Fm2::FilePath path);

    void setMode(Mode mode);

    Mode mode() const {
        return mode_;
    }

    QWidget* view() const {
        return view_;
    }

    static const char* modeName(Mode mode);

    static Mode modeByName(const char* str);

#if 0 // FIXME: are these APIs from libfm-qt needed?
    int modeCount(void) {
        return NumModes;
    }

    QString modeLabel(Mode mode);

    QString modeTooltip(Mode mode);
#endif

    void setShowHidden(bool show_hidden);

    bool showHidden() const {
        return showHidden_;
    }

    bool setHomeDir(const char* home_dir);

    void chdir(Fm2::FilePath path) {
        setCurrentPath(std::move(path));
    }

Q_SIGNALS:
    void chdirRequested(int type, const Fm2::FilePath& path);
    void openFolderInNewWindowRequested(const Fm2::FilePath& path);
    void openFolderInNewTabRequested(const Fm2::FilePath& path);
    void openFolderInTerminalRequested(const Fm2::FilePath& path);
    void createNewFolderRequested(const Fm2::FilePath& path);
    void modeChanged(Fm::SidePane::Mode mode);

    void prepareFileMenu(Fm::FileMenu* menu); // emit before showing a Fm::FileMenu

protected Q_SLOTS:
    void onComboCurrentIndexChanged(int current);

private:
    void initDirTree();

private:
    Fm2::FilePath currentPath_;
    QWidget* view_;
    QComboBox* combo_;
    QVBoxLayout* verticalLayout;
    QSize iconSize_;
    Mode mode_;
    bool showHidden_;
};

}

#endif // FM_SIDEPANE_H

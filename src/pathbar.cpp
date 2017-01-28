/*
 * Copyright (C) 2016  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#include "pathbar.h"
#include "pathbar_p.h"
#include <QToolButton>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QDebug>
#include "pathedit.h"


namespace Fm {

PathBar::PathBar(QWidget* parent):
    QWidget(parent),
    tempPathEdit_(nullptr) {

    QHBoxLayout* topLayout = new QHBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);
    bool rtl(layoutDirection() == Qt::RightToLeft);

    // the arrow button used to scroll to start of the path
    scrollToStart_ = new QToolButton(this);
    scrollToStart_->setArrowType(rtl ? Qt::RightArrow : Qt::LeftArrow);
    scrollToStart_->setAutoRepeat(true);
    scrollToStart_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    connect(scrollToStart_, &QToolButton::clicked, this, &PathBar::onScrollButtonClicked);
    topLayout->addWidget(scrollToStart_);

    // there might be too many buttons when the path is long, so make it scrollable.
    scrollArea_ = new QScrollArea(this);
    scrollArea_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    scrollArea_->setFrameShape(QFrame::NoFrame);
    scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    scrollArea_->verticalScrollBar()->setDisabled(true);
    connect(scrollArea_->horizontalScrollBar(), &QAbstractSlider::valueChanged, this, &PathBar::setArrowEnabledState);
    topLayout->addWidget(scrollArea_, 1); // stretch factor=1, make it expandable

    // the arrow button used to scroll to end of the path
    scrollToEnd_ = new QToolButton(this);
    scrollToEnd_->setArrowType(rtl ? Qt::LeftArrow : Qt::RightArrow);
    scrollToEnd_->setAutoRepeat(true);
    scrollToEnd_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    connect(scrollToEnd_, &QToolButton::clicked, this, &PathBar::onScrollButtonClicked);
    topLayout->addWidget(scrollToEnd_);

    // container widget of the path buttons
    buttonsWidget_ = new QWidget(this);
    buttonsWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    buttonsLayout_ = new QHBoxLayout(buttonsWidget_);
    buttonsLayout_->setContentsMargins(0, 0, 0, 0);
    buttonsLayout_->setSpacing(0);
    buttonsLayout_->setSizeConstraint(QLayout::SetFixedSize); // required when added to scroll area according to QScrollArea doc.
    scrollArea_->setWidget(buttonsWidget_); // make the buttons widget scrollable if the path is too long
}

void PathBar::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateScrollButtonVisibility();
}

void PathBar::wheelEvent(QWheelEvent* event) {
    QWidget::wheelEvent(event);
    QAbstractSlider::SliderAction action = QAbstractSlider::SliderNoAction;
    int vDelta = event->angleDelta().y();
    if(vDelta > 0) {
        if(scrollToStart_->isEnabled()) {
            action = QAbstractSlider::SliderSingleStepSub;
        }
    }
    else if(vDelta < 0) {
        if(scrollToEnd_->isEnabled()) {
            action = QAbstractSlider::SliderSingleStepAdd;
        }
    }
    scrollArea_->horizontalScrollBar()->triggerAction(action);
}

void PathBar::mousePressEvent(QMouseEvent* event) {
    QWidget::mousePressEvent(event);
    if(event->button() == Qt::LeftButton) {
        openEditor();
    }
    else if(event->button() == Qt::MiddleButton) {
        PathButton* btn = qobject_cast<PathButton*>(childAt(event->x(), event->y()));
        if(btn != nullptr) {
            scrollArea_->ensureWidgetVisible(btn, 0);
            Q_EMIT middleClickChdir(pathForButton(btn));
        }
    }
}

void PathBar::contextMenuEvent(QContextMenuEvent* event) {
    QMenu* menu = new QMenu(this);
    connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);

    QAction* action = menu->addAction(tr("&Edit Path"));
    connect(action, &QAction::triggered, this, &PathBar::openEditor);

    action = menu->addAction(tr("&Copy Path"));
    connect(action, &QAction::triggered, this, &PathBar::copyPath);

    menu->popup(mapToGlobal(event->pos()));
}

void PathBar::updateScrollButtonVisibility() {
    bool showScrollers;
    if(tempPathEdit_ != nullptr) {
        showScrollers = false;
    }
    else {
        showScrollers = (buttonsLayout_->sizeHint().width() > width());
    }
    scrollToStart_->setVisible(showScrollers);
    scrollToEnd_->setVisible(showScrollers);
    if(showScrollers) {
        QScrollBar* sb = scrollArea_->horizontalScrollBar();
        int value = sb->value();
        scrollToStart_->setEnabled(value != sb->minimum());
        scrollToEnd_->setEnabled(value != sb->maximum());
    }
}

Fm2::FilePath PathBar::pathForButton(PathButton* btn) {
    std::string fullPath;
    int buttonCount = buttonsLayout_->count() - 1; // the last item is a spacer
    for(int i = 0; i < buttonCount; ++i) {
        if(!fullPath.empty() && fullPath.back() != '/') {
            fullPath += '/';
        }
        PathButton* elem = static_cast<PathButton*>(buttonsLayout_->itemAt(i)->widget());
        fullPath += elem->name();
        if(elem == btn)
            break;
    }
    return Fm2::FilePath::fromPathStr(fullPath.c_str());
}

void PathBar::onButtonToggled(bool checked) {
    if(checked) {
        PathButton* btn = static_cast<PathButton*>(sender());
        scrollArea_->ensureWidgetVisible(btn, 0); // make the button visible

        currentPath_ = pathForButton(btn);
        Q_EMIT chdir(currentPath_);
    }
}


void PathBar::onScrollButtonClicked() {
    QToolButton* btn = static_cast<QToolButton*>(sender());
    QAbstractSlider::SliderAction action = QAbstractSlider::SliderNoAction;
    if(btn == scrollToEnd_) {
        action = QAbstractSlider::SliderSingleStepAdd;
    }
    else if(btn == scrollToStart_) {
        action = QAbstractSlider::SliderSingleStepSub;
    }
    scrollArea_->horizontalScrollBar()->triggerAction(action);
}

void PathBar::setPath(Fm2::FilePath path) {
    if(currentPath_ == path) { // same path, do nothing
        return;
    }

    auto oldPath = std::move(currentPath_);
    currentPath_ = std::move(path);
    // check if we already have a button for this path
    int buttonCount = buttonsLayout_->count() - 1; // the last item is a spacer
    if(oldPath && currentPath_.isPrefixOf(oldPath)) {
        auto btnPath = oldPath;
        for(int i = buttonCount - 1; i >= 0; --i) {
            if(btnPath == currentPath_) {
                auto btn = static_cast<PathButton*>(buttonsLayout_->itemAt(i)->widget());
                btn->setChecked(true); // toggle the button
                /* we don't need to emit chdir signal here since later
                 * toggled signal will be triggered on the button, which
                 * in turns emit chdir. */
                return;
            }
            btnPath = btnPath.parent();
        }
    }

    /* FIXME: if the new path is the subdir of our full path, actually
     *        we can append several new buttons rather than re-create
     *        all of the buttons. */

    setUpdatesEnabled(false);
    // we do not have the path in the buttons list
    // destroy existing path element buttons and the spacer
    QLayoutItem* item;
    while((item = buttonsLayout_->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // create new buttons for the new path
    auto btnPath = currentPath_;
    while(btnPath) {
        Fm2::CStrPtr name;
        auto parent = btnPath.parent();
        auto isRoot = !parent.isValid();
        if(isRoot) {
            name = btnPath.displayName();
        }
        else {
            name = btnPath.baseName();
        }
        auto btn = new PathButton(name.get(), name.get(), isRoot, buttonsWidget_);
        btn->show();
        connect(btn, &QPushButton::toggled, this, &PathBar::onButtonToggled);
        btnPath = parent;
        buttonsLayout_->insertWidget(0, btn);
    }
    buttonsLayout_->addStretch(1); // add a spacer at the tail of the buttons

    buttonCount = buttonsLayout_->count() - 1;
    if(buttonCount > 0) {
        PathButton* lastBtn = static_cast<PathButton*>(buttonsLayout_->itemAt(buttonCount - 1)->widget());
        // we don't have to emit the chdir signal since the "onButtonToggled()" slot will be triggered by this.
        lastBtn->setChecked(true);
    }

    // we don't want to scroll vertically. make the scroll area fit the height of the buttons
    // FIXME: this is a little bit hackish :-(
    scrollArea_->setFixedHeight(buttonsLayout_->sizeHint().height());
    updateScrollButtonVisibility();
    setUpdatesEnabled(true);
}

void PathBar::openEditor() {
    if(tempPathEdit_ == nullptr) {
        tempPathEdit_ = new PathEdit(this);
        layout()->replaceWidget(scrollArea_, tempPathEdit_, Qt::FindDirectChildrenOnly);
        scrollArea_->hide();
        scrollToStart_->setVisible(false);
        scrollToEnd_->setVisible(false);
        tempPathEdit_->setText(currentPath_.displayName().get());

        connect(tempPathEdit_, &PathEdit::returnPressed, this, &PathBar::onReturnPressed);
        connect(tempPathEdit_, &PathEdit::editingFinished, this, &PathBar::closeEditor);
    }
    tempPathEdit_->setFocus();
    tempPathEdit_->selectAll();
}

void PathBar::closeEditor() {
    if(tempPathEdit_ == nullptr) {
        return;
    }
    // If a menu has popped up synchronously (with QMenu::exec), the path buttons may be drawn
    // but the path-edit may not disappear until the menu is closed. So, we hide it here.
    tempPathEdit_->setVisible(false);
    layout()->replaceWidget(tempPathEdit_, scrollArea_, Qt::FindDirectChildrenOnly);
    scrollArea_->show();
    if(buttonsLayout_->sizeHint().width() > width()) {
        scrollToStart_->setVisible(true);
        scrollToEnd_->setVisible(true);
    }

    tempPathEdit_->deleteLater();
    tempPathEdit_ = nullptr;
    updateScrollButtonVisibility();

    Q_EMIT editingFinished();
}

void PathBar::copyPath() {
    QApplication::clipboard()->setText(currentPath_.toString().get());
}

void PathBar::onReturnPressed() {
    QByteArray pathStr = tempPathEdit_->text().toLocal8Bit();
    setPath(Fm2::FilePath::fromPathStr(pathStr.constData()));
}

void PathBar::setArrowEnabledState(int value) {
    if(buttonsLayout_->sizeHint().width() > width()) {
        QScrollBar* sb = scrollArea_->horizontalScrollBar();
        scrollToStart_->setEnabled(value != sb->minimum());
        scrollToEnd_->setEnabled(value != sb->maximum());
    }
}

} // namespace Fm

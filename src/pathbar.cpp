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

PathBar::PathBar(QWidget *parent):
  QWidget(parent),
  tempPathEdit_(nullptr) {

  QHBoxLayout* topLayout = new QHBoxLayout(this);
  topLayout->setContentsMargins(0, 0, 0, 0);
  topLayout->setSpacing(0);
  bool rtl(layoutDirection() == Qt::RightToLeft);

  // left arrow (scroll to left)
  leftArrow_ = new QToolButton(this);
  if(rtl)
    leftArrow_->setArrowType(Qt::RightArrow);
  else
    leftArrow_->setArrowType(Qt::LeftArrow);
  leftArrow_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
  connect(leftArrow_, &QToolButton::clicked, this, &PathBar::onScrollButtonClicked);
  topLayout->addWidget(leftArrow_);

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

  // right arrow (scroll to right)
  rightArrow_ = new QToolButton(this);
  if(rtl)
    rightArrow_->setArrowType(Qt::LeftArrow);
  else
    rightArrow_->setArrowType(Qt::RightArrow);
  rightArrow_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
  connect(rightArrow_, &QToolButton::clicked, this, &PathBar::onScrollButtonClicked);
  topLayout->addWidget(rightArrow_);

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
    if(leftArrow_->isEnabled())
      action = QAbstractSlider::SliderSingleStepSub;
  }
  else if(vDelta < 0) {
    if(rightArrow_->isEnabled())
      action = QAbstractSlider::SliderSingleStepAdd;
  }
  scrollArea_->horizontalScrollBar()->triggerAction(action);
}

void PathBar::mousePressEvent(QMouseEvent *event) {
  QWidget::mousePressEvent(event);
  if(event->button() == Qt::LeftButton)
    openEditor();
}

void PathBar::contextMenuEvent(QContextMenuEvent *event) {
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
  leftArrow_->setVisible(showScrollers);
  rightArrow_->setVisible(showScrollers);
  QScrollBar* sb = scrollArea_->horizontalScrollBar();
  int value = sb->value();
  leftArrow_->setEnabled(value != sb->minimum());
  rightArrow_->setEnabled(value != sb->maximum());
}

void PathBar::onButtonToggled(bool checked) {
  if(checked) {
    PathButton* btn = static_cast<PathButton*>(sender());
    scrollArea_->ensureWidgetVisible(btn, 0); // make the button visible

    currentPath_ = btn->pathElement();
    // qDebug("chdir: %s", currentPath_.displayName(false));
    Q_EMIT chdir(currentPath_.dataPtr());
  }
}

void PathBar::onScrollButtonClicked() {
  QToolButton* btn = static_cast<QToolButton*>(sender());
  QAbstractSlider::SliderAction action = QAbstractSlider::SliderNoAction;
  if(btn == rightArrow_)
    action = QAbstractSlider::SliderSingleStepAdd;
  else if (btn == leftArrow_)
    action = QAbstractSlider::SliderSingleStepSub;
  scrollArea_->horizontalScrollBar()->triggerAction(action);
}

void PathBar::setPath(Path path) {
  if(!currentPath_.isNull() && !path.isNull() && currentPath_ == path) // same path, do nothing
    return;

  currentPath_ = path;
  int buttonCount = buttonsLayout_->count() - 1; // the last item is a spacer
  // check if we already have a button for this path (FIXME: this loop is inefficient)
  for(int i = buttonCount - 1; i >= 0; --i) {
    PathButton* btn = static_cast<PathButton*>(buttonsLayout_->itemAt(i)->widget());
    if(btn->pathElement() == path) {  // we have a button for this path
      btn->setChecked(true); // toggle the button
      /* we don't need to emit chdir signal here since later
       * toggled signal will be triggered on the button, which
       * in turns emit chdir. */
      return;
    }
  }

  /* FIXME: if the new path is the subdir of our full path, actually
   *        we can append several new buttons rather than re-create
   *        all of the buttons. */

  // we do not have the path in the buttons list
  // destroy existing path element buttons and the spacer
  QLayoutItem* item;
  while((item = buttonsLayout_->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }

  Path pathElement = path;
  // create new buttons for the new path
  while(!pathElement.isNull()) {
    // qDebug("%s", pathElement.displayName(false));
    PathButton* btn = new PathButton(pathElement, buttonsWidget_);
    btn->show();
    connect(btn, &QPushButton::toggled, this, &PathBar::onButtonToggled);
    pathElement = pathElement.getParent();
    buttonsLayout_->insertWidget(0, btn);
  }
  buttonCount = buttonsLayout_->count();
  if(buttonCount) {
    PathButton* lastBtn = static_cast<PathButton*>(buttonsLayout_->itemAt(buttonCount - 1)->widget());
    // we don't have to emit the chdir signal since the "onButtonToggled()" slot will be triggered by this.
    lastBtn->setChecked(true);
  }
  buttonsLayout_->addStretch(1);

  // we don't want to scroll vertically. make the scroll area fit the height of the buttons
  // FIXME: this is a little bit hackish :-(
  scrollArea_->setFixedHeight(buttonsLayout_->sizeHint().height());
  updateScrollButtonVisibility();
}

void PathBar::openEditor() {
  if(tempPathEdit_ == nullptr) {
    tempPathEdit_ = new PathEdit(this);
    layout()->replaceWidget(scrollArea_, tempPathEdit_, Qt::FindDirectChildrenOnly);
    scrollArea_->hide();
    leftArrow_->setVisible(false);
    rightArrow_->setVisible(false);
    char* pathStr = currentPath_.toStr();
    tempPathEdit_->setText(pathStr);
    g_free(pathStr);

    connect(tempPathEdit_, &PathEdit::returnPressed, this, &PathBar::onReturnPressed);
    connect(tempPathEdit_, &PathEdit::editingFinished, this, &PathBar::closeEditor);
  }
  tempPathEdit_->setFocus();
  tempPathEdit_->selectAll();
}

void PathBar::closeEditor() {
  if(tempPathEdit_ == nullptr)
    return;
  layout()->replaceWidget(tempPathEdit_, scrollArea_, Qt::FindDirectChildrenOnly);
  scrollArea_->show();
    leftArrow_->setVisible(true);
    rightArrow_->setVisible(true);

  tempPathEdit_->deleteLater();
  tempPathEdit_ = nullptr;
  updateScrollButtonVisibility();

  Q_EMIT editingFinished();
}

void PathBar::copyPath() {
  char* pathStr = currentPath_.toStr();
  QApplication::clipboard()->setText(pathStr);
  g_free(pathStr);
}

void PathBar::onReturnPressed() {
  QByteArray pathStr = tempPathEdit_->text().toLocal8Bit();
  Path path = Path::newForDisplayName(pathStr.constData());
  setPath(path);
}

void PathBar::setArrowEnabledState(int value) {
  QScrollBar* sb = scrollArea_->horizontalScrollBar();
  leftArrow_->setEnabled(value != sb->minimum());
  rightArrow_->setEnabled(value != sb->maximum());
}


} // namespace Fm

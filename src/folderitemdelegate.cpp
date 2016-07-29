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


#include "folderitemdelegate.h"
#include "foldermodel.h"
#include <QPainter>
#include <QModelIndex>
#include <QStyleOptionViewItemV4>
#include <QApplication>
#include <QIcon>
#include <QTextLayout>
#include <QTextOption>
#include <QTextLine>
#include <QDebug>

namespace Fm {

FolderItemDelegate::FolderItemDelegate(QAbstractItemView* view, QObject* parent):
  QStyledItemDelegate(parent ? parent : view),
  symlinkIcon_(QIcon::fromTheme("emblem-symbolic-link")),
  view_(view) {
}

FolderItemDelegate::~FolderItemDelegate() {

}

QSize FolderItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
  QVariant value = index.data(Qt::SizeHintRole);
  if(value.isValid())
    return qvariant_cast<QSize>(value);
  if(option.decorationPosition == QStyleOptionViewItem::Top ||
    option.decorationPosition == QStyleOptionViewItem::Bottom) {

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    opt.decorationAlignment = Qt::AlignHCenter|Qt::AlignTop;
    opt.displayAlignment = Qt::AlignTop|Qt::AlignHCenter;

    Q_ASSERT(gridSize_ != QSize());
    QRectF textRect(0, 0, gridSize_.width(), gridSize_.height() - opt.decorationSize.height());
    drawText(nullptr, opt, textRect); // passing NULL for painter will calculate the bounding rect only.
    int width = qMax((int)textRect.width(), opt.decorationSize.width());
    int height = opt.decorationSize.height() + textRect.height();
    return QSize(width, height);
  }
  return QStyledItemDelegate::sizeHint(option, index);
}

QIcon::Mode FolderItemDelegate::iconModeFromState(const QStyle::State state) {

  if(state & QStyle::State_Enabled)
    return (state & QStyle::State_Selected) ? QIcon::Selected : QIcon::Normal;

  return QIcon::Disabled;
}

// special thanks to Razor-qt developer Alec Moskvin(amoskvin) for providing the fix!
void FolderItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
  Q_ASSERT(index.isValid());
  FmFileInfo* file = static_cast<FmFileInfo*>(index.data(FolderModel::FileInfoRole).value<void*>());
  bool isSymlink = file && fm_file_info_is_symlink(file);

  if(option.decorationPosition == QStyleOptionViewItem::Top ||
    option.decorationPosition == QStyleOptionViewItem::Bottom) {
    painter->save();
    painter->setClipRect(option.rect);

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    opt.decorationAlignment = Qt::AlignHCenter|Qt::AlignTop;
    opt.displayAlignment = Qt::AlignTop|Qt::AlignHCenter;

    // draw the icon
    QIcon::Mode iconMode = iconModeFromState(opt.state);
    QPoint iconPos(opt.rect.x() + (opt.rect.width() - opt.decorationSize.width()) / 2, opt.rect.y());
    QPixmap pixmap = opt.icon.pixmap(opt.decorationSize, iconMode);
    painter->drawPixmap(iconPos, pixmap);

    // draw some emblems for the item if needed
    // we only support symlink emblem at the moment
    if(isSymlink)
      painter->drawPixmap(iconPos, symlinkIcon_.pixmap(opt.decorationSize / 2, iconMode));

    // draw the text
    // The text rect dimensions should be exactly as they were in sizeHint()
    QRectF textRect(opt.rect.x() - (gridSize_.width() - opt.rect.width()) / 2,
                    opt.rect.y() + opt.decorationSize.height(),
                    gridSize_.width(),
                    gridSize_.height() - opt.decorationSize.height());
    drawText(painter, opt, textRect);
    painter->restore();
  }
  else {
    // let QStyledItemDelegate does its default painting
    QStyledItemDelegate::paint(painter, option, index);

    // draw emblems if needed
    if(isSymlink) {
      QStyleOptionViewItemV4 opt = option;
      initStyleOption(&opt, index);
      QIcon::Mode iconMode = iconModeFromState(opt.state);
      QPoint iconPos(opt.rect.x(), opt.rect.y() + (opt.rect.height() - opt.decorationSize.height()) / 2);
      // draw some emblems for the item if needed
      // we only support symlink emblem at the moment
      painter->drawPixmap(iconPos, symlinkIcon_.pixmap(opt.decorationSize / 2, iconMode));
    }
  }
}

// if painter is nullptr, the method calculate the bounding rectangle of the text and save it to textRect
void FolderItemDelegate::drawText(QPainter* painter, QStyleOptionViewItemV4& opt, QRectF& textRect) const {
  QTextLayout layout(opt.text, opt.font);
  QTextOption textOption;
  textOption.setAlignment(opt.displayAlignment);
  textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  if (opt.text.isRightToLeft())
    textOption.setTextDirection(Qt::RightToLeft);
  else
    textOption.setTextDirection(Qt::LeftToRight);
  layout.setTextOption(textOption);
  qreal height = 0;
  qreal width = 0;
  int visibleLines = 0;
  layout.beginLayout();
  QString elidedText;
  textRect.adjust(2, 2, -2, -2); // a 2-px margin is considered at FolderView::updateGridSize()
  for(;;) {
    QTextLine line = layout.createLine();
    if(!line.isValid())
      break;
    line.setLineWidth(textRect.width());
    height += opt.fontMetrics.leading();
    line.setPosition(QPointF(0, height));
    if((height + line.height() + textRect.y()) > textRect.bottom()) {
      // if part of this line falls outside the textRect, ignore it and quit.
      QTextLine lastLine = layout.lineAt(visibleLines - 1);
      elidedText = opt.text.mid(lastLine.textStart());
      elidedText = opt.fontMetrics.elidedText(elidedText, opt.textElideMode, textRect.width());
      if(visibleLines == 1) // this is the only visible line
        width = textRect.width();
      break;
    }
    height += line.height();
    width = qMax(width, line.naturalTextWidth());
    ++ visibleLines;
  }
  layout.endLayout();
  width = qMax(width, (qreal)opt.fontMetrics.width(elidedText));

  // draw background for selected item
  QRectF boundRect = layout.boundingRect();
  //qDebug() << "bound rect: " << boundRect << "width: " << width;
  boundRect.setWidth(width);
  boundRect.setHeight(height);
  boundRect.moveTo(textRect.x() + (textRect.width() - width)/2, textRect.y());

  QRectF selRect = boundRect.adjusted(-2, -2, 2, 2);

  if(!painter) { // no painter, calculate the bounding rect only
    textRect = selRect;
    return;
  }

  QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
  if(opt.state & QStyle::State_Selected) {
    if(!opt.widget)
      painter->fillRect(selRect, opt.palette.highlight());
    painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
  }
  else
    painter->setPen(opt.palette.color(cg, QPalette::Text));

  if (opt.state & QStyle::State_Selected || opt.state & QStyle::State_MouseOver) {
    if (const QWidget* widget = opt.widget) { // let the style engine do it
      QStyle* style = widget->style() ? widget->style() : qApp->style();
      QStyleOptionViewItemV4 o(opt);
      o.text = QString();
      o.rect = selRect.toAlignedRect().intersected(opt.rect); // due to clipping and rounding, we might lose 1px
      o.showDecorationSelected = true;
      style->drawPrimitive(QStyle::PE_PanelItemViewItem, &o, painter, widget);
    }
  }

  // draw text
  for(int i = 0; i < visibleLines; ++i) {
    QTextLine line = layout.lineAt(i);
    if(i == (visibleLines - 1) && !elidedText.isEmpty()) { // the last line, draw elided text
      QPointF pos(boundRect.x() + line.position().x(), boundRect.y() + line.y() + line.ascent());
      painter->drawText(pos, elidedText);
    }
    else {
      line.draw(painter, textRect.topLeft());
    }
  }

  if(opt.state & QStyle::State_HasFocus) {
    // draw focus rect
    QStyleOptionFocusRect o;
    o.QStyleOption::operator=(opt);
    o.rect = selRect.toRect(); // subElementRect(SE_ItemViewItemFocusRect, vopt, widget);
    o.state |= QStyle::State_KeyboardFocusChange;
    o.state |= QStyle::State_Item;
    QPalette::ColorGroup cg = (opt.state & QStyle::State_Enabled)
                  ? QPalette::Normal : QPalette::Disabled;
    o.backgroundColor = opt.palette.color(cg, (opt.state & QStyle::State_Selected)
                                  ? QPalette::Highlight : QPalette::Window);
    if (const QWidget* widget = opt.widget) {
      QStyle* style = widget->style() ? widget->style() : qApp->style();
      style->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter, widget);
    }
  }
}


} // namespace Fm

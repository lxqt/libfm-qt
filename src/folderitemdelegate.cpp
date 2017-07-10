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
#include <QAbstractItemView>
#include <QStyleOptionViewItem>
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
    fileInfoRole_(Fm::FolderModel::FileInfoRole),
    iconInfoRole_(-1),
    margins_(QSize(3, 3)) {
}

FolderItemDelegate::~FolderItemDelegate() {

}

QSize FolderItemDelegate::iconViewTextSize(const QModelIndex& index) const {
    QStyleOptionViewItem opt;
    initStyleOption(&opt, index);
    opt.decorationSize = iconSize_.isValid() ? iconSize_ : QSize(0, 0);
    opt.decorationAlignment = Qt::AlignHCenter | Qt::AlignTop;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;
    QRectF textRect(0, 0,
                    itemSize_.width() - 2 * margins_.width(),
                    itemSize_.height() - 2 * margins_.height() - opt.decorationSize.height());
    drawText(nullptr, opt, textRect); // passing nullptr for painter will calculate the bounding rect only
    return textRect.toRect().size();
}

QSize FolderItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QVariant value = index.data(Qt::SizeHintRole);
    if(value.isValid()) {
        // no further processing if the size is specified by the data model
        return qvariant_cast<QSize>(value);
    }

    if(option.decorationPosition == QStyleOptionViewItem::Top ||
            option.decorationPosition == QStyleOptionViewItem::Bottom) {
        // we handle vertical layout just by returning our item size
        return itemSize_;
    }

    // fallback to default size hint for horizontal layout.
    return QStyledItemDelegate::sizeHint(option, index);
}

QIcon::Mode FolderItemDelegate::iconModeFromState(const QStyle::State state) {

    if(state & QStyle::State_Enabled) {
        return (state & QStyle::State_Selected) ? QIcon::Selected : QIcon::Normal;
    }

    return QIcon::Disabled;
}

void FolderItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if(!index.isValid())
        return;

    // get emblems for this icon
    std::forward_list<std::shared_ptr<const Fm::IconInfo>> icon_emblems;
    auto fmicon = index.data(iconInfoRole_).value<std::shared_ptr<const Fm::IconInfo>>();
    if(fmicon) {
        icon_emblems = fmicon->emblems();
    }
    // get file info for the item
    auto file = index.data(fileInfoRole_).value<std::shared_ptr<const Fm::FileInfo>>();
    const auto& emblems = file ? file->emblems() : icon_emblems;

    bool isSymlink = file && file->isSymlink();
    // vertical layout (icon mode, thumbnail mode)
    if(option.decorationPosition == QStyleOptionViewItem::Top ||
            option.decorationPosition == QStyleOptionViewItem::Bottom) {
        painter->save();
        painter->setClipRect(option.rect);

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.decorationAlignment = Qt::AlignHCenter | Qt::AlignTop;
        opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

        // draw the icon
        QIcon::Mode iconMode = iconModeFromState(opt.state);
        QPoint iconPos(opt.rect.x() + (opt.rect.width() - option.decorationSize.width()) / 2, opt.rect.y() + margins_.height());
        QPixmap pixmap = opt.icon.pixmap(option.decorationSize, iconMode);
        // in case the pixmap is smaller than the requested size
        QSize margin = ((option.decorationSize - pixmap.size()) / 2).expandedTo(QSize(0, 0));
        painter->drawPixmap(iconPos + QPoint(margin.width(), margin.height()), pixmap);

        // draw some emblems for the item if needed
        if(isSymlink) {
            // draw the emblem for symlinks
            painter->drawPixmap(iconPos, symlinkIcon_.pixmap(option.decorationSize / 2, iconMode));
        }

        // draw other emblems if there's any
        if(!emblems.empty()) {
            // FIXME: we only support one emblem now
            QPoint emblemPos(opt.rect.x() + opt.rect.width() / 2, opt.rect.y() + option.decorationSize.height() / 2);
            QIcon emblem = emblems.front()->qicon();
            painter->drawPixmap(emblemPos, emblem.pixmap(option.decorationSize / 2, iconMode));
        }

        // draw the text
        QSize drawAreaSize = itemSize_ - 2 * margins_;
        // The text rect dimensions should be exactly as they were in sizeHint()
        QRectF textRect(opt.rect.x() + (opt.rect.width() - drawAreaSize.width()) / 2,
                        opt.rect.y() + margins_.height() + option.decorationSize.height(),
                        drawAreaSize.width(),
                        drawAreaSize.height() - option.decorationSize.height());
        drawText(painter, opt, textRect);
        painter->restore();
    }
    else {  // horizontal layout (list view)

        // let QStyledItemDelegate does its default painting
        QStyledItemDelegate::paint(painter, option, index);

        // draw emblems if needed
        if(isSymlink || !emblems.empty()) {
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            QIcon::Mode iconMode = iconModeFromState(opt.state);
            // draw some emblems for the item if needed
            if(isSymlink) {
                QPoint iconPos(opt.rect.x(), opt.rect.y() + (opt.rect.height() - option.decorationSize.height()) / 2);
                painter->drawPixmap(iconPos, symlinkIcon_.pixmap(option.decorationSize / 2, iconMode));
            }
            else {
                // FIXME: we only support one emblem now
                QPoint iconPos(opt.rect.x() + option.decorationSize.width() / 2, opt.rect.y() + opt.rect.height() / 2);
                QIcon emblem = emblems.front()->qicon();
                painter->drawPixmap(iconPos, emblem.pixmap(option.decorationSize / 2, iconMode));
            }
        }
    }
}

// if painter is nullptr, the method calculate the bounding rectangle of the text and save it to textRect
void FolderItemDelegate::drawText(QPainter* painter, QStyleOptionViewItem& opt, QRectF& textRect) const {
    QTextLayout layout(opt.text, opt.font);
    QTextOption textOption;
    textOption.setAlignment(opt.displayAlignment);
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    // FIXME:     textOption.setTextDirection(opt.direction); ?
    if(opt.text.isRightToLeft()) {
        textOption.setTextDirection(Qt::RightToLeft);
    }
    else {
        textOption.setTextDirection(Qt::LeftToRight);
    }
    layout.setTextOption(textOption);
    qreal height = 0;
    qreal width = 0;
    int visibleLines = 0;
    layout.beginLayout();
    QString elidedText;
    textRect.adjust(2, 2, -2, -2); // a 2-px margin is considered at FolderView::updateGridSize()
    for(;;) {
        QTextLine line = layout.createLine();
        if(!line.isValid()) {
            break;
        }
        line.setLineWidth(textRect.width());
        height += opt.fontMetrics.leading();
        line.setPosition(QPointF(0, height));
        if((height + line.height() + textRect.y()) > textRect.bottom()) {
            // if part of this line falls outside the textRect, ignore it and quit.
            QTextLine lastLine = layout.lineAt(visibleLines - 1);
            elidedText = opt.text.mid(lastLine.textStart());
            elidedText = opt.fontMetrics.elidedText(elidedText, opt.textElideMode, textRect.width());
            if(visibleLines == 1) { // this is the only visible line
                width = textRect.width();
            }
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
    boundRect.moveTo(textRect.x() + (textRect.width() - width) / 2, textRect.y());

    QRectF selRect = boundRect.adjusted(-2, -2, 2, 2);

    if(!painter) { // no painter, calculate the bounding rect only
        textRect = selRect;
        return;
    }

    // ?????
    QPalette::ColorGroup cg = (opt.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
    if(opt.state & QStyle::State_Selected) {
        if(!opt.widget) {
            painter->fillRect(selRect, opt.palette.highlight());
        }
        painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
    }
    else {
        painter->setPen(opt.palette.color(cg, QPalette::Text));
    }

    if(opt.state & QStyle::State_Selected || opt.state & QStyle::State_MouseOver) {
        if(const QWidget* widget = opt.widget) {  // let the style engine do it
            QStyle* style = widget->style() ? widget->style() : qApp->style();
            QStyleOptionViewItem o(opt);
            o.text = QString();
            o.rect = selRect.toAlignedRect().intersected(opt.rect); // due to clipping and rounding, we might lose 1px
            o.showDecorationSelected = true;
            style->drawPrimitive(QStyle::PE_PanelItemViewItem, &o, painter, widget);
        }
    }

    // draw shadow for text if the item is not selected and a shadow color is set
    if(!(opt.state & QStyle::State_Selected) && shadowColor_.isValid()) {
        QPen prevPen = painter->pen();
        painter->setPen(QPen(shadowColor_));
        for(int i = 0; i < visibleLines; ++i) {
            QTextLine line = layout.lineAt(i);
            if(i == (visibleLines - 1) && !elidedText.isEmpty()) { // the last line, draw elided text
                QPointF pos(boundRect.x() + line.position().x() + 1, boundRect.y() + line.y() + line.ascent() + 1);
                painter->drawText(pos, elidedText);
            }
            else {
                line.draw(painter, textRect.topLeft() + QPointF(1, 1));
            }
        }
        painter->setPen(prevPen);
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
        if(const QWidget* widget = opt.widget) {
            QStyle* style = widget->style() ? widget->style() : qApp->style();
            style->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter, widget);
        }
    }
}


} // namespace Fm

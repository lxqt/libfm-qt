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

#ifndef FM_ICONENGINE_H
#define FM_ICONENGINE_H

#include <QIconEngine>
#include <QPainter>
#include "../libfmqtglobals.h"
#include "iconinfo.h"
#include <gio/gio.h>

namespace Fm {

class IconEngine: public QIconEngine {
public:

    IconEngine(std::shared_ptr<const Fm::IconInfo> info);

    ~IconEngine() override;

    QSize actualSize(const QSize& size, QIcon::Mode mode, QIcon::State state) override;

    // not supported
    void addFile(const QString& /*fileName*/, const QSize& /*size*/, QIcon::Mode /*mode*/, QIcon::State /*state*/) override {}

    // not supported
    void addPixmap(const QPixmap& /*pixmap*/, QIcon::Mode /*mode*/, QIcon::State /*state*/) override {}

    QIconEngine* clone() const override;

    QString key() const override;

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;

    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;

    QString iconName() override;
    bool isNull() override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override;
    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) override;
    void virtual_hook(int id, void* data) override;

private:
    std::weak_ptr<const Fm::IconInfo> info_;
};

IconEngine::IconEngine(std::shared_ptr<const IconInfo> info): info_{info} {
}

IconEngine::~IconEngine() {
}

QSize IconEngine::actualSize(const QSize& size, QIcon::Mode mode, QIcon::State state) {
    auto info = info_.lock();
    return info ? info->internalQicon().actualSize(size, mode, state) : QSize{};
}

QIconEngine* IconEngine::clone() const {
    IconEngine* engine = new IconEngine(info_.lock());
    return engine;
}

QString IconEngine::key() const {
    return QStringLiteral("Fm::IconEngine");
}

void IconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) {
    auto info = info_.lock();
    if(info) {
        info->internalQicon().paint(painter, rect, Qt::AlignCenter, mode, state);
    }
}

QPixmap IconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) {
    auto info = info_.lock();
    return info ? info->internalQicon().pixmap(size, mode, state) : QPixmap{};
}

QString IconEngine::iconName() {
    auto info = info_.lock();
    return info ? info->internalQicon().name() : QString{};
}

bool IconEngine::isNull() {
    auto info = info_.lock();
    return info ? info->internalQicon().isNull() : true;
}

QPixmap IconEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) {
    auto info = info_.lock();
    return info ?
           // According to Qt doc, "size" is device-independent since Qt 6.8,
           // while it was device-dependent prior to Qt 6.8.
#if (QT_VERSION < QT_VERSION_CHECK(6,8,0))
           info->internalQicon().pixmap((size.toSizeF() / scale).toSize(), scale, mode, state)
#else
           info->internalQicon().pixmap(size, scale, mode, state)
#endif
           : QPixmap{};
}

QList<QSize> IconEngine::availableSizes(QIcon::Mode mode, QIcon::State state) {
    auto info = info_.lock();
    return info ? info->internalQicon().availableSizes(mode, state) : QList<QSize>{};
}

void IconEngine::virtual_hook(int id, void* data) {
    auto info = info_.lock();
    switch(id) {
    case QIconEngine::IsNullHook: {
        bool* result = reinterpret_cast<bool*>(data);
        *result = info ? info->internalQicon().isNull() : true;
        break;
    }
    case QIconEngine::ScaledPixmapHook: {
        auto* arg = reinterpret_cast<QIconEngine::ScaledPixmapArgument*>(data);
        arg->pixmap = info ? info->internalQicon().pixmap(arg->size, arg->mode, arg->state) : QPixmap{};
        break;
    }
    }
}

} // namespace Fm

#endif // FM_ICONENGINE_H

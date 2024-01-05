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
#include <private/qicon_p.h>

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
        info->internalQicon().data_ptr()->engine->paint(painter, rect, mode, state);
    }
}

QPixmap IconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) {
    auto info = info_.lock();
    return info ? info->internalQicon().data_ptr()->engine->pixmap(size, mode, state) : QPixmap{};
}

void IconEngine::virtual_hook(int id, void* data) {
    auto info = info_.lock();
    if (info) {
        info->internalQicon().data_ptr()->engine->virtual_hook(id, data);
    }
}

} // namespace Fm

#endif // FM_ICONENGINE_H

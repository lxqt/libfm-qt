/*
 * Copyright (C) 2016 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef __LIBFM_QT_FM_JOB_H__
#define __LIBFM_QT_FM_JOB_H__

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobal>
#include <QThread>
#include <memory>
#include <gio/gio.h>
#include "gobjectptr.h"
#include "libfmqtglobals.h"


namespace Fm2 {


class LIBFM_QT_API Job: public QObject {
    Q_OBJECT
public:

    Job();

    virtual ~Job() {
    }

    bool isCancelled() const {
        return g_cancellable_is_cancelled(cancellable_.get());
    }

    bool runSync();
    void runAsync();

    void setAutoDelete(bool autoDelete) {
        autoDelete_ = autoDelete;
    }

    bool autoDelete() const {
        return autoDelete_;
    }

    bool pause();

    void resume();

Q_SIGNALS:
    void finished();

public Q_SLOTS:

    void cancel() {
        g_cancellable_cancel(cancellable_.get());
    }

protected Q_SLOTS:
    virtual bool run() {
        return true;
    }
/*
protected:
    void emitFinished() {
        QMetaMethod::fromSignal(&Job::finished).invoke(this, Qt::BlockingQueuedConnection);
    }
*/
protected:
    GObjectPtr<GCancellable> cancellable_;

private:
    std::unique_ptr<QThread> thread_;
    bool autoDelete_;
    bool paused_;
};


}

#endif // __LIBFM_QT_FM_JOB_H__

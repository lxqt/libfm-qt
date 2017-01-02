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
#include <QRunnable>
#include <memory>
#include <gio/gio.h>
#include "gobjectptr.h"
#include "libfmqtglobals.h"


namespace Fm2 {

/*
 * Fm2::Job can be used in several different modes.
 * 1. run with QThreadPool::start()
 * 2. call runAsync(), which will create a new QThread and move the object to the thread.
 * 3. create a new QThread, and connect the started() signal to the slot Job::run()
 * 4. Directly call Job::run(), which executes synchrounously as a normal blocking call
*/

class LIBFM_QT_API Job: public QObject, public QRunnable {
    Q_OBJECT
public:

    explicit Job();

    virtual ~Job();

    bool isCancelled() const {
        return g_cancellable_is_cancelled(cancellable_.get());
    }

    void runAsync();

    bool pause();

    void resume();

    const GObjectPtr<GCancellable>& cancellable() const {
        return cancellable_;
    }

Q_SIGNALS:
    void cancelled();
    void finished();

public Q_SLOTS:

    void cancel();

    virtual void run();

private:
    static void _onCancellableCancelled(GCancellable* cancellable, Job* _this) {
        _this->onCancellableCancelled(cancellable);
    }

    void onCancellableCancelled(GCancellable* cancellable) {
        Q_EMIT cancelled();
    }

protected:
    GObjectPtr<GCancellable> cancellable_;
    gulong cancellableHandler_;

private:
    bool paused_;
};


}

#endif // __LIBFM_QT_FM_JOB_H__

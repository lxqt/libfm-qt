#include "job.h"

namespace Fm2 {

Job::Job():
    cancellable_{g_cancellable_new(), false},
    paused_{false} {
    cancellableHandler_ = g_signal_connect(cancellable_.get(), "cancelled", G_CALLBACK(_onCancellableCancelled), this);
}

Job::~Job()
{
    if(cancellable_) {
        g_cancellable_disconnect(cancellable_.get(), cancellableHandler_);
    }
}


void Job::runAsync() {
    auto thread = new QThread();
    moveToThread(thread);
    connect(thread, &QThread::started, this, &Job::run);
    connect(thread, &QThread::finished, this, &Job::finished, Qt::BlockingQueuedConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    if(autoDelete()) {
        connect(thread, &QThread::finished, this, &Job::deleteLater);
    }
    thread->start();
}

void Job::cancel() {
    g_cancellable_cancel(cancellable_.get());
}

void Job::run() {
    Q_EMIT finished();
}

Job::ErrorAction Job::emitError(const GErrorPtr &err, Job::ErrorSeverity severity) {
    ErrorAction response = ErrorAction::CONTINUE;
    // if the error is already handled, don't emit it.
    if(err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_FAILED_HANDLED) {
        return response;
    }
    Q_EMIT error(err, severity, response);

    if(severity == ErrorSeverity::CRITICAL || response == ErrorAction::ABORT) {
        cancel();
    }
    else if(response == ErrorAction::RETRY ) {
        /* If the job is already cancelled, retry is not allowed. */
        if(isCancelled() || (err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_CANCELLED)) {
            response = ErrorAction::CONTINUE;
        }
    }
    return response;
}

} // namespace Fm2

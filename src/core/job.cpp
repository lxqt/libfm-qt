#include "job.h"

namespace Fm2 {

Job::Job():
    cancellable_{g_cancellable_new(), false},
    paused_{false} {
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

void Job::run() {
    Q_EMIT finished();
}

} // namespace Fm2

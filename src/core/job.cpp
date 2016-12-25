#include "job.h"

namespace Fm2 {

Job::Job():
    cancellable_{g_cancellable_new(), false},
    autoDelete_{false},
    paused_{false} {
}

bool Job::runSync() {
    auto ret = run();
    Q_EMIT finished();
    if(autoDelete_)
        delete this;
    return ret;
}

void Job::runAsync() {
    thread_ = std::unique_ptr<QThread>(new QThread());
    moveToThread(thread_.get());
    connect(thread_.get(), &QThread::started, this, &Job::run);
    connect(thread_.get(), &QThread::finished, this, &Job::finished, Qt::BlockingQueuedConnection);
    if(autoDelete_) {
        connect(thread_.get(), &QThread::finished, this, &Job::deleteLater);
    }
    thread_->start();
}

} // namespace Fm2

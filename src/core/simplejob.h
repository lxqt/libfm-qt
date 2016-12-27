#ifndef FM2_SIMPLEJOB_H
#define FM2_SIMPLEJOB_H

#include "job.h"
#include <functional>

namespace Fm2 {

class SimpleJob : public Job {
    Q_OBJECT
public:
    SimpleJob(std::function<void()> func): func_{func} {
    }

public Q_SLOTS:

    void run() override {
        func_();
        Q_EMIT finished();
    }

private:
    std::function<void()> func_;
};

} // namespace Fm2

#endif // FM2_SIMPLEJOB_H

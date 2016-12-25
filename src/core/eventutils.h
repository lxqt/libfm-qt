#ifndef __EVENT_UTILS_H__
#define __EVENT_UTILS_H__

#include <vector>
#include <functional>
#include <algorithm>

namespace Fm2 {

/*
Example:

    std::function<void (FolderListener&, EventSource&)> func = &FolderListener::contentChanged;
    emitEvent(func);

    emitEvent<FolderListener>(&FolderListener::contentChanged);
    std::vector<std::shared_ptr<const FileInfo>> files;

    using namespace std::placeholders;
    emitEvent<FolderListener>(std::bind(&FolderListener::filesAdded, _1, _2, files));

*/

class EventListener {
public:
    EventListener() {}
    virtual ~EventListener() {}
};

class EventSource {
public:
    virtual ~EventSource() {}

    void addListener(EventListener* listener) {
        listeners_.push_back(listener);
    }

    void removeListener(EventListener* listener) {
        std::remove(listeners_.begin(), listeners_.end(), listener);
    }

    template<typename T>
    void emitEvent(std::function<void (T&, EventSource&)> func) {
        for(auto listener: listeners_) {
            func(*static_cast<T*>(listener), *this);
        }
    }

private:
    std::vector<EventListener*> listeners_;
};

} // namespace Fm2

#endif // __EVENT_UTILS_H__

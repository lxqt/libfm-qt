#ifndef FM2_GOBJECTPTR_H
#define FM2_GOBJECTPTR_H

#include <glib.h>
#include <glib-object.h>
#include <cstddef>
#include <QDebug>

namespace Fm2 {

template <typename T>
class GObjectPtr {
public:

    explicit GObjectPtr(): gobj_{nullptr} {
    }

    explicit GObjectPtr(T* gobj, bool add_ref = true): gobj_{gobj} {
        if(gobj_ != nullptr && add_ref)
            g_object_ref(gobj_);
    }

    GObjectPtr(const GObjectPtr& other): GObjectPtr{} {
        *this = other;
    }

    GObjectPtr(GObjectPtr&& other): GObjectPtr{} {
        *this = other;
    }

    ~GObjectPtr() {
        if(gobj_ != nullptr)
            g_object_unref(gobj_);
    }

    T* get() const {
        return gobj_;
    }

    GObjectPtr& operator = (const GObjectPtr& other) {
        if(gobj_ != nullptr)
            g_object_unref(gobj_);
        gobj_ = other.gobj_ ? reinterpret_cast<T*>(g_object_ref(other.gobj_)) : nullptr;
        return *this;
    }

    GObjectPtr& operator = (GObjectPtr&& other) {
        if(gobj_ != nullptr)
            g_object_unref(gobj_);
        gobj_ = other.gobj_ ? reinterpret_cast<T*>(other.gobj_) : nullptr;
        other.gobj_ = nullptr;
        return *this;
    }

    GObjectPtr& operator = (T* gobj) {
        if(gobj_ != nullptr)
            g_object_unref(gobj_);
        gobj_ = gobj ? reinterpret_cast<T*>(g_object_ref(gobj_)) : nullptr;
        return *this;
    }

    bool operator == (const GObjectPtr& other) const {
        return gobj_ == other.gobj_;
    }

    bool operator == (T* gobj) const {
        return gobj_ == gobj;
    }

    bool operator != (std::nullptr_t) const {
        return gobj_ != nullptr;
    }

    operator bool() const {
        return gobj_ != nullptr;
    }

private:
    mutable T* gobj_;
};


} // namespace Fm2

#endif // FM2_GOBJECTPTR_H

#ifndef GSIGNALCONNECTION_H
#define GSIGNALCONNECTION_H

#include <glib.h>
#include <glib-object.h>


namespace Fm2 {

// automatic signal/slot connection management with type safety for GObject

template <class T, class G, class R, class ...Args>
class GSignalHandler {
public:
    typedef R (T::*MemFn)(G*, Args...);

    GSignalHandler(T* obj, MemFn callback): obj_(obj), callback_{callback} {
    }

    ~GSignalHandler() {
        disconnect();
    }

    GSignalHandler(const GSignalHandler& other) = delete;

    GSignalHandler& operator = (GSignalHandler&& other) = delete;

    gulong connect(G* obj, const char* sig) {
        if(isConnected() || !obj)
            return 0;
        g_object_weak_ref(gobj_, onGObjectDestroyed, this);
        return g_signal_connect(obj, sig, G_CALLBACK(&stub), this);
    }

    guint disconnect() {
        if(isConnected()) {
            guint ret = g_signal_handlers_disconnect_by_func(gobj_, reinterpret_cast<void*>(&stub), this);
            g_object_weak_unref(gobj_, onGObjectDestroyed, this);
            gobj_ = nullptr;
            return ret;
        }
        return 0;
    }

    bool isConnected() const {
        return gobj_ != nullptr;
    }

private:

    static R stub(G* sender, Args... args, void* user_data) {
        auto _this = reinterpret_cast<GSignalHandler*>(user_data);
        return (_this->obj_->*_this->callback_)(sender, args...);
    }

    static void onGObjectDestroyed(gpointer user_data, GObject* obj) {
        GSignalHandler* _this = reinterpret_cast<GSignalHandler*>(user_data);
        _this->gobj_ = nullptr;
    }

    GObject* gobj_;
    T* obj_;
    MemFn callback_; // member function pointer
};

} // namespace Fm2

#endif // GSIGNALCONNECTION_H

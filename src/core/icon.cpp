#include "icon.h"

namespace Fm2 {

std::unordered_map<GIcon*, std::shared_ptr<Icon>, Icon::GIconHash, Icon::GIconEqual> Icon::cache_;
std::mutex Icon::mutex_;


Icon::Icon(const char* name):
    gicon_{g_themed_icon_new(name), false} {
}

Icon::Icon(GObjectPtr<GIcon>& gicon):
    gicon_{gicon} {
}

Icon::~Icon() {
}

// static
std::shared_ptr<const Icon> Icon::fromName(const char* name) {
    GObjectPtr<GIcon> gicon{g_themed_icon_new(name), false};
    return fromGIcon(gicon);
}

// static
std::shared_ptr<const Icon> Icon::fromGIcon(GObjectPtr<GIcon>& gicon) {
    std::lock_guard<std::mutex> lock{mutex_};
    auto it = cache_.find(gicon.get());
    if(it != cache_.end()) {
        return it->second;
    }
    // not found in the cache, create a new entry for it.
    auto icon = std::make_shared<Icon>(gicon);
    cache_.insert(std::make_pair(icon->gicon_.get(), icon));
    return icon;
}

void Icon::unloadCache() {
    std::lock_guard<std::mutex> lock{mutex_};
    // cache_.clear();

}

} // namespace Fm2

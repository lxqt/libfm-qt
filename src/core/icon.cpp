#include "icon.h"

namespace Fm2 {

std::unordered_map<GIcon*, std::shared_ptr<Icon>, Icon::GIconHash, Icon::GIconEqual> Icon::cache_;
std::mutex Icon::mutex_;


Icon::Icon(const char* name):
    gicon_{g_themed_icon_new(name), false} {
}

Icon::Icon(const GIconPtr gicon):
    gicon_{std::move(gicon)} {
}

Icon::~Icon() {
}

// static
std::shared_ptr<const Icon> Icon::fromName(const char* name) {
    GObjectPtr<GIcon> gicon{g_themed_icon_new(name), false};
    return fromGIcon(gicon);
}

// static
std::shared_ptr<const Icon> Icon::fromGIcon(GObjectPtr<GIcon> gicon) {
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

QIcon Icon::qicon() const {
    if(qicon_.isNull()) {
        GIcon* gicon = gicon_.get();
        if(G_IS_EMBLEMED_ICON(gicon_.get())) {
            gicon = g_emblemed_icon_get_icon(G_EMBLEMED_ICON(gicon));
        }
        if(G_IS_THEMED_ICON(gicon)) {
            const gchar* const* names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
            qicon_ = qiconFromNames(names);
        }
        else if(G_IS_FILE_ICON(gicon)) {
            GFile* file = g_file_icon_get_file(G_FILE_ICON(gicon));
            char* fpath = g_file_get_path(file);
            qicon_ = QIcon(fpath);
            g_free(fpath);
        }
        if(!qicon_.isNull()) {
            return qicon_;
        }
    }
    return qicon_; // FIXME: return fallack icon instead
}

QIcon Icon::qiconFromNames(const char* const* names) {
    const gchar* const* name;
    // qDebug("names: %p", names);
    for(name = names; *name; ++name) {
        // qDebug("icon name=%s", *name);
        QIcon qicon = QIcon::fromTheme(*name);
        if(!qicon.isNull()) {
            return qicon;
        }
    }
    return QIcon();
}

std::forward_list<std::shared_ptr<const Icon>> Icon::emblems() const {
    std::forward_list<std::shared_ptr<const Icon>> result;
    if(hasEmblems()) {
        const GList* emblems_glist = g_emblemed_icon_get_emblems(G_EMBLEMED_ICON(gicon_.get()));
        for(auto l = emblems_glist; l; l = l->next) {
            auto gemblem = G_EMBLEM(l->data);
            GIconPtr gemblem_icon{g_emblem_get_icon(gemblem), true};
            result.emplace_front(fromGIcon(gemblem_icon));
        }
        result.reverse();
    }
    return result;
}

} // namespace Fm2

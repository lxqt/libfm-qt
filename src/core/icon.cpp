#include "icon.h"

namespace Fm2 {

std::unordered_map<GIcon*, std::shared_ptr<Icon>, Icon::GIconHash, Icon::GIconEqual> Icon::cache_;
std::mutex Icon::mutex_;


Icon::Icon(const char* name):
    gicon_{g_themed_icon_new(name), false} {
}

Icon::Icon(const GObjectPtr<GIcon> &gicon):
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
std::shared_ptr<const Icon> Icon::fromGIcon(const GObjectPtr<GIcon>& gicon) {
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
        if(G_IS_THEMED_ICON(gicon_.get())) {
          const gchar * const * names = g_themed_icon_get_names(G_THEMED_ICON(gicon_.get()));
          qicon_ = qiconFromNames(names);
        }
        else if(G_IS_FILE_ICON(gicon_.get())) {
          GFile* file = g_file_icon_get_file(G_FILE_ICON(gicon_.get()));
          char* fpath = g_file_get_path(file);
          qicon_ = QIcon(fpath);
          g_free(fpath);
        }
        if(!qicon_.isNull())
          return qicon_;
    }
    return qicon_; // FIXME: return fallack icon instead
}

QIcon Icon::qiconFromNames(const char * const *names) {
    const gchar* const* name;
    // qDebug("names: %p", names);
    for(name = names; *name; ++name) {
        // qDebug("icon name=%s", *name);
        QString qname = *name;
        QIcon qicon = QIcon::fromTheme(qname);
        if(!qicon.isNull()) {
            return qicon;
        }
    }
    return QIcon();
}

} // namespace Fm2

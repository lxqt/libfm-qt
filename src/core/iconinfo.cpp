#include "iconinfo.h"
#include "iconinfo_p.h"

namespace Fm2 {

std::unordered_map<GIcon*, std::shared_ptr<IconInfo>, IconInfo::GIconHash, IconInfo::GIconEqual> IconInfo::cache_;
std::mutex IconInfo::mutex_;
QIcon IconInfo::fallbackQicon_;

static const char* fallbackIconNames[] = {
    "unknown",
    "application-octet-stream",
    "application-x-generic",
    "text-x-generic",
    nullptr
};

IconInfo::IconInfo(const char* name):
    gicon_{g_themed_icon_new(name), false} {
}

IconInfo::IconInfo(const GIconPtr gicon):
    gicon_{std::move(gicon)} {
}

IconInfo::~IconInfo() {
}

// static
std::shared_ptr<const IconInfo> IconInfo::fromName(const char* name) {
    GObjectPtr<GIcon> gicon{g_themed_icon_new(name), false};
    return fromGIcon(gicon);
}

// static
std::shared_ptr<const IconInfo> IconInfo::fromGIcon(GIconPtr gicon) {
    if(Q_LIKELY(gicon)) {
        std::lock_guard<std::mutex> lock{mutex_};
        auto it = cache_.find(gicon.get());
        if(it != cache_.end()) {
            return it->second;
        }
        // not found in the cache, create a new entry for it.
        auto icon = std::make_shared<IconInfo>(std::move(gicon));
        cache_.insert(std::make_pair(icon->gicon_.get(), icon));
        return icon;
    }
    return std::shared_ptr<const IconInfo>{};
}

void IconInfo::updateQIcons() {
    std::lock_guard<std::mutex> lock{mutex_};
    fallbackQicon_ = QIcon();
    for(auto& elem: cache_) {
        auto& info = elem.second;
        info->internalQicon_ = QIcon();
    }
}

QIcon IconInfo::qicon() const {
    if(Q_UNLIKELY(qicon_.isNull() && gicon_)) {
        if(!G_IS_FILE_ICON(gicon_.get())) {
            qicon_ = QIcon(new IconEngine{shared_from_this()});
        }
        else {
            qicon_ = internalQicon_;
        }
    }
    return qicon_;
}

QIcon IconInfo::qiconFromNames(const char* const* names) {
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

std::forward_list<std::shared_ptr<const IconInfo>> IconInfo::emblems() const {
    std::forward_list<std::shared_ptr<const IconInfo>> result;
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

QIcon IconInfo::internalQicon() const {
    if(Q_UNLIKELY(internalQicon_.isNull())) {
        GIcon* gicon = gicon_.get();
        if(G_IS_EMBLEMED_ICON(gicon_.get())) {
            gicon = g_emblemed_icon_get_icon(G_EMBLEMED_ICON(gicon));
        }
        if(G_IS_THEMED_ICON(gicon)) {
            const gchar* const* names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
            internalQicon_ = qiconFromNames(names);
        }
        else if(G_IS_FILE_ICON(gicon)) {
            GFile* file = g_file_icon_get_file(G_FILE_ICON(gicon));
            CStrPtr fpath{g_file_get_path(file)};
            internalQicon_ = QIcon(fpath.get());
        }

        // fallback to default icon
        if(Q_UNLIKELY(internalQicon_.isNull())) {
            if(Q_UNLIKELY(fallbackQicon_.isNull())) {
                fallbackQicon_ = qiconFromNames(fallbackIconNames);
            }
            internalQicon_ = fallbackQicon_;
        }
    }
    return internalQicon_;
}

} // namespace Fm2

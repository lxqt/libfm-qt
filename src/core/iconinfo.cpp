#include "iconinfo.h"
#include "iconinfo_p.h"

namespace Fm {

std::unordered_map<GIcon*, std::shared_ptr<IconInfo>, IconInfo::GIconHash, IconInfo::GIconEqual> IconInfo::cache_;
std::mutex IconInfo::mutex_;
QList<QIcon> IconInfo::fallbackQicons_;

static const char* fallbackIconNames[] = {
    "unknown",
    "application-octet-stream",
    "application-x-generic",
    "text-x-generic",
    nullptr
};

static QIcon getFirst(const QList<QIcon> & icons)
{
    for (const auto & icon : icons) {
        if (!icon.isNull())
            return icon;
    }
    return QIcon{};
}

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
    for(auto& elem: cache_) {
        auto& info = elem.second;
        info->internalQicons_.clear();
    }
}

QIcon IconInfo::qicon(const bool& transparent) const {
    if(Q_LIKELY(!transparent)) {
        if(Q_UNLIKELY(qicon_.isNull() && gicon_)) {
            if(!G_IS_FILE_ICON(gicon_.get())) {
                qicon_ = QIcon(new IconEngine{shared_from_this()});
            }
            else {
                qicon_ = getFirst(internalQicons_);
            }
        }
    }
    else { // transparent == true
        if(Q_UNLIKELY(qiconTransparent_.isNull() && gicon_)) {
            if(!G_IS_FILE_ICON(gicon_.get())) {
                qiconTransparent_ = QIcon(new IconEngine{shared_from_this(), transparent});
            }
            else {
                qiconTransparent_ = getFirst(internalQicons_);
            }
        }
    }
    return !transparent ? qicon_ : qiconTransparent_;
}

QList<QIcon> IconInfo::qiconsFromNames(const char* const* names) {
    QList<QIcon> icons;
    // qDebug("names: %p", names);
    for(const gchar* const* name = names; *name; ++name) {
        // qDebug("icon name=%s", *name);
        icons.push_back(QIcon::fromTheme(*name));
    }
    return icons;
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
    QIcon ret_icon;
    if(Q_UNLIKELY(internalQicons_.isEmpty())) {
        GIcon* gicon = gicon_.get();
        if(G_IS_EMBLEMED_ICON(gicon_.get())) {
            gicon = g_emblemed_icon_get_icon(G_EMBLEMED_ICON(gicon));
        }
        if(G_IS_THEMED_ICON(gicon)) {
            const gchar* const* names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
            internalQicons_ = qiconsFromNames(names);
        }
        else if(G_IS_FILE_ICON(gicon)) {
            GFile* file = g_file_icon_get_file(G_FILE_ICON(gicon));
            CStrPtr fpath{g_file_get_path(file)};
            internalQicons_.push_back(QIcon(fpath.get()));
        }

    }

    ret_icon = getFirst(internalQicons_);

    // fallback to default icon
    if(Q_UNLIKELY(ret_icon.isNull())) {
        if(Q_UNLIKELY(fallbackQicons_.isEmpty())) {
            fallbackQicons_ = qiconsFromNames(fallbackIconNames);
        }
        ret_icon = getFirst(fallbackQicons_);
    }
    return ret_icon;
}

} // namespace Fm

#include "userinfocache.h"
#include <pwd.h>
#include <grp.h>

namespace Fm2 {

UserInfoCache* UserInfoCache::globalInstance_ = nullptr;

UserInfoCache::UserInfoCache() : QObject() {
}

const std::shared_ptr<const UserInfo>& UserInfoCache::userFromId(uid_t uid) {
    auto it = users_.find(uid);
    if(it != users_.end())
        return it->second;
    std::shared_ptr<const UserInfo> user;
    auto pw = getpwuid(uid);
    if(pw)
        user = std::make_shared<UserInfo>(uid, pw->pw_name, pw->pw_gecos);
    return (users_[uid] = user);
}

const std::shared_ptr<const GroupInfo>& UserInfoCache::groupFromId(gid_t gid) {
    auto it = groups_.find(gid);
    if(it != groups_.end())
        return it->second;
    std::shared_ptr<const GroupInfo> group;
    auto gr = getgrgid(gid);
    if(gr)
        group = std::make_shared<GroupInfo>(gid, gr->gr_name);
    return (groups_[gid] = group);
}

// static
UserInfoCache* UserInfoCache::instance() {
   if(!globalInstance_)
       globalInstance_ = new UserInfoCache();
   return globalInstance_;
}


} // namespace Fm2

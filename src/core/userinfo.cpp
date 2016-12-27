#include "userinfo.h"
#include <pwd.h>
#include <grp.h>

namespace Fm2 {

UserInfo* UserInfo::globalInstance_ = nullptr;

UserInfo::UserInfo() : QObject() {
}

const std::shared_ptr<const User>& UserInfo::userFromId(uid_t uid) {
    auto it = users_.find(uid);
    if(it != users_.end())
        return it->second;
    std::shared_ptr<const User> user;
    auto pw = getpwuid(uid);
    if(pw)
        user = std::make_shared<User>(uid, pw->pw_name, pw->pw_gecos);
    return (users_[uid] = user);
}

const std::shared_ptr<const Group>& UserInfo::groupFromId(gid_t gid) {
    auto it = groups_.find(gid);
    if(it != groups_.end())
        return it->second;
    std::shared_ptr<const Group> group;
    auto gr = getgrgid(gid);
    if(gr)
        group = std::make_shared<Group>(gid, gr->gr_name);
    return (groups_[gid] = group);
}

// static
UserInfo* UserInfo::instance() {
   if(!globalInstance_)
       globalInstance_ = new UserInfo();
   return globalInstance_;
}


} // namespace Fm2

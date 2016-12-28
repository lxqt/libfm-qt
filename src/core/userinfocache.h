#ifndef FM2_USERINFOCACHE_H
#define FM2_USERINFOCACHE_H

#include <QObject>
#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <memory>

namespace Fm2 {

class UserInfo {
public:
    UserInfo(uid_t uid, const char* name, const char* realName):
        uid_{uid}, name_{name}, realName_{realName} {
    }

    uid_t uid() const {
        return uid_;
    }

    const std::string& name() const {
        return name_;
    }

    const QString& realName() const {
        return realName_;
    }

private:
    uid_t uid_;
    std::string name_;
    QString realName_;

};

class GroupInfo {
public:
    GroupInfo(gid_t gid, const char* name): gid_{gid}, name_{name} {
    }

    gid_t gid() const {
        return gid_;
    }

    const std::string& name() const {
        return name_;
    }

private:
    gid_t gid_;
    std::string name_;
};

class UserInfoCache : public QObject {
    Q_OBJECT
public:
    explicit UserInfoCache();

    const std::shared_ptr<const UserInfo>& userFromId(uid_t uid);

    const std::shared_ptr<const GroupInfo>& groupFromId(gid_t gid);

    static UserInfoCache* instance();

Q_SIGNALS:
    void changed();

private:
    std::unordered_map<uid_t, std::shared_ptr<const UserInfo>> users_;
    std::unordered_map<gid_t, std::shared_ptr<const GroupInfo>> groups_;
    static UserInfoCache* globalInstance_;
};

} // namespace Fm2

#endif // FM2_USERINFOCACHE_H

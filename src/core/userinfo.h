#ifndef FM2_USERINFO_H
#define FM2_USERINFO_H

#include <QObject>
#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <memory>

namespace Fm2 {

class User {
public:
    User(uid_t uid, const char* name, const char* realName):
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

class Group {
public:
    Group(gid_t gid, const char* name): gid_{gid}, name_{name} {
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

class UserInfo : public QObject {
    Q_OBJECT
public:
    explicit UserInfo();

    const std::shared_ptr<const User>& userFromId(uid_t uid);

    const std::shared_ptr<const Group>& groupFromId(gid_t gid);

    static UserInfo* instance();

Q_SIGNALS:
    void changed();

private:
    std::unordered_map<uid_t, std::shared_ptr<const User>> users_;
    std::unordered_map<gid_t, std::shared_ptr<const Group>> groups_;
    static UserInfo* globalInstance_;
};

} // namespace Fm2

#endif // FM2_USERINFO_H

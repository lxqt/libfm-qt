#ifndef FM2_BOOKMARKS_H
#define FM2_BOOKMARKS_H

#include <QObject>

namespace Fm2 {

class Bookmarks : public QObject {
    Q_OBJECT
public:
    explicit Bookmarks(QObject* parent = 0);

Q_SIGNALS:

public Q_SLOTS:
};

} // namespace Fm2

#endif // FM2_BOOKMARKS_H

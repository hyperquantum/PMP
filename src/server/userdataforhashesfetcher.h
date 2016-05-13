/*
    Copyright (C) 2016, Kevin Andre <hyperquantum@gmail.com>

    This file is part of PMP (Party Music Player).

    PMP is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    PMP is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along
    with PMP.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PMP_USERDATAFORHASHESFETCHER_H
#define PMP_USERDATAFORHASHESFETCHER_H

#include "common/filehash.h"

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QRunnable>

namespace PMP {

    class Resolver;

    struct UserDataForHash {
        FileHash hash;
        QDateTime previouslyHeard;
    };

    class UserDataForHashesFetcher : public QObject, public QRunnable {
        Q_OBJECT
    public:
        UserDataForHashesFetcher(quint32 userId, QList<FileHash> hashes,
                                 Resolver &resolver);

        void run();

    Q_SIGNALS:
        void finishedWithResult(quint32 userId, QList<PMP::UserDataForHash> results);

    private:
        quint32 _userId;
        QList<FileHash> _hashes;
        Resolver& _resolver;
    };
}

Q_DECLARE_METATYPE(PMP::UserDataForHash)

#endif

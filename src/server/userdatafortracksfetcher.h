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

#ifndef PMP_USERDATAFORTRACKSFETCHER_H
#define PMP_USERDATAFORTRACKSFETCHER_H

#include <QDateTime>
#include <QObject>
#include <QRunnable>
#include <QVector>

namespace PMP {

    struct UserDataForHashId {
        uint hashId;
        QDateTime previouslyHeard;
        qint16 score;
    };

    class UserDataForTracksFetcher : public QObject, public QRunnable {
        Q_OBJECT
    public:
        UserDataForTracksFetcher(quint32 userId, QVector<uint> hashIds);

        void run();

    Q_SIGNALS:
        void finishedWithResult(quint32 userId, QVector<PMP::UserDataForHashId> results);

    private:
        quint32 _userId;
        QVector<uint> _hashIds;
    };

}

Q_DECLARE_METATYPE(PMP::UserDataForHashId)

#endif

/*
    Copyright (C) 2014-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_DATABASERECORDS_H
#define PMP_DATABASERECORDS_H

#include <QByteArray>
#include <QDateTime>
#include <QString>

namespace PMP::Server
{
    namespace DatabaseRecords
    {
        struct HashHistoryStats
        {
            uint lastHistoryId;
            quint32 hashId;
            quint32 scoreHeardCount;
            QDateTime lastHeard;
            qint16 averagePermillage;
        };

        class User
        {
        public:
            User()
             : id(0), login("")
            {
                //
            }

            User(quint32 id, QString login, QByteArray salt, QByteArray password)
             : id(id), login(login), salt(salt), password(password)
            {
                //
            }

            static User fromDb(quint32 id, QString login, QString salt, QString password)
            {
                return User(
                    id, login,
                    QByteArray::fromBase64(salt.toLatin1()),
                    QByteArray::fromBase64(password.toLatin1())
                );
            }

            quint32 id;
            QString login;
            QByteArray salt;
            QByteArray password;
        };

        class UserDynamicModePreferences
        {
        public:
            UserDynamicModePreferences()
             : dynamicModeEnabled(true),
               trackRepetitionAvoidanceIntervalSeconds(3600 /*1hr*/)
            {
                //
            }

            bool dynamicModeEnabled;
            qint32 trackRepetitionAvoidanceIntervalSeconds;
        };

        class LastFmScrobblingDataRecord
        {
        public:
            LastFmScrobblingDataRecord()
                : enableLastFmScrobbling(false), lastFmScrobbledUpTo(0)
            {
                //
            }

            bool enableLastFmScrobbling;
            QString lastFmUser;
            QString lastFmSessionKey;
            quint32 lastFmScrobbledUpTo;
        };

        class UserScrobblingDataRecord : public LastFmScrobblingDataRecord
        {
        public:
            UserScrobblingDataRecord()
                : userId(0)
            {
                //
            }

            quint32 userId;
        };

        class HistoryRecord
        {
        public:
            HistoryRecord()
                : id(0), hashId(0), userId(0), permillage(-1), validForScoring(false)
            {
                //
            }

            quint32 id;
            quint32 hashId;
            quint32 userId;
            QDateTime start;
            QDateTime end;
            qint16 permillage;
            bool validForScoring;
        };
    }
}
#endif

/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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
#include <QString>

namespace PMP
{
    namespace DatabaseRecords
    {
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
    }
}
#endif

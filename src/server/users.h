/*
    Copyright (C) 2015-2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_USERS_H
#define PMP_USERS_H

#include "common/networkprotocol.h"

#include "database.h"  // for User :-/

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QtGlobal>

namespace PMP
{
    typedef QPair<quint32, QString> UserIdAndLogin;

    class Users : public QObject
    {
        Q_OBJECT
    public:
        enum ErrorCode
        {
            Successfull = 0,
            InvalidAccountName = 1,
            AccountAlreadyExists = 2,
            DatabaseProblem = 3
        };

        Users();

        QList<UserIdAndLogin> getUsers();
        QString getUserLogin(quint32 userId) const;
        bool getUserByLogin(QString login, User& user);
        static bool checkUserLoginPassword(User const& user,
                                           QByteArray const& sessionSalt,
                                           QByteArray const& hashedPassword);

        static QByteArray generateSalt();
        static ErrorCode generateSaltForNewAccount(QString accountName,
                                                   QByteArray& salt);

        QPair<ErrorCode, quint32> registerNewAccount(QString accountName,
                                                     QByteArray const& salt,
                                                     QByteArray const& hashedPassword);

        static NetworkProtocol::ErrorType toNetworkProtocolError(ErrorCode code);

    private:
        void loadUsers();

        QHash<quint32, User> _usersById;
        QHash<QString, quint32> _userIdsByLogin;
    };
}
#endif

/*
    Copyright (C) 2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "users.h"

#include "database.h"

#include <QtGlobal>

namespace PMP {

    Users::Users()
    {
        loadUsers();
    }

    void Users::loadUsers() {
        Database* db = Database::instance();
        if (!db) return;

        _users = db->getUsers();
    }

    QList<UserIdAndLogin> Users::getUsers() {
        QList<UserIdAndLogin> result;

        Q_FOREACH(User u, _users) {
            result.append(UserIdAndLogin(u.id, u.login));
        }

        return result;
    }

    QByteArray Users::generateSalt() {
        QByteArray buffer;
        do {
            buffer.append((char)(qrand() % 256));
        } while (buffer.size() < 24);

        //buffer.truncate(24);
        return buffer;
    }

    Users::ErrorCode Users::generateSaltForNewAccount(QString accountName,
                                                      QByteArray& salt)
    {
        QString account = accountName.trimmed();

        if (account != accountName
            || account.length() == 0 || account.length() > 63)
        {
            return InvalidAccountName;
        }

        Database* db = Database::instance();
        if (db) {
            if (db->checkUserExists(accountName)) {
                return AccountAlreadyExists;
            }
        }
        else {
            return DatabaseProblem;
        }

        salt = generateSalt();

        return Successfull;
    }

    QPair<Users::ErrorCode, quint32> Users::registerNewAccount(QString accountName,
                                                               QByteArray const& salt,
                                                        QByteArray const& hashedPassword)
    {
        QString account = accountName.trimmed();

        if (account != accountName
            || account.length() == 0 || account.length() > 63)
        {
            return QPair<Users::ErrorCode, quint32>(InvalidAccountName, 0);
        }

        Database* db = Database::instance();
        if (db) {
            if (db->checkUserExists(accountName)) {
                return QPair<Users::ErrorCode, quint32>(AccountAlreadyExists, 0);
            }
        }
        else {
            return QPair<Users::ErrorCode, quint32>(DatabaseProblem, 0);
        }

        User u(0, accountName, salt, hashedPassword);

        u.id = db->registerNewUser(u);
        if (u.id <= 0)
            return QPair<ErrorCode, quint32>(DatabaseProblem, 0);

        loadUsers(); /* reload users list */

        return QPair<ErrorCode, quint32>(Successfull, u.id);
    }

    NetworkProtocol::ErrorType Users::toNetworkProtocolError(ErrorCode code) {
        switch (code) {
        case Successfull:
            return NetworkProtocol::NoError;
        case InvalidAccountName:
            return NetworkProtocol::InvalidUserAccountName;
        case AccountAlreadyExists:
            return NetworkProtocol::UserAccountAlreadyExists;
        case DatabaseProblem:
            return NetworkProtocol::DatabaseProblem;
        default:
            return NetworkProtocol::UnknownError;
        }
    }
}

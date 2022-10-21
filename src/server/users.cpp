/*
    Copyright (C) 2015-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/networkprotocol.h"

#include "database.h"

#include <QRandomGenerator>
#include <QtGlobal>

namespace PMP
{
    Users::Users()
    {
        loadUsers();
    }

    void Users::loadUsers()
    {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db) return;

        auto usersOrError = db->getUsers();
        if (usersOrError.failed())
        {
            qWarning() << "failed to load users";
            return;
        }

        auto const users = usersOrError.result();

        _usersById.clear();
        _userIdsByLogin.clear();
        _usersById.reserve(users.size());
        _userIdsByLogin.reserve(users.size());
        for (auto& u : users)
        {
            _usersById.insert(u.id, u);
            _userIdsByLogin.insert(u.login.toLower(), u.id);
        }

        qDebug() << "Users: loaded" << users.size() << "users";
    }

    QVector<UserIdAndLogin> Users::getUsers()
    {
        QVector<UserIdAndLogin> result;
        result.reserve(_usersById.size());

        for (auto& u : qAsConst(_usersById))
        {
            result.append(UserIdAndLogin(u.id, u.login));
        }

        return result;
    }

    bool Users::checkUserIdExists(quint32 userId) const
    {
        return _usersById.contains(userId);
    }

    QString Users::getUserLogin(quint32 userId) const
    {
        auto it = _usersById.find(userId);
        if (it == _usersById.end()) return ""; /* not found */

        return it->login;
    }

    bool Users::getUserByLogin(QString login, DatabaseRecords::User& user)
    {
        quint32 id = _userIdsByLogin.value(login.toLower(), 0);
        if (id == 0) return false;

        user = _usersById.value(id);
        return true;
    }

    bool Users::checkUserLoginPassword(const DatabaseRecords::User& user,
                                       const QByteArray &sessionSalt,
                                       const QByteArray& hashedPassword)
    {
        QByteArray expected =
            NetworkProtocol::hashPasswordForSession(sessionSalt, user.password);

        return hashedPassword == expected;
    }

    QByteArray Users::generateSalt()
    {
        auto* randomGenerator = QRandomGenerator::global();

        QByteArray buffer;
        do
        {
            buffer.append(char(randomGenerator->bounded(256)));
        }
        while (buffer.size() < 24);

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

        auto db = Database::getDatabaseForCurrentThread();
        if (db)
        {
            auto userExistsOrError = db->checkUserExists(accountName);
            if (userExistsOrError.failed())
                return DatabaseProblem;

            if (userExistsOrError.result() == true)
                return AccountAlreadyExists;
        }
        else
        {
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

        /* Account name length must fit in a byte when using UTF-8 (that's why its length
         *  must be <= 63), it cannot start or end with whitespace, and the name 'Public'
         * is reserved to prevent confusion between public and personal mode.
         */
        if (account != accountName
            || account.length() == 0 || account.length() > 63
            || account.compare("PUBLIC", Qt::CaseInsensitive) == 0)
        {
            return QPair<Users::ErrorCode, quint32>(InvalidAccountName, 0);
        }

        auto db = Database::getDatabaseForCurrentThread();
        if (db)
        {
            auto userExistsOrError = db->checkUserExists(accountName);
            if (userExistsOrError.failed())
                return QPair<Users::ErrorCode, quint32>(AccountAlreadyExists, 0);

            if (userExistsOrError.result() == true)
                return QPair<Users::ErrorCode, quint32>(AccountAlreadyExists, 0);
        }
        else
        {
            return QPair<Users::ErrorCode, quint32>(DatabaseProblem, 0);
        }

        DatabaseRecords::User u(0, accountName, salt, hashedPassword);

        auto newUserIdOrError = db->registerNewUser(u);
        if (newUserIdOrError.failed())
            return QPair<ErrorCode, quint32>(DatabaseProblem, 0);

        u.id = newUserIdOrError.result();

        loadUsers(); /* reload users list */

        return QPair<ErrorCode, quint32>(Successfull, u.id);
    }

    ResultMessageErrorCode Users::toNetworkProtocolError(ErrorCode code)
    {
        switch (code)
        {
        case Successfull:
            return ResultMessageErrorCode::NoError;

        case InvalidAccountName:
            return ResultMessageErrorCode::InvalidUserAccountName;

        case AccountAlreadyExists:
            return ResultMessageErrorCode::UserAccountAlreadyExists;

        case DatabaseProblem:
            return ResultMessageErrorCode::DatabaseProblem;
        }

        return ResultMessageErrorCode::UnknownError;
    }
}

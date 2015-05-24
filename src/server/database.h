/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_DATABASE_H
#define PMP_DATABASE_H

#include "common/hashid.h"

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QSqlDatabase>
#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QSqlQuery)
QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace PMP {

    class FileData;

    class User {
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

        static User fromDb(quint32 id, QString login, QString salt, QString password) {
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

    class Database : public QObject {
        Q_OBJECT
    public:
        static bool init(QTextStream& out);

        static Database* instance() { return _instance; }

        void registerHash(const HashID& hash);
        uint getHashID(const HashID& hash);
        QList<QPair<uint,HashID> > getHashes(uint largerThanID = 0);

        void registerFilename(uint hashID, const QString& filenameWithoutPath);
        QList<QString> getFilenames(uint hashID);

        QList<User> getUsers();
        bool checkUserExists(QString userName);
        quint32 registerNewUser(User& user);

        void addToHistory(quint32 hashId, quint32 userId, QDateTime start, QDateTime end,
                          int permillage, bool validForScoring);

    private:
        bool executeScalar(QSqlQuery& q, int& i, int defaultValue = 0);
        bool executeScalar(QSqlQuery& q, uint& i, uint defaultValue = 0);
        bool executeScalar(QSqlQuery& q, QString& s, const QString& defaultValue = "");

        bool executeQuery(QSqlQuery& q);

        static Database* _instance;

        QSqlDatabase _db;
    };
}
#endif

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

#ifndef PMP_DATABASE_H
#define PMP_DATABASE_H

#include "common/filehash.h"
#include "common/resultorerror.h"

#include <QAtomicInt>
#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QPair>
#include <QSharedPointer>
#include <QString>
#include <QSqlDatabase>
#include <QtGlobal>
#include <QThreadStorage>
#include <QUuid>
#include <QVector>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QSqlQuery)
QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace PMP
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

    class UserDynamicModePreferencesRecord
    {
    public:
        UserDynamicModePreferencesRecord()
         : dynamicModeEnabled(true), trackRepetitionAvoidanceIntervalSeconds(3600 /*1hr*/)
        {
            //
        }

        bool dynamicModeEnabled;
        qint32 trackRepetitionAvoidanceIntervalSeconds;
    };

    struct DatabaseConnectionSettings;
    class ServerSettings;

    class Database
    {
    public:
        struct HashHistoryStats
        {
            quint32 hashId;
            quint32 scoreHeardCount;
            QDateTime lastHeard;
            qint16 score;
        };

        static bool init(QTextStream& out, ServerSettings const& serverSettings);
        static bool init(QTextStream& out,
                         DatabaseConnectionSettings const& connectionSettings);

        bool isConnectionOpen() const;

        void registerHash(const FileHash& hash);
        uint getHashID(const FileHash& hash);
        ResultOrError<QList<QPair<uint,FileHash>>, FailureType> getHashes(
                                                                   uint largerThanID = 0);

        bool registerFilenameSeen(uint hashId, const QString& filenameWithoutPath,
                                  int currentYear);
        ResultOrError<QList<QString>, FailureType> getFilenames(uint hashID);

        void registerFileSizeSeen(uint hashId, qint64 size, int currentYear);
        ResultOrError<QList<qint64>, FailureType> getFileSizes(uint hashID);

        QList<User> getUsers();
        bool checkUserExists(QString userName);
        quint32 registerNewUser(User& user);

        UserDynamicModePreferencesRecord getUserDynamicModePreferences(quint32 userId,
                                                                       bool* ok);
        bool setUserDynamicModePreferences(quint32 userId,
                                     UserDynamicModePreferencesRecord const& preferences);

        void addToHistory(quint32 hashId, quint32 userId, QDateTime start, QDateTime end,
                          int permillage, bool validForScoring);
        QDateTime getLastHeard(quint32 hashId, quint32 userId);
        QList<QPair<quint32, QDateTime>> getLastHeard(quint32 userId,
                                                      QList<quint32> hashIds);
        QVector<HashHistoryStats> getHashHistoryStats(quint32 userId,
                                                      QList<quint32> hashIds);

        static QSharedPointer<Database> getDatabaseForCurrentThread();
        static QUuid getDatabaseUuid();

    private:
        Database(QSqlDatabase db);

        QString buildParamsList(unsigned paramsCount);

        std::function<void (QSqlQuery&)> prepareSimple(QString sql);

        bool executeScalar(std::function<void (QSqlQuery&)> preparer,
                           bool& b, bool defaultValue);
        bool executeScalar(std::function<void (QSqlQuery&)> preparer,
                           int& i, int defaultValue);
        bool executeScalar(std::function<void (QSqlQuery&)> preparer,
                           uint& i, uint defaultValue);
        bool executeScalar(std::function<void (QSqlQuery&)> preparer,
                           QDateTime& d);

        bool executeVoid(std::function<void (QSqlQuery&)> preparer);

        bool executeQuery(std::function<void (QSqlQuery&)> preparer,
                          bool processResult,
                          std::function<void (QSqlQuery&)> resultFetcher);
        bool executeQuery(QSqlQuery& q);

        static bool addColumnIfNotExists(QSqlQuery& q, QString tableName,
                                         QString columnName, QString type);

        static bool getBool(QVariant v, bool nullValue);
        static int getInt(QVariant v, int nullValue);
        static uint getUInt(QVariant v, uint nullValue);
        static QDateTime getUtcDateTime(QVariant v);

        static qint16 calculateScore(qint32 permillageFromDB, quint32 heardCount);

        static QSqlDatabase createDatabaseConnection(QString name, bool setSchema);
        static void printInitializationError(QTextStream& out, QSqlDatabase& db);

        static bool initMiscTable(QSqlQuery& q);
        static ResultOrError<QUuid, FailureType> initDatabaseUuid(QSqlQuery& q);
        static bool initHashTable(QSqlQuery& q);
        static bool initFilenameTable(QSqlQuery& q);
        static bool initFileSizeTable(QSqlQuery& q);
        static bool initUsersTable(QSqlQuery& q);
        static bool initHistoryTable(QSqlQuery& q);

        static QString _hostname;
        static int _port;
        static QString _username;
        static QString _password;
        static QUuid _uuid;
        static bool _initDoneSuccessfully;
        static QThreadStorage<QSharedPointer<Database>> _threadLocalDatabases;
        static QAtomicInt _nextDbNameNumber;

        QSqlDatabase _db;
    };
}
#endif

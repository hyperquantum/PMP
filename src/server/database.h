/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP {

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

    class LastFmScrobblingDataRecord {
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

    class UserScrobblingDataRecord : public LastFmScrobblingDataRecord {
    public:
        UserScrobblingDataRecord()
         : userId(0)
        {
            //
        }

        quint32 userId;
    };

    class HistoryRecord {
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

    class Database {
    public:
        struct HashHistoryStats {
            quint32 hashId;
            quint32 scoreHeardCount;
            QDateTime lastHeard;
            qint16 score;
        };

        static bool init(QTextStream& out);
        static bool init(QTextStream& out, QString hostname, QString username,
                         QString password);

        bool isConnectionOpen() const;

        QUuid getDatabaseIdentifier() const;

        void registerHash(const FileHash& hash);
        uint getHashID(const FileHash& hash);
        QList<QPair<uint,FileHash> > getHashes(uint largerThanID = 0);

        void registerFilename(uint hashID, const QString& filenameWithoutPath);
        QList<QString> getFilenames(uint hashID);

        void registerFileSize(uint hashId, qint64 size);
        QList<qint64> getFileSizes(uint hashID);

        QList<User> getUsers();
        bool checkUserExists(QString userName);
        quint32 registerNewUser(User& user);

        QVector<UserScrobblingDataRecord> getUsersScrobblingData();
        LastFmScrobblingDataRecord getUserLastFmScrobblingData(quint32 userId);
        bool setLastFmScrobblingEnabled(quint32 userId, bool enabled = true);
        quint32 getLastFmScrobbledUpTo(quint32 userId, bool* ok);
        bool updateLastFmScrobbledUpTo(quint32 userId, quint32 newValue);

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
        QVector<HistoryRecord> getUserHistoryForScrobbling(quint32 userId,
                                                           quint32 startId,
                                                           QDateTime earliestDateTime,
                                                           int limit);

        static QSharedPointer<Database> getDatabaseForCurrentThread();

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
        static QString getString(QVariant v, const char* nullValue);

        static qint16 calculateScore(qint32 permillageFromDB, quint32 heardCount);

        static QSqlDatabase createDatabaseConnection(QString name, bool setSchema);
        static void printInitializationError(QTextStream& out, QSqlDatabase& db);

        static bool initUsersTable(QSqlQuery& q);

        static QString _hostname;
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

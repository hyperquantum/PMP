/*
    Copyright (C) 2014-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "databaserecords.h"

#include "common/filehash.h"
#include "common/resultorerror.h"

#include <QAtomicInt>
#include <QByteArray>
#include <QDateTime>
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

namespace PMP::Server
{
    struct DatabaseConnectionSettings;
    class ServerSettings;

    class Database
    {
    public:
        static bool init(QTextStream& out, ServerSettings const& serverSettings);
        static bool init(QTextStream& out,
                         DatabaseConnectionSettings const& connectionSettings);

        bool isConnectionOpen() const;

        ResultOrError<Nullable<QString>, FailureType> getMiscDataValue(
                                                                    QString const& key);
        ResultOrError<SuccessType, FailureType> insertMiscData(QString const& key,
                                                               QString const& value);
        ResultOrError<SuccessType, FailureType> insertOrUpdateMiscData(QString const& key,
                                                                    QString const& value);
        ResultOrError<SuccessType, FailureType> insertMiscDataIfNotPresent(
                                                                    QString const& key,
                                                                    QString const& value);
        ResultOrError<SuccessType, FailureType> updateMiscDataValueFromSpecific(
                                                                QString const& key,
                                                                QString const& oldValue,
                                                                QString const& newValue);

        ResultOrError<SuccessType, FailureType> registerHash(const FileHash& hash);
        ResultOrError<uint, FailureType> getHashId(const FileHash& hash);
        ResultOrError<QVector<QPair<uint,FileHash>>, FailureType> getHashes(
                                                                   uint largerThanID = 0);

        ResultOrError<SuccessType, FailureType> registerFilenameSeen(uint hashId,
                                                    const QString& filenameWithoutPath,
                                                    int currentYear);
        ResultOrError<QVector<QString>, FailureType> getFilenames(uint hashID);

        ResultOrError<SuccessType, FailureType> registerFileSizeSeen(uint hashId,
                                                                     qint64 size,
                                                                     int currentYear);
        ResultOrError<QVector<qint64>, FailureType> getFileSizes(uint hashID);

        ResultOrError<QVector<DatabaseRecords::User>, FailureType> getUsers();
        ResultOrError<bool, FailureType> checkUserExists(QString userName);
        ResultOrError<quint32, FailureType> registerNewUser(DatabaseRecords::User& user);

        // TODO : use ResultOrError for all return types
        QVector<DatabaseRecords::UserScrobblingDataRecord> getUsersScrobblingData();
        DatabaseRecords::LastFmScrobblingDataRecord getUserLastFmScrobblingData(
                                                                        quint32 userId);
        bool setLastFmScrobblingEnabled(quint32 userId, bool enabled = true);
        quint32 getLastFmScrobbledUpTo(quint32 userId, bool* ok);
        bool updateLastFmScrobbledUpTo(quint32 userId, quint32 newValue);
        bool updateLastFmAuthentication(quint32 userId, QString lastFmUsername,
                                        QString lastFmSessionKey);
        bool updateUserScrobblingSessionKeys(
                                DatabaseRecords::UserScrobblingDataRecord const& record);

        DatabaseRecords::UserDynamicModePreferences getUserDynamicModePreferences(
                                                                           quint32 userId,
                                                                           bool* ok);
        ResultOrError<SuccessType, FailureType> setUserDynamicModePreferences(
                        quint32 userId,
                        DatabaseRecords::UserDynamicModePreferences const& preferences);

        ResultOrError<uint, FailureType> addToHistory(quint32 hashId, quint32 userId,
                                                      QDateTime start, QDateTime end,
                                                      int permillage,
                                                      bool validForScoring);
        ResultOrError<uint, FailureType> getLastHistoryId();
        ResultOrError<quint32, FailureType> getMostRecentRealUserHavingHistory();
        ResultOrError<QVector<DatabaseRecords::BriefHistoryRecord>, FailureType>
            getBriefHistoryFragment(quint32 startId, int limit);
        ResultOrError<QVector<DatabaseRecords::HashHistoryStats>, FailureType>
                                                                      getHashHistoryStats(
                                                                quint32 userId,
                                                                QVector<quint32> hashIds);
        ResultOrError<QVector<DatabaseRecords::HistoryRecord>, FailureType>
                                                              getUserHistoryForScrobbling(
                                                            quint32 userId,
                                                            quint32 startId,
                                                            QDateTime earliestDateTime,
                                                            int limit);
        ResultOrError<QVector<DatabaseRecords::HistoryRecord>, FailureType>
                                                                   getTrackHistoryForUser(
                                                                quint32 userId,
                                                                QVector<quint32> hashIds,
                                                                uint startId,
                                                                int limit);

        ResultOrError<QVector<DatabaseRecords::HashHistoryStats>, FailureType>
                                                            getAllCachedHashStatsForUser(
                                                                        quint32 userId);
        ResultOrError<QVector<DatabaseRecords::HashHistoryStats>, FailureType>
                                                                    getCachedHashStats(
                                                                quint32 userId,
                                                                QVector<quint32> hashIds);
        SuccessOrFailure updateUserHashStatsCacheEntry(quint32 userId,
                                        DatabaseRecords::HashHistoryStats const& stats);
        SuccessOrFailure removeUserHashStatsCacheEntry(quint32 userId, quint32 hashId);

        ResultOrError<QVector<QPair<quint32, quint32>>, FailureType> getEquivalences();
        ResultOrError<SuccessType, FailureType> registerEquivalence(quint32 hashId1,
                                                                    quint32 hashId2,
                                                                    int currentYear);

        static QSharedPointer<Database> getDatabaseForCurrentThread();
        static QUuid getDatabaseUuid();

    private:
        class DatabaseConnection
        {
        public:
            explicit DatabaseConnection(QSqlDatabase database);
            DatabaseConnection(DatabaseConnection const&) = delete;
            DatabaseConnection(DatabaseConnection&&) = default;

            bool isOpen() const;

            bool executeVoid(std::function<void (QSqlQuery&)> preparer);

            bool executeNullableScalar(std::function<void (QSqlQuery&)> preparer,
                                       Nullable<QString>& s);

            bool executeScalar(std::function<void (QSqlQuery&)> preparer,
                               bool& b, bool defaultValue);
            bool executeScalar(std::function<void (QSqlQuery&)> preparer,
                               int& i, int defaultValue);
            bool executeScalar(std::function<void (QSqlQuery&)> preparer,
                               uint& i, uint defaultValue);
            bool executeScalar(std::function<void (QSqlQuery&)> preparer,
                               QDateTime& d);

            template<class T>
            ResultOrError<QVector<T>, FailureType> executeRecords(
                std::function<void (QSqlQuery&)> preparer,
                std::function<T (QSqlQuery&)> extractRecord,
                int recordsToReserveCount = -1);

            bool executeQuery(std::function<void (QSqlQuery&)> preparer,
                              bool processResult,
                              std::function<void (QSqlQuery&)> resultFetcher);

            QSqlDatabase& qSqlDatabase() { return _db; }

        private:
            QSqlDatabase _db;
        };

        class TableEditor
        {
        public:
            TableEditor(Database* database, QString tableName);

            bool addColumnIfNotExists(QString columnName, QString type);

        private:
            Database* _database;
            QString _tableName;
        };

        Database(DatabaseConnection&& databaseConnection);

        DatabaseConnection& connection() { return _dbConnection; }

        static QString buildParamsList(unsigned paramsCount);

        static std::function<void (QSqlQuery&)> prepareSimple(QString sql);

        static bool addColumnIfNotExists(QSqlQuery& q, QString tableName,
                                         QString columnName, QString type);

        static bool getBool(QVariant v, bool nullValue);
        static int getInt(QVariant v, int nullValue);
        static uint getUInt(QVariant v, uint nullValue);
        static QDateTime getUtcDateTime(QVariant v);
        static QString getString(QVariant v, const char* nullValue);

        static QSqlDatabase createQSqlDatabase(QString name, bool setSchema);
        static void printInitializationError(QTextStream& out, QSqlDatabase& db);
        static void printInitializationError(QTextStream& out, Database& database);

        static bool initTables(QTextStream& out, Database& database);
        static bool initMiscTable(Database& database);
        static ResultOrError<QUuid, FailureType> initDatabaseUuid(Database& database);
        static bool initHashesTable(Database& database);
        static bool initFilenameTable(Database& database);
        static bool initFileSizeTable(Database& database);
        static bool initUsersTable(Database& database);
        static bool initHistoryTable(Database& database);
        static bool initEquivalenceTable(Database& database);
        static bool initUserHashStatsCacheTable(Database& database);
        static bool initUserHashStatsCacheBookkeeping(Database& database);

        static QString _hostname;
        static int _port;
        static QString _username;
        static QString _password;
        static QUuid _uuid;
        static bool _initDoneSuccessfully;
        static QThreadStorage<QSharedPointer<Database>> _threadLocalDatabases;
        static QAtomicInt _nextDbNameNumber;

        //QSqlDatabase _db;
        DatabaseConnection _dbConnection;
    };
}
#endif

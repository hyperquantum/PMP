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

        DatabaseRecords::UserDynamicModePreferences getUserDynamicModePreferences(
                                                                           quint32 userId,
                                                                           bool* ok);
        ResultOrError<SuccessType, FailureType> setUserDynamicModePreferences(
                        quint32 userId,
                        DatabaseRecords::UserDynamicModePreferences const& preferences);

        ResultOrError<SuccessType, FailureType> addToHistory(quint32 hashId,
                                                             quint32 userId,
                                                             QDateTime start,
                                                             QDateTime end,
                                                             int permillage,
                                                             bool validForScoring);
        ResultOrError<quint32, FailureType> getMostRecentRealUserHavingHistory();
        ResultOrError<QVector<DatabaseRecords::HashHistoryStats>, FailureType>
                                                                      getHashHistoryStats(
                                                                quint32 userId,
                                                                QVector<quint32> hashIds);

        ResultOrError<QVector<QPair<quint32, quint32>>, FailureType> getEquivalences();
        ResultOrError<SuccessType, FailureType> registerEquivalence(quint32 hashId1,
                                                                    quint32 hashId2,
                                                                    int currentYear);

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

        template<class T>
        ResultOrError<QVector<T>, FailureType> executeRecords(
                                            std::function<void (QSqlQuery&)> preparer,
                                            std::function<T (QSqlQuery&)> extractRecord,
                                            int recordsToReserveCount = -1);

        bool executeVoid(std::function<void (QSqlQuery&)> preparer);

        bool executeQuery(std::function<void (QSqlQuery&)> preparer,
                          bool processResult,
                          std::function<void (QSqlQuery&)> resultFetcher);

        static bool addColumnIfNotExists(QSqlQuery& q, QString tableName,
                                         QString columnName, QString type);

        static bool getBool(QVariant v, bool nullValue);
        static int getInt(QVariant v, int nullValue);
        static uint getUInt(QVariant v, uint nullValue);
        static QDateTime getUtcDateTime(QVariant v);

        static QSqlDatabase createDatabaseConnection(QString name, bool setSchema);
        static void printInitializationError(QTextStream& out, QSqlDatabase& db);

        static bool initMiscTable(QSqlQuery& q);
        static ResultOrError<QUuid, FailureType> initDatabaseUuid(QSqlQuery& q);
        static bool initHashTable(QSqlQuery& q);
        static bool initFilenameTable(QSqlQuery& q);
        static bool initFileSizeTable(QSqlQuery& q);
        static bool initUsersTable(QSqlQuery& q);
        static bool initHistoryTable(QSqlQuery& q);
        static bool initEquivalenceTable(QSqlQuery& q);

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

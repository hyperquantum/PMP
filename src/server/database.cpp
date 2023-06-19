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

#include "database.h"

#include "common/filehash.h"

#include "serversettings.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>
#include <QTextStream>

using namespace PMP::Server::DatabaseRecords;

namespace PMP::Server
{
    QString Database::_hostname;
    int Database::_port;
    QString Database::_username;
    QString Database::_password;
    QUuid Database::_uuid;
    bool Database::_initDoneSuccessfully = false;
    QThreadStorage<QSharedPointer<Database>> Database::_threadLocalDatabases;
    QAtomicInt Database::_nextDbNameNumber;

    bool Database::init(QTextStream& out, const ServerSettings& serverSettings)
    {
        auto connectionSettings = serverSettings.databaseConnectionSettings();

        return init(out, connectionSettings);
    }

    bool Database::init(QTextStream& out,
                        const DatabaseConnectionSettings& connectionSettings)
    {
        out << "initializing database" << Qt::endl;
        _initDoneSuccessfully = false;

        if (!connectionSettings.isComplete())
        {
            out << " incomplete database settings!\n" << Qt::endl;
            return false;
        }

        _hostname = connectionSettings.hostname;
        _port = connectionSettings.port;
        _username = connectionSettings.username;
        _password = connectionSettings.password;

        /* open connection */
        QSqlDatabase db = createDatabaseConnection("PMP_main_dbconn", false);
        if (!db.isOpen())
        {
            out << " ERROR: could not connect to database: " << db.lastError().text()
                << "\n" << Qt::endl;
            return false;
        }

        /* create schema if needed */
        QSqlQuery q(db);
        q.prepare("CREATE DATABASE IF NOT EXISTS pmp; USE pmp;");
        if (!q.exec())
        {
            printInitializationError(out, db);
            return false;
        }

        /* create table 'pmp_misc' if needed */
        if (!initMiscTable(q))
        {
            printInitializationError(out, db);
            return false;
        }

        /* get UUID, or generate one and store it if it does not exist yet */
        auto maybeUuid = initDatabaseUuid(q);
        if (maybeUuid.succeeded())
        {
            _uuid = maybeUuid.result();
            out << " UUID is " << _uuid.toString() << Qt::endl;
        }
        else
        {
            out << " error inserting UUID into database\n" << Qt::endl;
            return false;
        }

        bool tablesInitialized =
            initHashTable(q)             /* pmp_hash */
            && initFilenameTable(q)      /* pmp_filename */
            && initFileSizeTable(q)      /* pmp_filesize */
            && initUsersTable(q)         /* pmp_user */
            && initHistoryTable(q)       /* pmp_history */
            && initEquivalenceTable(q);  /* pmp_equivalence */

        if (!tablesInitialized)
        {
            printInitializationError(out, db);
            return false;
        }

        _initDoneSuccessfully = true;

        out << " database initialization completed successfully\n" << Qt::endl;
        return true;
    }

    bool Database::initMiscTable(QSqlQuery& q)
    {
        q.prepare(
            "CREATE TABLE IF NOT EXISTS pmp_misc("
            " `Key` VARCHAR(63) NOT NULL,"
            " `Value` VARCHAR(255),"
            " PRIMARY KEY(`Key`) "
            ") "
            "ENGINE = InnoDB "
            "DEFAULT CHARACTER SET = utf8 COLLATE = utf8_general_ci"
        );

        return q.exec();
    }

    ResultOrError<QUuid, FailureType> Database::initDatabaseUuid(QSqlQuery& q)
    {
        q.prepare("SELECT `Value` FROM pmp_misc WHERE `Key`=?");
        q.addBindValue("UUID");
        if (!q.exec())
        {
            qCritical() << "could not see if UUID already exists";
            return failure;
        }

        if (q.next())
        {
            auto uuid = QUuid(q.value(0).toString());
            return uuid;
        }

        auto uuid { QUuid::createUuid() };

        q.prepare("INSERT INTO pmp_misc(`Key`, `Value`) VALUES (?,?)");
        q.addBindValue("UUID");
        q.addBindValue(uuid.toString());
        if (!q.exec())
        {
            qCritical() << "error inserting UUID into database";
            return failure;
        }

        qDebug() << "generated new UUID for the database:" << uuid;
        return uuid;
    }

    bool Database::initHashTable(QSqlQuery& q)
    {
        q.prepare(
            "CREATE TABLE IF NOT EXISTS pmp_hash("
            " `HashID` INT UNSIGNED NOT NULL AUTO_INCREMENT,"
            " `InputLength` INT UNSIGNED NOT NULL,"
            " `SHA1` VARCHAR(40) NOT NULL,"
            " `MD5` VARCHAR(32) NOT NULL,"
            " PRIMARY KEY (`HashID`),"
            " UNIQUE INDEX `IDX_pmphash` (`InputLength` ASC, `SHA1` ASC, `MD5` ASC) "
            ") ENGINE = InnoDB"
        );

        return q.exec();
    }

    bool Database::initFilenameTable(QSqlQuery& q)
    {
        q.prepare(
            "CREATE TABLE IF NOT EXISTS pmp_filename("
            " `HashID` INT UNSIGNED NOT NULL,"
            " `FilenameWithoutDir` VARCHAR(255) NOT NULL,"
            " CONSTRAINT `FK_pmpfilenamehashid`"
            "  FOREIGN KEY (`HashID`)"
            "   REFERENCES pmp_hash (`HashID`)"
            "   ON DELETE CASCADE ON UPDATE CASCADE"
            ") "
            "ENGINE = InnoDB "
            "DEFAULT CHARACTER SET = utf8 COLLATE = utf8_general_ci"
        );

        if (!q.exec())
            return false;

        const auto tableName = "pmp_filename";

        if (!addColumnIfNotExists(q, tableName, "YearLastSeen", "INT"))
            return false;

        return true;
    }

    bool Database::initFileSizeTable(QSqlQuery& q)
    {
        q.prepare(
            "CREATE TABLE IF NOT EXISTS pmp_filesize("
            " `HashID` INT UNSIGNED NOT NULL,"
            " `FileSize` BIGINT NOT NULL," /* signed; Qt uses qint64 for file sizes */
            " CONSTRAINT `FK_pmpfilesizehashid`"
            "  FOREIGN KEY (`HashID`)"
            "   REFERENCES pmp_hash (`HashID`)"
            "   ON DELETE CASCADE ON UPDATE CASCADE,"
            " UNIQUE INDEX `IDX_pmpfilesize` (`FileSize` ASC, `HashID` ASC) "
            ") ENGINE = InnoDB"
        );

        if (!q.exec())
            return false;

        const auto tableName = "pmp_filesize";

        if (!addColumnIfNotExists(q, tableName, "YearLastSeen", "INT"))
            return false;

        return true;
    }

    bool Database::initUsersTable(QSqlQuery& q)
    {
        q.prepare(
            "CREATE TABLE IF NOT EXISTS pmp_user("
            " `UserID` INT UNSIGNED NOT NULL AUTO_INCREMENT,"
            " `Login` VARCHAR(63) NOT NULL,"
            " `Salt` VARCHAR(255),"
            " `Password` VARCHAR(255),"
            " PRIMARY KEY (`UserID`),"
            " UNIQUE INDEX `IDX_pmpuser_login` (`Login`)"
            ") "
            "ENGINE = InnoDB "
            "DEFAULT CHARACTER SET = utf8 COLLATE = utf8_general_ci"
        );
        if (!q.exec())
            return false;

        const auto tableName = "pmp_user";

        /* add columns for dynamic mode preferences */
        if (!addColumnIfNotExists(q, tableName, "DynamicModeEnabled", "BIT")
            || !addColumnIfNotExists(q, tableName,
                                     "TrackRepetitionAvoidanceIntervalSeconds", "INT"))
        {
            return false;
        }

        /* extra columns for Last.Fm scrobbling */
        if (!addColumnIfNotExists(q, tableName, "EnableLastFmScrobbling", "BIT")
            || !addColumnIfNotExists(q, tableName, "LastFmUser", "VARCHAR(255)")
            || !addColumnIfNotExists(q, tableName, "LastFmSessionKey", "VARCHAR(255)")
            || !addColumnIfNotExists(q, tableName, "LastFmScrobbledUpTo", "INT UNSIGNED"))
        {
            return false;
        }

        return true;
    }

    bool Database::initHistoryTable(QSqlQuery& q)
    {
        q.prepare(
            "CREATE TABLE IF NOT EXISTS pmp_history ("
            " `HistoryID` INT UNSIGNED NOT NULL AUTO_INCREMENT,"
            " `HashID` INT UNSIGNED NOT NULL,"
            " `UserID` INT UNSIGNED,"
            " `Start` DATETIME NOT NULL,"
            " `End` DATETIME,"
            " `Permillage` INT NOT NULL,"
            " `ValidForScoring` BIT NOT NULL,"
            " PRIMARY KEY (`HistoryID`),"
            " INDEX `IDX_history_hash` (`HashID` ASC),"
            " INDEX `IDX_history_user_hash` (`UserID` ASC, `HashID` ASC),"
            " CONSTRAINT `FK_history_hash`"
            "  FOREIGN KEY (`HashID`)"
            "  REFERENCES `pmp_hash` (`HashID`)"
            "   ON DELETE RESTRICT ON UPDATE CASCADE,"
            " CONSTRAINT `FK_history_user`"
            "  FOREIGN KEY (`UserID`)"
            "  REFERENCES `pmp_user` (`UserID`)"
            "   ON DELETE RESTRICT ON UPDATE CASCADE"
            ") "
            "ENGINE = InnoDB "
            "DEFAULT CHARACTER SET = utf8 COLLATE = utf8_general_ci"
        );

        return q.exec();
    }

    bool Database::initEquivalenceTable(QSqlQuery& q)
    {
        q.prepare(
            "CREATE TABLE IF NOT EXISTS pmp_equivalence("
            " `Hash1` INT UNSIGNED NOT NULL,"
            " `Hash2` INT UNSIGNED NOT NULL,"
            " `YearAdded` INT NOT NULL,"
            " CONSTRAINT `FK_pmpequivalencehash1`"
            "  FOREIGN KEY (`Hash1`)"
            "   REFERENCES pmp_hash (`HashID`)"
            "   ON DELETE CASCADE ON UPDATE CASCADE,"
            " CONSTRAINT `FK_pmpequivalencehash2`"
            "  FOREIGN KEY (`Hash2`)"
            "   REFERENCES pmp_hash (`HashID`)"
            "   ON DELETE CASCADE ON UPDATE CASCADE,"
            " UNIQUE INDEX `IDX_pmpequivalence` (`Hash1` ASC, `Hash2` ASC) "
            ") ENGINE = InnoDB"
        );

        return q.exec();
    }

    Database::Database(QSqlDatabase db)
     : _db(db)
    {
        //
    }

    QSqlDatabase Database::createDatabaseConnection(QString name, bool setSchema)
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", name);
        db.setHostName(_hostname);
        if (_port > 0) { db.setPort(_port); }

        db.setUserName(_username);
        db.setPassword(_password);

        if (setSchema) { db.setDatabaseName("pmp"); }

        if (!db.open())
        {
            qDebug() << "Database::createDatabaseConnection FAIL:"
                     << "could not open connection; name:" << name;
        }

        return db;
    }

    void Database::printInitializationError(QTextStream& out, QSqlDatabase& db)
    {
        out << " database initialization problem: " << db.lastError().text() << Qt::endl;
        out << Qt::endl;
    }

    QSharedPointer<Database> Database::getDatabaseForCurrentThread()
    {
        if (_threadLocalDatabases.hasLocalData())
        {
            return _threadLocalDatabases.localData();
        }

        if (!_initDoneSuccessfully) return QSharedPointer<Database>(nullptr);

        int num = _nextDbNameNumber.fetchAndAddAcquire(1);
        QString dbName = QString::number(num) + "_threaddb";

        qDebug() << "Database: creating connection for thread; num=" << num;

        QSqlDatabase qsqlDb = createDatabaseConnection(dbName, true);
        QSharedPointer<Database> dbPtr(new Database(qsqlDb));

        if (!dbPtr->isConnectionOpen()) return QSharedPointer<Database>(nullptr);

        _threadLocalDatabases.setLocalData(dbPtr);

        return dbPtr;
    }

    QUuid Database::getDatabaseUuid()
    {
        return _uuid;
    }

    bool Database::isConnectionOpen() const
    {
        return _db.isOpen();
    }

    ResultOrError<SuccessType, FailureType> Database::registerHash(const FileHash& hash)
    {
        QString sha1 = hash.SHA1().toHex();
        QString md5 = hash.MD5().toHex();

        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "INSERT INTO pmp_hash(InputLength, `SHA1`, `MD5`) "
                    "VALUES(?,?,?) "
                    "ON DUPLICATE KEY UPDATE InputLength=InputLength"
                );
                q.addBindValue(hash.length());
                q.addBindValue(sha1);
                q.addBindValue(md5);
            };

        if (!executeVoid(preparer))
        {
            qWarning() << "Database::registerHash : insert failed!" << Qt::endl;
            return failure;
        }

        return success;
    }

    ResultOrError<uint, FailureType> Database::getHashId(const FileHash& hash)
    {
        QString sha1 = hash.SHA1().toHex();
        QString md5 = hash.MD5().toHex();

        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "SELECT HashID FROM pmp_hash"
                    " WHERE InputLength=? AND `SHA1`=? AND `MD5`=?"
                );
                q.addBindValue(hash.length());
                q.addBindValue(sha1);
                q.addBindValue(md5);
            };

        uint id;
        if (!executeScalar(preparer, id, 0))
        {
            qDebug() << "Database::getHashID : query failed!" << Qt::endl;
            return failure;
        }

        return id;
    }

    ResultOrError<QVector<QPair<uint, FileHash> >, FailureType> Database::getHashes(
                                                                        uint largerThanID)
    {
        auto preparer =
            [=](QSqlQuery& q)
            {
                q.prepare(
                    "SELECT HashID,InputLength,`SHA1`,`MD5` FROM pmp_hash"
                    " WHERE HashID > ? "
                    "ORDER BY HashID"
                );
                q.addBindValue(largerThanID);
            };

        auto extractRecord =
            [](QSqlQuery& q) -> QPair<uint,FileHash>
            {
                uint hashID = q.value(0).toUInt();
                uint length = q.value(1).toUInt();
                QByteArray sha1 = QByteArray::fromHex(q.value(2).toByteArray());
                QByteArray md5 = QByteArray::fromHex(q.value(3).toByteArray());

                return { hashID, FileHash(length, sha1, md5) };
            };

        return executeRecords<QPair<uint, FileHash>>(preparer, extractRecord);
    }

    ResultOrError<SuccessType, FailureType> Database::registerFilenameSeen(uint hashId,
                                                    const QString& filenameWithoutPath,
                                                    int currentYear)
    {
        /* We do not support extremely long file names.  Lookup for those files should be
           done by other means. */
        if (filenameWithoutPath.length() > 255)
            return failure;

        /* A race condition could cause duplicate records to be registered; that is
           tolerable however. */

        /* step 1 - find the oldest year that has been registered (or 0 for NULL) */

        auto preparer1 =
            [=](QSqlQuery& query)
            {
                query.prepare(
                    "SELECT `YearLastSeen` FROM pmp_filename "
                    "WHERE `HashID`=? AND `FilenameWithoutDir`=? "
                    "ORDER BY COALESCE(`YearLastSeen`, 0) "
                    "LIMIT 1"
                );
                query.addBindValue(hashId);
                query.addBindValue(filenameWithoutPath);
            };

        int oldYear;
        if (!executeScalar(preparer1, oldYear, -1))
            return failure;

        if (oldYear == currentYear)
            return success; /* nothing to update */

        /* step 2 - delete existing entries with an older year or without a year. This
                    also causes duplicate entries to be cleaned up eventually. */

        auto preparer2 =
            [=](QSqlQuery& query)
            {
                query.prepare(
                    "DELETE FROM pmp_filename "
                    "WHERE `HashID`=? AND `FilenameWithoutDir`=?"
                );
                query.addBindValue(hashId);
                query.addBindValue(filenameWithoutPath);
            };

        if (!executeVoid(preparer2))
            return failure;

        /* step 3 - insert the filename with the current year */

        auto preparer3 =
            [=](QSqlQuery& query)
            {
                query.prepare(
                    "INSERT INTO pmp_filename(`HashID`,`FilenameWithoutDir`,"
                    "                         `YearLastSeen`) "
                    "VALUES(?,?,?)"
                );
                query.addBindValue(hashId);
                query.addBindValue(filenameWithoutPath);
                query.addBindValue(currentYear);
            };

        if (!executeVoid(preparer3))
            return failure;

        return success;
    }

    ResultOrError<QVector<QString>, FailureType> Database::getFilenames(uint hashID)
    {
        auto preparer =
            [=](QSqlQuery& q)
            {
                q.prepare( // we use DISTINCT because there's no unique index
                    "SELECT DISTINCT `FilenameWithoutDir` FROM pmp_filename"
                    " WHERE HashID=?"
                );
                q.addBindValue(hashID);
            };

        auto extractRecord =
            [](QSqlQuery& q)
            {
                return q.value(0).toString();
            };

        return executeRecords<QString>(preparer, extractRecord);
    }

    ResultOrError<SuccessType, FailureType> Database::registerFileSizeSeen(uint hashId,
                                                                        qint64 size,
                                                                        int currentYear)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "INSERT INTO pmp_filesize(`HashID`,`FileSize`,`YearLastSeen`)"
                    " VALUES(?,?,?)"
                    " ON DUPLICATE KEY UPDATE `YearLastSeen`=?"
                );
                q.addBindValue(hashId);
                q.addBindValue(size);
                q.addBindValue(currentYear);
                q.addBindValue(currentYear);
            };

        if (!executeVoid(preparer))
        {
            qDebug() << "Database::registerFileSize : insert/update failed!" << Qt::endl;
            return failure;
        }

        return success;
    }

    ResultOrError<QVector<qint64>, FailureType> Database::getFileSizes(uint hashID)
    {
        auto preparer =
            [=](QSqlQuery& q)
            {
                q.prepare(
                    "SELECT `FileSize` FROM pmp_filesize"
                    " WHERE HashID=?"
                );
                q.addBindValue(hashID);
            };

        auto extractRecord =
            [](QSqlQuery& q) -> qint64
            {
                return q.value(0).toLongLong();
            };

        return executeRecords<qint64>(preparer, extractRecord);
    }

    ResultOrError<QVector<DatabaseRecords::User>, FailureType> Database::getUsers()
    {
        auto preparer =
                prepareSimple("SELECT `UserID`,`Login`,`Salt`,`Password` FROM pmp_user");

        auto extractRecord =
            [](QSqlQuery& q)
            {
                quint32 userID = q.value(0).toUInt();
                QString login = q.value(1).toString();
                QString salt = q.value(2).toString();
                QString password = q.value(3).toString();

                return User::fromDb(userID, login, salt, password);
            };

        return executeRecords<User>(preparer, extractRecord);
    }

    ResultOrError<bool, FailureType> Database::checkUserExists(QString userName)
    {
        auto preparer =
            [=](QSqlQuery& q)
            {
                q.prepare(
                    "SELECT EXISTS(SELECT * FROM pmp_user WHERE LOWER(`Login`)=LOWER(?))"
                );
                q.addBindValue(userName);
            };

        bool exists;
        if (!executeScalar(preparer, exists, false)) /* error */
        {
            qDebug() << "Database::checkUserExists : query failed!" << Qt::endl;
            return failure;
        }

        return exists;
    }

    ResultOrError<quint32, FailureType> Database::registerNewUser(User& user)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "INSERT INTO pmp_user(`Login`,`Salt`,`Password`) VALUES(?,?,?)"
                );
                q.addBindValue(user.login);
                q.addBindValue(QString::fromLatin1(user.salt.toBase64()));
                q.addBindValue(QString::fromLatin1(user.password.toBase64()));
            };

        if (!executeVoid(preparer)) /* error */
        {
            qDebug() << "Database::registerNewUser : insert failed!" << Qt::endl;
            return failure;
        }

        auto preparer2 = prepareSimple("SELECT LAST_INSERT_ID()");

        uint userId = 0;
        if (!executeScalar(preparer2, userId, 0))
        {
            qDebug() << "Database::registerNewUser : select failed!" << Qt::endl;
            return failure;
        }

        return userId;
    }

    QVector<UserScrobblingDataRecord> Database::getUsersScrobblingData()
    {
        auto preparer =
            prepareSimple(
                "SELECT `UserID`,`EnableLastFmScrobbling`,`LastFmUser`,"
                "  `LastFmSessionKey`,`LastFmScrobbledUpTo` "
                "FROM pmp_user"
            );

        QVector<UserScrobblingDataRecord> result;

        auto resultGetter =
            [&result] (QSqlQuery& q)
            {
                while (q.next())
                {
                    UserScrobblingDataRecord record;
                    record.userId = q.value(0).toUInt();
                    record.enableLastFmScrobbling = getBool(q.value(1), false);
                    record.lastFmUser = getString(q.value(2), "");
                    record.lastFmSessionKey = getString(q.value(3), "");
                    record.lastFmScrobbledUpTo = getUInt(q.value(4), 0);

                    result << record;
                }
            };

        if (!executeQuery(preparer, true, resultGetter)) /* error */
        {
            qWarning() << "Database::getUsersScrobblingData : could not execute";
            return {};
        }

        return result;
    }

    LastFmScrobblingDataRecord Database::getUserLastFmScrobblingData(quint32 userId)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "SELECT `EnableLastFmScrobbling`,`LastFmUser`,"
                    "  `LastFmSessionKey`,`LastFmScrobbledUpTo` "
                    "FROM pmp_user "
                    "WHERE UserId=?"
                );
                q.addBindValue(userId);
            };

        LastFmScrobblingDataRecord result;

        auto resultGetter =
            [&result] (QSqlQuery& q)
            {
                if (q.next())
                {
                    result.enableLastFmScrobbling = getBool(q.value(0), false);
                    result.lastFmUser = getString(q.value(1), "");
                    result.lastFmSessionKey = getString(q.value(2), "");
                    result.lastFmScrobbledUpTo = getUInt(q.value(3), 0);
                }
            };

        if (!executeQuery(preparer, true, resultGetter)) /* error */
        {
            qWarning() << "Database::getUserLastFmScrobblingData : could not execute";
            return {};
        }

        return result;
    }

    UserDynamicModePreferences Database::getUserDynamicModePreferences(quint32 userId,
                                                                       bool* ok)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "SELECT DynamicModeEnabled, TrackRepetitionAvoidanceIntervalSeconds "
                    "FROM pmp_user "
                    "WHERE UserId=?"
                );
                q.addBindValue(userId);
            };

        UserDynamicModePreferences result;

        auto resultGetter =
            [&result] (QSqlQuery& q)
            {
                if (q.next())
                {
                    result.dynamicModeEnabled = getBool(q.value(0), true);
                    result.trackRepetitionAvoidanceIntervalSeconds =
                                                                 getInt(q.value(1), 3600);
                }
            };

        if (!executeQuery(preparer, true, resultGetter))
        {
            qWarning() << "Database::getUserDynamicModePreferences : could not execute";

            if (ok)
                *ok = false;

            return {};
        }

        if (ok)
            *ok = true;

        return result;
    }

    ResultOrError<SuccessType, FailureType> Database::setUserDynamicModePreferences(
                                            quint32 userId,
                                            UserDynamicModePreferences const& preferences)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "UPDATE pmp_user "
                    "SET DynamicModeEnabled=?, TrackRepetitionAvoidanceIntervalSeconds=? "
                    "WHERE UserId=?"
                );
                q.addBindValue(preferences.dynamicModeEnabled);
                q.addBindValue(preferences.trackRepetitionAvoidanceIntervalSeconds);
                q.addBindValue(userId);
            };

        if (!executeVoid(preparer))
        {
            qWarning() << "Database::setUserDynamicModePreferences : could not execute";
            return failure;
        }

        return success;
    }

    ResultOrError<SuccessType, FailureType> Database::addToHistory(quint32 hashId,
                                                                   quint32 userId,
                                                                   QDateTime start,
                                                                   QDateTime end,
                                                                   int permillage,
                                                                   bool validForScoring)
    {
//        qDebug() << "  Database::addToHistory called\n"
//                 << "   hash" << hashId << " user" << userId << " perm" << permillage
//                 << " forScoring" << validForScoring << "\n"
//                 << "   start" << start << "\n"
//                 << "   end" << end;

        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "INSERT INTO pmp_history"
                    " (`HashID`,`UserID`,`Start`,`End`,`Permillage`,`ValidForScoring`) "
                    "VALUES(?,?,?,?,?,?)"
                );
                q.addBindValue(hashId);
                q.addBindValue(userId == 0 ? /*NULL*/QVariant(QVariant::UInt) : userId);
                q.addBindValue(start.toUTC());
                q.addBindValue(end.toUTC());
                q.addBindValue(permillage);
                q.addBindValue(validForScoring);
            };

        if (!executeVoid(preparer)) /* error */
        {
            qWarning() << "Database::addToHistory : insert failed!" << Qt::endl;
            return failure;
        }

        return success;
    }

    ResultOrError<quint32, FailureType> Database::getMostRecentRealUserHavingHistory()
    {
        auto preparer =
            prepareSimple(
                "SELECT UserID FROM pmp_history "
                "WHERE UserID > 0 "
                "ORDER BY HistoryID DESC "
                "LIMIT 1"
            );

        uint userId = 0;
        if (!executeScalar(preparer, userId, 0))
        {
            qDebug() << "Database::getMostRecentRealUserHavingHistory : select failed!"
                     << Qt::endl;
            return failure;
        }

        return userId;
    }

    ResultOrError<QVector<HashHistoryStats>, FailureType> Database::getHashHistoryStats(
                                                                 quint32 userId,
                                                                 QVector<quint32> hashIds)
    {
        if (hashIds.isEmpty())
            return QVector<HashHistoryStats> {};

        auto preparer =
            [=](QSqlQuery& q)
            {
                q.prepare(
                    "SELECT ha.HashID, hi.LastHistoryId, hi.PrevHeard,"
                    "       hi2.ScoreHeardCount, hi2.ScorePermillage "
                    "FROM pmp_hash AS ha"
                    " LEFT JOIN"
                    "  (SELECT HashID, MAX(HistoryID) AS LastHistoryId,"
                    "          MAX(End) AS PrevHeard"
                    "   FROM pmp_history"
                    "   WHERE COALESCE(UserID, 0)=? GROUP BY HashID) AS hi"
                    "  ON ha.HashID=hi.HashID"
                    " LEFT JOIN"
                    "  (SELECT HashID, COUNT(*) AS ScoreHeardCount,"
                    "   AVG(Permillage) AS ScorePermillage FROM pmp_history"
                    "   WHERE COALESCE(UserID, 0)=? AND ValidForScoring != 0"
                    "   GROUP BY HashID) AS hi2"
                    "  ON ha.HashID=hi2.HashID "
                    "WHERE ha.HashID IN " + buildParamsList(hashIds.size())
                );

                q.addBindValue(userId);
                q.addBindValue(userId); /* twice */
                for (auto hashId : qAsConst(hashIds))
                {
                    q.addBindValue(hashId);
                }
            };

        auto extractRecord =
            [](QSqlQuery& q)
            {
                quint32 hashID = q.value(0).toUInt();
                uint lastHistoryId = getUInt(q.value(1), 0);
                QDateTime prevHeard = getUtcDateTime(q.value(2));
                quint32 scoreHeardCount = (quint32)getUInt(q.value(3), 0);
                qint32 scorePermillage = (qint32)getInt(q.value(4), -1);

                HashHistoryStats stats;
                stats.lastHistoryId = lastHistoryId;
                stats.hashId = hashID;
                stats.lastHeard = prevHeard;
                stats.scoreHeardCount = scoreHeardCount;
                stats.averagePermillage = scorePermillage;

                return stats;
            };

        return executeRecords<HashHistoryStats>(preparer, extractRecord, hashIds.size());
    }

    ResultOrError<QVector<QPair<quint32, quint32>>, FailureType>
        Database::getEquivalences()
    {
        auto preparer = prepareSimple("SELECT Hash1, Hash2 FROM pmp_equivalence");

        auto extractRecord =
            [](QSqlQuery& q) -> QPair<quint32, quint32>
            {
                quint32 hashId1 = q.value(0).toUInt();
                quint32 hashId2 = q.value(1).toUInt();

                return { hashId1, hashId2 };
            };

        return executeRecords<QPair<quint32, quint32>>(preparer, extractRecord);
    }

    ResultOrError<SuccessType, FailureType> Database::registerEquivalence(quint32 hashId1,
                                                                          quint32 hashId2,
                                                                          int currentYear)
    {
        Q_ASSERT_X(hashId1 != hashId2,
                   "Database::registerEquivalence",
                   "hashes must be different");

        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "INSERT INTO pmp_equivalence(`Hash1`,`Hash2`,`YearAdded`)"
                    " VALUES(?,?,?)"
                    " ON DUPLICATE KEY UPDATE `YearAdded`=`YearAdded`"
                );
                q.addBindValue(hashId1);
                q.addBindValue(hashId2);
                q.addBindValue(currentYear);
            };

        if (!executeVoid(preparer))
        {
            qDebug() << "Database::registerEquivalence : insert/update failed!"
                     << Qt::endl;
            return failure;
        }

        return success;
    }

    QVector<HistoryRecord> Database::getUserHistoryForScrobbling(quint32 userId,
                                                                 quint32 startId,
                                                               QDateTime earliestDateTime,
                                                                 int limit)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "SELECT"
                    " HistoryID, HashID, UserID, `Start`, `End`,"
                    " Permillage, ValidForScoring "
                    "FROM pmp_history "
                    "WHERE UserId=? AND HistoryID >= ? AND `Start` >= ?"
                    " AND Permillage >= 500 AND ValidForScoring != 0 "
                    "ORDER BY HistoryID "
                    "LIMIT ?"
                );
                q.addBindValue(userId);
                q.addBindValue(startId);
                q.addBindValue(earliestDateTime.toUTC());
                q.addBindValue(limit);
            };

        QVector<HistoryRecord> result;
        result.reserve(limit);

        auto resultGetter =
            [&result] (QSqlQuery& q)
            {
                while (q.next())
                {
                    HistoryRecord record;
                    record.id = q.value(0).toUInt();
                    record.hashId = q.value(1).toUInt();
                    record.userId = q.value(2).toUInt();
                    record.start = getUtcDateTime(q.value(3));
                    record.end = getUtcDateTime(q.value(4));
                    record.permillage = qint16(q.value(5).toInt());
                    record.validForScoring = q.value(6).toBool();

                    result << record;
                }
            };

        if (!executeQuery(preparer, true, resultGetter)) /* error */
        {
            qWarning() << "Database::getUserHistoryForScrobbling : could not execute";
            return {};
        }

        return result;
    }

    bool Database::setLastFmScrobblingEnabled(quint32 userId, bool enabled)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "UPDATE pmp_user SET EnableLastFmScrobbling=? "
                    "WHERE UserId=?"
                );
                q.addBindValue(enabled);
                q.addBindValue(userId);
            };

        if (!executeVoid(preparer))
        {
            qWarning() << "Database::setLastFmScrobblingEnabled : could not execute";
            return false;
        }

        return true;
    }

    quint32 Database::getLastFmScrobbledUpTo(quint32 userId, bool* ok)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "SELECT LastFmScrobbledUpTo FROM pmp_user "
                    "WHERE UserId=?"
                );
                q.addBindValue(userId);
            };

        uint result;
        bool success = executeScalar(preparer, result, 0);

        if (ok)
            *ok = success;

        if (success)
            return quint32(result);

        return 0;
    }

    bool Database::updateLastFmScrobbledUpTo(quint32 userId, quint32 newValue)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "UPDATE pmp_user "
                    "SET LastFmScrobbledUpTo=? "
                    "WHERE UserId=?"
                );
                q.addBindValue(newValue);
                q.addBindValue(userId);
            };

        return executeVoid(preparer);
    }

    bool Database::updateLastFmAuthentication(quint32 userId, QString lastFmUsername,
                                              QString lastFmSessionKey)
    {
        auto preparer =
            [=] (QSqlQuery& q)
        {
            q.prepare(
                "UPDATE pmp_user "
                "SET LastFmUser=?, LastFmSessionKey=? "
                "WHERE UserId=?"
                );
            q.addBindValue(lastFmUsername);
            q.addBindValue(lastFmSessionKey);
            q.addBindValue(userId);
        };

        return executeVoid(preparer);
    }

    bool Database::updateUserScrobblingSessionKeys(const UserScrobblingDataRecord& record)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "UPDATE pmp_user "
                    "SET LastFmSessionKey=? "
                    "WHERE UserId=?"
                );
                q.addBindValue(record.lastFmSessionKey);
                q.addBindValue(record.userId);
            };

        return executeVoid(preparer);
    }

    QString Database::buildParamsList(unsigned paramsCount)
    {
        QString s;
        s.reserve(2 + paramsCount * 2);
        s += "(";

        for (unsigned i = 0; i < paramsCount; ++i)
        {
            if (i == 0)
                s += "?";
            else
                s += ",?";
        }

        s += ")";
        return s;
    }

    bool Database::executeScalar(std::function<void (QSqlQuery&)> preparer, bool& b,
                                 bool defaultValue)
    {
        auto resultGetter =
            [&b, defaultValue] (QSqlQuery& q)
            {
                if (q.next())
                {
                    QVariant v = q.value(0);
                    b = (v.isNull()) ? defaultValue : v.toBool();
                }
                else
                {
                    b = defaultValue;
                }
            };

        if (executeQuery(preparer, true, resultGetter))
            return true;

        b = defaultValue;
        return false;
    }

    bool Database::executeScalar(std::function<void (QSqlQuery&)> preparer, int& i,
                                 int defaultValue)
    {
        auto resultGetter =
            [&i, defaultValue] (QSqlQuery& q)
            {
                if (q.next())
                {
                    QVariant v = q.value(0);
                    i = (v.isNull()) ? defaultValue : v.toInt();
                }
                else
                {
                    i = defaultValue;
                }
            };

        if (executeQuery(preparer, true, resultGetter))
            return true;

        i = defaultValue;
        return false;
    }

    bool Database::executeScalar(std::function<void (QSqlQuery&)> preparer, uint& i,
                                 uint defaultValue)
    {
        auto resultGetter =
            [&i, defaultValue] (QSqlQuery& q)
            {
                if (q.next())
                {
                    QVariant v = q.value(0);
                    i = (v.isNull()) ? defaultValue : v.toUInt();
                }
                else
                {
                    i = defaultValue;
                }
            };

        if (executeQuery(preparer, true, resultGetter))
            return true;

        i = defaultValue;
        return false;
    }

    bool Database::executeScalar(std::function<void (QSqlQuery&)> preparer, QDateTime& d)
    {
        auto resultGetter =
            [&d] (QSqlQuery& q)
            {
                if (q.next())
                {
                    QVariant v = q.value(0);
                    d = (v.isNull()) ? QDateTime() : v.toDateTime();
                }
                else
                {
                    d = QDateTime();
                }
            };

        if (executeQuery(preparer, true, resultGetter))
            return true;

        d = QDateTime();
        return false;
    }

    template<class T>
    ResultOrError<QVector<T>, FailureType> Database::executeRecords(
                                            std::function<void (QSqlQuery&)> preparer,
                                            std::function<T (QSqlQuery&)> extractRecord,
                                            int recordsToReserveCount)
    {
        QVector<T> vector;

        auto resultGetter =
            [&vector, extractRecord, recordsToReserveCount] (QSqlQuery& q)
            {
                if (recordsToReserveCount >= 0)
                    vector.reserve(recordsToReserveCount);

                while (q.next())
                {
                    vector.append(extractRecord(q));
                }
            };

        if (executeQuery(preparer, true, resultGetter))
            return vector;

        return failure;
    }

    std::function<void (QSqlQuery&)> Database::prepareSimple(QString sql)
    {
        return [=] (QSqlQuery& q) { q.prepare(sql); };
    }

    bool Database::executeVoid(std::function<void (QSqlQuery&)> preparer)
    {
        return executeQuery(preparer, false, std::function<void (QSqlQuery&)>());
    }

    bool Database::executeQuery(std::function<void (QSqlQuery&)> preparer,
                                bool processResult,
                                std::function<void (QSqlQuery&)> resultFetcher)
    {
        QSqlQuery q(_db);
        preparer(q);
        if (q.exec())
        {
            if (processResult) resultFetcher(q);
            return true;
        }

        /* something went wrong */

        QSqlError error = q.lastError();
        qDebug() << "Database: query failed:" << error.text();
        qDebug() << " error type:" << error.type();
        qDebug() << " native error code:" << error.nativeErrorCode();
        qDebug() << " db error:" << error.databaseText();
        qDebug() << " driver error:" << error.driverText();
        qDebug() << " sql:" << q.lastQuery();

        if (!_db.isOpen())
        {
            qDebug() << " connection not open!";
        }

        if (!_db.isValid())
        {
            qDebug() << " connection not valid!";
        }

        if (error.nativeErrorCode() != "2006") return false;

        /* An extended sleep period followed by a resume will result in the error "MySQL
           server has gone away", error code 2006. In that case we try to reopen the
           connection and re-execute the query. */

        qDebug() << " will try to re-establish a connection and re-execute the query";
        _db.close();
        _db.setDatabaseName("pmp");
        if (!_db.open())
        {
            qDebug() << "  FAILED.  Connection not reopened.";
            return false;
        }

        preparer(q);

        if (!q.exec())
        {
            QSqlError error2 = q.lastError();
            qDebug() << "  FAILED to re-execute query:" << error2.text();
            qDebug() << "  error type:" << error2.type();
            qDebug() << "  native error code:" << error2.nativeErrorCode();
            qDebug() << "  db error:" << error2.databaseText();
            qDebug() << "  driver error:" << error2.driverText();
            qDebug() << "  sql:" << q.lastQuery();
            qDebug() << "  BAILING OUT.";
            return false;
        }

        qDebug() << "  SUCCESS!";

        if (processResult) resultFetcher(q);
        return true;
    }

    bool Database::addColumnIfNotExists(QSqlQuery& q, QString tableName,
                                        QString columnName, QString type)
    {
        QString sql =
            QString(
                "SELECT EXISTS("
                " SELECT * FROM information_schema.COLUMNS"
                " WHERE TABLE_SCHEMA = 'pmp' AND TABLE_NAME = '%1' AND COLUMN_NAME = '%2'"
                ") AS column_exists"
            ).arg(tableName, columnName);

        q.prepare(sql);
        if (!q.exec()) return false;
        bool exists = false;
        if (q.next())
        {
            exists = q.value(0).toBool();
        }
        if (exists) return true;

        sql = QString("ALTER TABLE %1 ADD `%2` %3 NULL").arg(tableName, columnName, type);
        q.prepare(sql);
        return q.exec();
    }

    bool Database::getBool(QVariant v, bool nullValue)
    {
        if (v.isNull())
            return nullValue;

        /* workaround for zero BIT value being loaded as "\0" instead of "0" */
        if (v == QVariant(QChar(0)))
            return false;

        return v.toBool();
    }

    int Database::getInt(QVariant v, int nullValue)
    {
        if (v.isNull()) return nullValue;
        return v.toInt();
    }

    uint Database::getUInt(QVariant v, uint nullValue)
    {
        if (v.isNull()) return nullValue;
        return v.toUInt();
    }

    QDateTime Database::getUtcDateTime(QVariant v)
    {
        /* assuming that it is not null */
        auto dateTime = v.toDateTime();
        dateTime.setTimeSpec(Qt::UTC);
        return dateTime;
    }

    QString Database::getString(QVariant v, const char* nullValue)
    {
        if (v.isNull()) return nullValue;
        return v.toString();
    }
}

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

#include "database.h"

#include "common/filehash.h"

#include "serversettings.h"

#include <QElapsedTimer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>
#include <QTextStream>

using namespace PMP::Server::DatabaseRecords;

namespace PMP::Server
{
    /* ===== DatabaseConnection ===== */

    Database::DatabaseConnection::DatabaseConnection(QSqlDatabase database)
        : _db(database)
    {
        //
    }

    bool Database::DatabaseConnection::isOpen() const
    {
        return _db.isOpen();
    }

    bool Database::DatabaseConnection::executeVoid(
                                                std::function<void (QSqlQuery&)> preparer)
    {
        return executeQuery(preparer, false, std::function<void (QSqlQuery&)>());
    }

    bool Database::DatabaseConnection::executeNullableScalar(
                                                std::function<void (QSqlQuery&)> preparer,
                                                Nullable<QString>& s)
    {
        auto resultGetter =
            [&s] (QSqlQuery& q)
            {
                if (q.next())
                {
                    QVariant v = q.value(0);
                    if (v.isNull())
                        s = null;
                    else
                        s = v.toString();
                }
                else
                {
                    s = null;
                }
            };

        if (executeQuery(preparer, true, resultGetter))
            return true;

        s = null;
        return false;
    }

    bool Database::DatabaseConnection::executeScalar(
        std::function<void (QSqlQuery&)> preparer, bool& b, bool defaultValue)
    {
        auto resultGetter =
            [&b, defaultValue] (QSqlQuery& q)
            {
                if (q.next())
                {
                    QVariant v = q.value(0);
                    b = getBool(v, defaultValue);
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

    bool Database::DatabaseConnection::executeScalar(
        std::function<void (QSqlQuery&)> preparer, int& i, int defaultValue)
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

    bool Database::DatabaseConnection::executeScalar(
        std::function<void (QSqlQuery&)> preparer, uint& i, uint defaultValue)
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

    bool Database::DatabaseConnection::executeScalar(
        std::function<void (QSqlQuery&)> preparer, QDateTime& d)
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
    ResultOrError<QVector<T>, FailureType> Database::DatabaseConnection::executeRecords(
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

    bool Database::DatabaseConnection::executeQuery(
                                        std::function<void (QSqlQuery&)> preparer,
                                        bool processResult,
                                        std::function<void (QSqlQuery&)> resultFetcher)
    {
        if (!_db.isOpen())
        {
            qWarning() << "DatabaseConnection: connection not open!";
            return false;
        }

        if (!_db.isValid())
        {
            qWarning() << "DatabaseConnection: connection not valid!";
            return false;
        }

        QSqlQuery q(_db);

        QElapsedTimer timer;
        timer.start();

        if (executeQueryInternal(q, preparer, processResult, resultFetcher))
            return true;

        /* something went wrong */

        auto elapsedTimeMs = timer.elapsed();
        logLastSqlError(q);
        auto error = q.lastError();

        if (!shouldReconnectAndRetryQueryAfter(error, elapsedTimeMs))
        {
            qWarning() << "SQL query failed:" << error.text();
            return false;
        }

        qDebug() << "DatabaseConnection: will try to reconnect and re-execute the query";

        if (!closeAndReopenConnection())
        {
            qWarning() << "Could not retry failed SQL query with error:" << error.text();
            return false;
        }

        if (executeQueryInternal(q, preparer, processResult, resultFetcher))
        {
            qDebug() << "DatabaseConnection: reconnect and re-execute succeeded!";
            return true;
        }

        logLastSqlError(q);
        qWarning() << "SQL query failed a second time:" << q.lastError().text();
        return false;
    }

    bool Database::DatabaseConnection::executeQueryInternal(
        QSqlQuery& query,
        std::function<void (QSqlQuery&)> preparer,
        bool processResult,
        std::function<void (QSqlQuery&)> resultFetcher)
    {
        preparer(query);

        if (!query.exec())
            return false;

        if (processResult)
            resultFetcher(query);

        return true;
    }

    void Database::DatabaseConnection::logLastSqlError(QSqlQuery const& query)
    {
        auto sql = query.lastQuery();
        logSqlError(query.lastError(), sql);
    }

    void Database::DatabaseConnection::logSqlError(QSqlError const& error,
                                                   QString const& sql)
    {
        qDebug() << "SQL ERROR: error type:" << error.type();
        qDebug() << "SQL ERROR: native error code:" << error.nativeErrorCode();
        qDebug() << "SQL ERROR: db error text:" << error.databaseText();
        qDebug() << "SQL ERROR: driver error text:" << error.driverText();
        qDebug() << "SQL ERROR: query:" << sql;
    }

    bool Database::DatabaseConnection::shouldReconnectAndRetryQueryAfter(
                                                                const QSqlError& error,
                                                                qint64 elapsedTimeMs)
    {
        auto errorCode = error.nativeErrorCode();

        /* If the network connection to the database has been lost due to a period of
           inactivity, the host going to sleep and waking up, or the mysql server having
           restarted, executing a query will result in error code 2006 (MySQL server has
           gone away). In that case we try to reopen the connection to the database and
           re-execute the query. */

        if (errorCode == "2006") /* "MySQL server has gone away" */
            return true;

        qDebug() << "DatabaseConnection: elapsed time:" << elapsedTimeMs;

        if (errorCode == "2013" /* "Lost connection to MySQL server during query" */
            && elapsedTimeMs < 100 /* not because of a timeout */)
        {
            return true;
        }

        return false;
    }

    bool Database::DatabaseConnection::closeAndReopenConnection()
    {
        qDebug() << "DatabaseConnection: closing and reopening connection";
        _db.close();
        _db.setDatabaseName("pmp");

        if (_db.open())
        {
            qDebug() << "DatabaseConnection: reopening successful";
            return true;
        }

        qWarning() << "DatabaseConnection: could not reopen connection after closing";
        return false;
    }

    /* ===== TableEditor ===== */

    Database::TableEditor::TableEditor(Database* database, QString tableName)
        : _database(database), _tableName(tableName)
    {
        //
    }

    bool Database::TableEditor::addColumnIfNotExists(QString columnName, QString type)
    {
        auto preparer =
            [=](QSqlQuery& q)
            {
                q.prepare(
                    "SELECT EXISTS("
                    " SELECT * FROM information_schema.COLUMNS"
                    " WHERE TABLE_SCHEMA = 'pmp' AND TABLE_NAME = ? AND COLUMN_NAME = ?"
                    ") AS column_exists"
                    );
                q.addBindValue(_tableName);
                q.addBindValue(columnName);
            };

        bool exists = false;
        if (!_database->connection().executeScalar(preparer, exists, false))
            return false;

        if (exists)
            return true;

        auto sqlForAddingColumn =
            QString("ALTER TABLE %1 ADD `%2` %3 NULL").arg(_tableName, columnName, type);

        return _database->connection().executeVoid(prepareSimple(sqlForAddingColumn));
    }

    /* ===== Database ===== */

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
        QSqlDatabase qSqlDatabase = createQSqlDatabase("PMP_main_dbconn", false);
        if (!qSqlDatabase.isOpen())
        {
            out << " ERROR: could not connect to database: "
                << qSqlDatabase.lastError().text() << "\n" << Qt::endl;
            return false;
        }

        /* create schema if needed */
        QSqlQuery q(qSqlDatabase);
        q.prepare("CREATE DATABASE IF NOT EXISTS pmp; USE pmp;");
        if (!q.exec())
        {
            printInitializationError(out, qSqlDatabase);
            return false;
        }

        Database database { DatabaseConnection(qSqlDatabase) };

        if (initTables(out, database))
        {
            _initDoneSuccessfully = true;
            out << " database initialization completed successfully\n" << Qt::endl;
            return true;
        }
        else
        {
            printInitializationError(out, database);
            return false;
        }
    }

    void Database::printInitializationError(QTextStream& out, QSqlDatabase& db)
    {
        out << " database initialization problem: " << db.lastError().text() << Qt::endl;
        out << Qt::endl;
    }

    void Database::printInitializationError(QTextStream& out, Database& database)
    {
        printInitializationError(out, database._dbConnection.qSqlDatabase());
    }

    bool Database::initTables(QTextStream& out, Database& db)
    {
        /* create table 'pmp_misc' if needed */
        if (!initMiscTable(db))
        {
            printInitializationError(out, db);
            return false;
        }

        /* get UUID, or generate one and store it if it does not exist yet */
        auto maybeUuid = initDatabaseUuid(db);
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
            initHashesTable(db)                  /* pmp_hash */
            && initFilenameTable(db)             /* pmp_filename */
            && initFileSizeTable(db)             /* pmp_filesize */
            && initUsersTable(db)                /* pmp_user */
            && initHistoryTable(db)              /* pmp_history */
            && initEquivalenceTable(db)          /* pmp_equivalence */
            && initUserHashStatsCacheTable(db);  /* pmp_userhashstatscache */

        if (!tablesInitialized)
            return false;

        bool dataInitialized =
            initUserHashStatsCacheBookkeeping(db);

        return dataInitialized;
    }

    bool Database::initMiscTable(Database& database)
    {
        auto sql =
            "CREATE TABLE IF NOT EXISTS pmp_misc("
            " `Key` VARCHAR(63) NOT NULL,"
            " `Value` VARCHAR(255),"
            " PRIMARY KEY(`Key`) "
            ") "
            "ENGINE = InnoDB "
            "DEFAULT CHARACTER SET = utf8 COLLATE = utf8_general_ci";

        return database.connection().executeVoid(prepareSimple(sql));
    }

    ResultOrError<QUuid, FailureType> Database::initDatabaseUuid(Database& database)
    {
        auto existingUuidValue = database.getMiscDataValue("UUID");
        if (existingUuidValue.failed())
            return failure;

        if (existingUuidValue.result().hasValue())
        {
            auto uuid = QUuid(existingUuidValue.result().value());
            qDebug() << "database UUID is:" << uuid;
            return uuid;
        }

        auto uuid { QUuid::createUuid() };

        auto insertResult = database.insertMiscData("UUID", uuid.toString());
        if (insertResult.failed())
            return failure;

        qDebug() << "generated new UUID for the database:" << uuid;
        return uuid;
    }

    bool Database::initHashesTable(Database& database)
    {
        auto sql =
            "CREATE TABLE IF NOT EXISTS pmp_hash("
            " `HashID` INT UNSIGNED NOT NULL AUTO_INCREMENT,"
            " `InputLength` INT UNSIGNED NOT NULL,"
            " `SHA1` VARCHAR(40) NOT NULL,"
            " `MD5` VARCHAR(32) NOT NULL,"
            " PRIMARY KEY (`HashID`),"
            " UNIQUE INDEX `IDX_pmphash` (`InputLength` ASC, `SHA1` ASC, `MD5` ASC) "
            ") ENGINE = InnoDB";

        return database.connection().executeVoid(prepareSimple(sql));
    }

    bool Database::initFilenameTable(Database& database)
    {
        auto sql =
            "CREATE TABLE IF NOT EXISTS pmp_filename("
            " `HashID` INT UNSIGNED NOT NULL,"
            " `FilenameWithoutDir` VARCHAR(255) NOT NULL,"
            " CONSTRAINT `FK_pmpfilenamehashid`"
            "  FOREIGN KEY (`HashID`)"
            "   REFERENCES pmp_hash (`HashID`)"
            "   ON DELETE CASCADE ON UPDATE CASCADE"
            ") "
            "ENGINE = InnoDB "
            "DEFAULT CHARACTER SET = utf8 COLLATE = utf8_general_ci";

        if (!database.connection().executeVoid(prepareSimple(sql)))
            return false;

        TableEditor editor(&database, "pmp_filename");

        return editor.addColumnIfNotExists("YearLastSeen", "INT");
    }

    bool Database::initFileSizeTable(Database& database)
    {
        auto sql =
            "CREATE TABLE IF NOT EXISTS pmp_filesize("
            " `HashID` INT UNSIGNED NOT NULL,"
            " `FileSize` BIGINT NOT NULL," /* signed; Qt uses qint64 for file sizes */
            " CONSTRAINT `FK_pmpfilesizehashid`"
            "  FOREIGN KEY (`HashID`)"
            "   REFERENCES pmp_hash (`HashID`)"
            "   ON DELETE CASCADE ON UPDATE CASCADE,"
            " UNIQUE INDEX `IDX_pmpfilesize` (`FileSize` ASC, `HashID` ASC) "
            ") ENGINE = InnoDB";

        if (!database.connection().executeVoid(prepareSimple(sql)))
            return false;

        TableEditor editor(&database, "pmp_filesize");

        return editor.addColumnIfNotExists("YearLastSeen", "INT");
    }

    bool Database::initUsersTable(Database& database)
    {
        auto sql =
            "CREATE TABLE IF NOT EXISTS pmp_user("
            " `UserID` INT UNSIGNED NOT NULL AUTO_INCREMENT,"
            " `Login` VARCHAR(63) NOT NULL,"
            " `Salt` VARCHAR(255),"
            " `Password` VARCHAR(255),"
            " PRIMARY KEY (`UserID`),"
            " UNIQUE INDEX `IDX_pmpuser_login` (`Login`)"
            ") "
            "ENGINE = InnoDB "
            "DEFAULT CHARACTER SET = utf8 COLLATE = utf8_general_ci";

        if (!database.connection().executeVoid(prepareSimple(sql)))
            return false;

        TableEditor editor(&database, "pmp_user");

        /* add columns for dynamic mode preferences */
        if (!editor.addColumnIfNotExists("DynamicModeEnabled", "BIT")
            || !editor.addColumnIfNotExists("TrackRepetitionAvoidanceIntervalSeconds",
                                            "INT"))
        {
            return false;
        }

        /* extra columns for Last.Fm scrobbling */
        if (!editor.addColumnIfNotExists("EnableLastFmScrobbling", "BIT")
            || !editor.addColumnIfNotExists("LastFmUser", "VARCHAR(255)")
            || !editor.addColumnIfNotExists("LastFmSessionKey", "VARCHAR(255)")
            || !editor.addColumnIfNotExists("LastFmScrobbledUpTo", "INT UNSIGNED"))
        {
            return false;
        }

        return true;
    }

    bool Database::initHistoryTable(Database& database)
    {
        auto sql =
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
            "DEFAULT CHARACTER SET = utf8 COLLATE = utf8_general_ci";

        return database.connection().executeVoid(prepareSimple(sql));
    }

    bool Database::initEquivalenceTable(Database& database)
    {
        auto sql =
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
            ") ENGINE = InnoDB";

        return database.connection().executeVoid(prepareSimple(sql));
    }

    bool Database::initUserHashStatsCacheTable(Database& database)
    {
        auto sql =
            "CREATE TABLE IF NOT EXISTS pmp_userhashstatscache("
            " `HashID` INT UNSIGNED NOT NULL,"
            " `UserID` INT UNSIGNED,"
            " `Timestamp` DATETIME NOT NULL,"
            " `LastHistoryID` INT UNSIGNED,"
            " `LastHeard` DATETIME,"
            " `ScoreHeardCount` INT NOT NULL,"
            " `AveragePermillage` INT,"
            " CONSTRAINT `FK_pmpuserhashstatscache_hash`"
            "  FOREIGN KEY (`HashID`)"
            "   REFERENCES pmp_hash (`HashID`)"
            "   ON DELETE CASCADE ON UPDATE CASCADE,"
            " CONSTRAINT `FK_pmpuserhashstatscache_user`"
            "  FOREIGN KEY (`UserID`)"
            "  REFERENCES `pmp_user` (`UserID`)"
            "   ON DELETE RESTRICT ON UPDATE CASCADE,"
            " UNIQUE INDEX `IDX_pmpuserhashstatscache` (`UserID` ASC, `HashID` ASC)"
            ") ENGINE = InnoDB";

        return database.connection().executeVoid(prepareSimple(sql));
    }

    bool Database::initUserHashStatsCacheBookkeeping(Database& database)
    {
        auto lookupResult = database.getMiscDataValue("UserHashStatsCacheHistoryId");
        if (lookupResult.failed())
            return false;

        auto idAsString = lookupResult.result().valueOr("");

        if (!idAsString.isEmpty())
            return true; /* nothing to do */

        auto lastHistoryIdOrFailure = database.getLastHistoryId();
        if (lastHistoryIdOrFailure.failed())
            return false;

        uint lastHistoryId = lastHistoryIdOrFailure.result();

        auto insertResult =
            database.insertOrUpdateMiscData("UserHashStatsCacheHistoryId",
                                            QString::number(lastHistoryId));

        return insertResult.succeeded();
    }

    Database::Database(DatabaseConnection&& databaseConnection)
        : _dbConnection(std::move(databaseConnection))
    {
        //
    }

    QSqlDatabase Database::createQSqlDatabase(QString name, bool setSchema)
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

        QSqlDatabase qsqlDb = createQSqlDatabase(dbName, true);
        QSharedPointer<Database> dbPtr(new Database(DatabaseConnection(qsqlDb)));

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
        return _dbConnection.isOpen();
    }

    ResultOrError<Nullable<QString>, FailureType> Database::getMiscDataValue(
        const QString& key)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare("SELECT `Value` FROM pmp_misc WHERE `Key`=?");
                q.addBindValue(key);
            };

        Nullable<QString> value;
        if (!_dbConnection.executeNullableScalar(preparer, value))
            return failure;

        return value;
    }

    ResultOrError<SuccessType, FailureType> Database::insertMiscData(const QString& key,
                                                                     const QString& value)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare("INSERT INTO pmp_misc(`Key`, `Value`) "
                          "VALUES(?,?)");
                q.addBindValue(key);
                q.addBindValue(value);
            };

        if (_dbConnection.executeVoid(preparer))
            return success;
        else
            return failure;
    }

    ResultOrError<SuccessType, FailureType> Database::insertOrUpdateMiscData(
                                                                    const QString& key,
                                                                    const QString& value)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare("INSERT INTO pmp_misc(`Key`, `Value`) "
                          "VALUES(?,?) "
                          "ON DUPLICATE KEY UPDATE `Value`=?");
                q.addBindValue(key);
                q.addBindValue(value);
                q.addBindValue(value);
            };

        if (_dbConnection.executeVoid(preparer))
            return success;
        else
            return failure;
    }

    ResultOrError<SuccessType, FailureType> Database::insertMiscDataIfNotPresent(
                                                                    const QString& key,
                                                                    const QString& value)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare("INSERT INTO pmp_misc(`Key`, `Value`) "
                          "VALUES(?,?) "
                          "ON DUPLICATE KEY UPDATE `Value`=`Value`");
                q.addBindValue(key);
                q.addBindValue(value);
            };

        if (_dbConnection.executeVoid(preparer))
            return success;
        else
            return failure;
    }

    ResultOrError<SuccessType, FailureType> Database::updateMiscDataValueFromSpecific(
                                                                const QString& key,
                                                                const QString& oldValue,
                                                                const QString& newValue)
    {
        auto preparer =
            [=] (QSqlQuery& q)
        {
            q.prepare("UPDATE pmp_misc "
                      "SET `Value`=? "
                      "WHERE `Key`=? AND `Value`=?");
            q.addBindValue(newValue);
            q.addBindValue(key);
            q.addBindValue(oldValue);
        };

        if (_dbConnection.executeVoid(preparer))
            return success;
        else
            return failure;
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

        if (!_dbConnection.executeVoid(preparer))
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
        if (!_dbConnection.executeScalar(preparer, id, 0))
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

        return _dbConnection.executeRecords<QPair<uint, FileHash>>(preparer,
                                                                   extractRecord);
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
        if (!_dbConnection.executeScalar(preparer1, oldYear, -1))
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

        if (!_dbConnection.executeVoid(preparer2))
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

        if (!_dbConnection.executeVoid(preparer3))
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

        return _dbConnection.executeRecords<QString>(preparer, extractRecord);
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

        if (!_dbConnection.executeVoid(preparer))
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

        return _dbConnection.executeRecords<qint64>(preparer, extractRecord);
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

        return _dbConnection.executeRecords<User>(preparer, extractRecord);
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
        if (!_dbConnection.executeScalar(preparer, exists, false)) /* error */
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

        if (!_dbConnection.executeVoid(preparer)) /* error */
        {
            qDebug() << "Database::registerNewUser : insert failed!" << Qt::endl;
            return failure;
        }

        auto preparer2 = prepareSimple("SELECT LAST_INSERT_ID()");

        uint userId = 0;
        if (!_dbConnection.executeScalar(preparer2, userId, 0))
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

        if (!_dbConnection.executeQuery(preparer, true, resultGetter)) /* error */
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

        if (!_dbConnection.executeQuery(preparer, true, resultGetter)) /* error */
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

        if (!_dbConnection.executeQuery(preparer, true, resultGetter))
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

        if (!_dbConnection.executeVoid(preparer))
        {
            qWarning() << "Database::setUserDynamicModePreferences : could not execute";
            return failure;
        }

        return success;
    }

    ResultOrError<uint, FailureType> Database::addToHistory(quint32 hashId,
                                                            quint32 userId,
                                                            QDateTime start,
                                                            QDateTime end, int permillage,
                                                            bool validForScoring)
    {
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

        if (!_dbConnection.executeVoid(preparer)) /* error */
        {
            qWarning() << "Database::addToHistory : insert failed!" << Qt::endl;
            return failure;
        }

        auto preparer2 = prepareSimple("SELECT LAST_INSERT_ID()");

        uint historyId = 0;
        if (!_dbConnection.executeScalar(preparer2, historyId, 0))
            return failure;

        qDebug() << "inserted new history entry with ID" << historyId
                 << "for user" << userId << "and hash ID" << hashId;
        return historyId;
    }

    ResultOrError<uint, FailureType> Database::getLastHistoryId()
    {
        uint maxHistoryId;
        bool maxHistoryIdObtained =
            _dbConnection.executeScalar(
                prepareSimple("SELECT MAX(HistoryID) FROM pmp_history"),
                maxHistoryId,
                0
            );

        if (!maxHistoryIdObtained)
            return failure;

        return maxHistoryId;
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
        if (!_dbConnection.executeScalar(preparer, userId, 0))
        {
            qDebug() << "Database::getMostRecentRealUserHavingHistory : select failed!"
                     << Qt::endl;
            return failure;
        }

        return userId;
    }

    ResultOrError<QVector<BriefHistoryRecord>, FailureType>
        Database::getBriefHistoryFragment(quint32 startId, int limit)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "SELECT HistoryID, HashID, UserID "
                    "FROM pmp_history "
                    "WHERE HistoryID >= ? "
                    "ORDER BY HistoryID "
                    "LIMIT ?"
                );
                q.addBindValue(startId);
                q.addBindValue(limit);
            };

        auto extractRecord =
            [](QSqlQuery& q)
            {
                BriefHistoryRecord record;
                record.id = q.value(0).toUInt();
                record.hashId = q.value(1).toUInt();
                record.userId = q.value(2).toUInt();

                return record;
            };

        return _dbConnection.executeRecords<BriefHistoryRecord>(preparer, extractRecord,
                                                                limit);
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

        return _dbConnection.executeRecords<HashHistoryStats>(preparer, extractRecord,
                                                              hashIds.size());
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

        return _dbConnection.executeRecords<QPair<quint32, quint32>>(preparer,
                                                                     extractRecord);
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

        if (!_dbConnection.executeVoid(preparer))
        {
            qDebug() << "Database::registerEquivalence : insert/update failed!"
                     << Qt::endl;
            return failure;
        }

        return success;
    }

    ResultOrError<QVector<HistoryRecord>, FailureType>
        Database::getUserHistoryForScrobbling(quint32 userId, quint32 startId,
                                              QDateTime earliestDateTime, int limit)
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

        auto extractRecord =
            [](QSqlQuery& q)
            {
                HistoryRecord record;
                record.id = q.value(0).toUInt();
                record.hashId = q.value(1).toUInt();
                record.userId = q.value(2).toUInt();
                record.start = getUtcDateTime(q.value(3));
                record.end = getUtcDateTime(q.value(4));
                record.permillage = qint16(q.value(5).toInt());
                record.validForScoring = getBool(q.value(6), false);

                return record;
            };

        return _dbConnection.executeRecords<HistoryRecord>(preparer, extractRecord,
                                                           limit);
    }

    ResultOrError<QVector<HistoryRecord>, FailureType> Database::getTrackHistoryForUser(
                        quint32 userId, QVector<quint32> hashIds, uint startId, int limit)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                auto sql =
                    (startId > 0)
                    ? "SELECT"
                      " HistoryID,HashID,UserID,`Start`,`End`,Permillage,ValidForScoring "
                      "FROM pmp_history "
                      "WHERE HashID IN " + buildParamsList(hashIds.size()) +
                      " AND COALESCE(UserID, 0)=? AND HistoryID < ? "
                      "ORDER BY HistoryID DESC "
                      "LIMIT ?"
                    : "SELECT"
                      " HistoryID,HashID,UserID,`Start`,`End`,Permillage,ValidForScoring "
                      "FROM pmp_history "
                      "WHERE HashID IN " + buildParamsList(hashIds.size()) +
                      " AND COALESCE(UserID, 0)=? "
                      "ORDER BY HistoryID DESC "
                      "LIMIT ?";

                q.prepare(sql);
                for (auto hashId : qAsConst(hashIds))
                {
                    q.addBindValue(hashId);
                }
                q.addBindValue(userId);

                if (startId > 0)
                    q.addBindValue(startId);

                q.addBindValue(limit);
            };

        auto extractRecord =
            [](QSqlQuery& q)
            {
                HistoryRecord record;
                record.id = q.value(0).toUInt();
                record.hashId = q.value(1).toUInt();
                record.userId = q.value(2).toUInt();
                record.start = getUtcDateTime(q.value(3));
                record.end = getUtcDateTime(q.value(4));
                record.permillage = qint16(q.value(5).toInt());
                record.validForScoring = getBool(q.value(6), false);

                return record;
            };

        return _dbConnection.executeRecords<HistoryRecord>(preparer, extractRecord,
                                                           limit);
    }

    ResultOrError<QVector<HashHistoryStats>, FailureType> Database::getCachedHashStats(
                                                                quint32 userId,
                                                                QVector<quint32> hashIds)
    {
        auto preparer =
            [=] (QSqlQuery& q)
        {
            auto sql =
                "SELECT HashID, LastHistoryId, LastHeard, ScoreHeardCount,"
                " AveragePermillage "
                "FROM pmp_userhashstatscache "
                "WHERE COALESCE(UserID, 0)=?"
                "  AND HashID IN " + buildParamsList(hashIds.size());

            q.prepare(sql);

            q.addBindValue(userId);
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
                QDateTime lastHeard = getUtcDateTime(q.value(2));
                quint32 scoreHeardCount = (quint32)getUInt(q.value(3), 0);
                qint32 averagePermillage = (qint32)getInt(q.value(4), -1);

                HashHistoryStats stats;
                stats.lastHistoryId = lastHistoryId;
                stats.hashId = hashID;
                stats.lastHeard = lastHeard;
                stats.scoreHeardCount = scoreHeardCount;
                stats.averagePermillage = averagePermillage;

                return stats;
            };

        return _dbConnection.executeRecords<HashHistoryStats>(preparer, extractRecord);
    }

    SuccessOrFailure Database::updateUserHashStatsCacheEntry(quint32 userId,
                                                            const HashHistoryStats& stats)
    {
        auto preparer =
            [=] (QSqlQuery& q)
        {
            auto sql =
                "INSERT INTO pmp_userhashstatscache("
                " `HashID`,`UserID`,`Timestamp`,`LastHistoryID`,`LastHeard`,"
                " `ScoreHeardCount`,`AveragePermillage`) "
                "VALUES(?,?,UTC_TIMESTAMP(),?,?,?,?) "
                "ON DUPLICATE KEY UPDATE"
                " HashID=?, UserID=?, `Timestamp`=UTC_TIMESTAMP(),"
                " LastHistoryID=?,LastHeard=?,ScoreHeardCount=?,AveragePermillage=?";

            q.prepare(sql);

            q.addBindValue(stats.hashId);
            q.addBindValue(userId == 0 ? /*NULL*/QVariant(QVariant::UInt) : userId);
            q.addBindValue(stats.lastHistoryId);
            q.addBindValue(stats.lastHeard.toUTC());
            q.addBindValue(stats.scoreHeardCount);
            q.addBindValue(stats.averagePermillage);

            q.addBindValue(stats.hashId);
            q.addBindValue(userId == 0 ? /*NULL*/QVariant(QVariant::UInt) : userId);
            q.addBindValue(stats.lastHistoryId);
            q.addBindValue(stats.lastHeard.toUTC());
            q.addBindValue(stats.scoreHeardCount);
            q.addBindValue(stats.averagePermillage);
        };

        if (!_dbConnection.executeVoid(preparer))
        {
            qDebug() << "Database::updateUserHashStatsCache : insert/update failed!"
                     << Qt::endl;
            return failure;
        }

        return success;
    }

    SuccessOrFailure Database::removeUserHashStatsCacheEntry(quint32 userId,
                                                             quint32 hashId)
    {
        auto preparer =
            [=] (QSqlQuery& q)
            {
                q.prepare(
                    "DELETE FROM pmp_userhashstatscache "
                    "WHERE UserId=? AND HashID=?"
                    );
                q.addBindValue(userId);
                q.addBindValue(hashId);
            };

        if (!_dbConnection.executeVoid(preparer))
            return failure;

        return success;
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

        if (!_dbConnection.executeVoid(preparer))
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
        bool success = _dbConnection.executeScalar(preparer, result, 0);

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

        return _dbConnection.executeVoid(preparer);
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

        return _dbConnection.executeVoid(preparer);
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

        return _dbConnection.executeVoid(preparer);
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

    std::function<void (QSqlQuery&)> Database::prepareSimple(QString sql)
    {
        return [=] (QSqlQuery& q) { q.prepare(sql); };
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

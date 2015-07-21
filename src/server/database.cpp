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

#include "database.h"

#include "common/hashid.h"

#include "serversettings.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>
#include <QTextStream>
#include <QUuid>

namespace PMP {

    Database* Database::_instance = 0;

    bool Database::init(QTextStream& out) {
        out << "initializing database" << endl;

        ServerSettings serversettings;
        QSettings& settings = serversettings.getSettings();

        bool incompleteDbSettings = false;

        QVariant hostname = settings.value("database/hostname");
        if (!hostname.isValid() || hostname.toString() == "") {
            settings.setValue("database/hostname", "");
            incompleteDbSettings = true;
        }

        QVariant user = settings.value("database/username");
        if (!user.isValid() || user.toString() == "") {
            settings.setValue("database/username", "");
            incompleteDbSettings = true;
        }

        QVariant password = settings.value("database/password");
        if (!password.isValid() || password.toString() == "") {
            settings.setValue("database/password", "");
            incompleteDbSettings = true;
        }

    //    QVariant schema = settings.value("database/schema");
    //    if (!schema.isValid() || schema.toString() == "") {
    //        settings.setValue("database/schema", "");
    //        incompleteDbSettings = true;
    //    }

        if (incompleteDbSettings) {
            out << " incomplete database settings!" << endl << endl;
            return false;
        }

        /* open connection */
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "PMP_main_dbconn");
        db.setHostName(hostname.toString());
        db.setUserName(user.toString());
        db.setPassword(password.toString());
        if (!db.open()) {
            out << " ERROR: could not connect to database: " << db.lastError().text()
                << endl << endl;
            return false;
        }

        /* create schema if needed */
        QSqlQuery q(db);
        q.prepare("CREATE DATABASE IF NOT EXISTS pmp; USE pmp;");
        if (!q.exec()) {
            out << " database initialization problem: " << db.lastError().text() << endl
                << endl;
            return false;
        }

        /* create table 'pmp_misc' if needed */
        q.prepare(
            "CREATE TABLE IF NOT EXISTS pmp_misc("
            " `Key` VARCHAR(63) NOT NULL,"
            " `Value` VARCHAR(255),"
            " PRIMARY KEY(`Key`) "
            ") "
            "ENGINE = InnoDB "
            "DEFAULT CHARACTER SET = utf8 COLLATE = utf8_general_ci"
        );
        if (!q.exec()) {
            out << " database initialization problem: " << db.lastError().text() << endl
                << endl;
            return false;
        }

        /* get UUID, or generate one and store it if it does not exist yet */
        q.prepare("SELECT `Value` FROM pmp_misc WHERE `Key`=?");
        q.addBindValue("UUID");
        if (!q.exec()) {
            out << " error: could not see if UUID already exists" << endl << endl;
            return false;
        }
        QUuid uuid;
        if (q.next()) {
            uuid = QUuid(q.value(0).toString());
        }
        else {
            uuid = QUuid::createUuid();
            q.prepare("INSERT INTO pmp_misc(`Key`, `Value`) VALUES (?,?)");
            q.addBindValue("UUID");
            q.addBindValue(uuid.toString());
            if (!q.exec()) {
                out << " error inserting UUID into database" << endl << endl;
                return false;
            }
        }
        out << " UUID is " << uuid.toString() << endl;

        /* create table 'pmp_hash' if needed */
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
        if (!q.exec()) {
            out << " database initialization problem: " << db.lastError().text() << endl
                << endl;
            return false;
        }

        /* create table 'pmp_filename' if needed */
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
        if (!q.exec()) {
            out << " database initialization problem: " << db.lastError().text() << endl
                << endl;
            return false;
        }

        /* create table 'pmp_user' if needed */
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
        if (!q.exec()) {
            out << " database initialization problem: " << db.lastError().text() << endl
                << endl;
            return false;
        }

        /* create table 'pmp_history' if needed */
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
        if (!q.exec()) {
            out << " database initialization problem: " << db.lastError().text() << endl
                << endl;
            return false;
        }

        _instance = new Database();
        _instance->_db = db;

        out << " database initialization completed successfully" << endl << endl;
        return true;
    }

    void Database::registerHash(const HashID& hash) {
        QString sha1 = hash.SHA1().toHex();
        QString md5 = hash.MD5().toHex();

        auto preparer =
            [=] (QSqlQuery& q) {
                q.prepare(
                    "INSERT IGNORE INTO pmp_hash(InputLength, `SHA1`, `MD5`) "
                    "VALUES(?,?,?)"
                );
                q.addBindValue(hash.length());
                q.addBindValue(sha1);
                q.addBindValue(md5);
            };

        if (!executeVoid(preparer)) { /* error */
            qDebug() << "Database::registerHash : insert failed!" << endl;
        }
    }

    uint Database::getHashID(const HashID& hash) {
        QString sha1 = hash.SHA1().toHex();
        QString md5 = hash.MD5().toHex();

        auto preparer =
            [=] (QSqlQuery& q) {
                q.prepare(
                    "SELECT HashID FROM pmp_hash"
                    " WHERE InputLength=? AND `SHA1`=? AND `MD5`=?"
                );
                q.addBindValue(hash.length());
                q.addBindValue(sha1);
                q.addBindValue(md5);
            };

        uint id;
        if (!executeScalar(preparer, id, 0)) { /* error */
            qDebug() << "Database::getHashID : query failed!" << endl;
            return 0;
        }

        return id;
    }

    QList<QPair<uint,HashID> > Database::getHashes(uint largerThanID) {
        QSqlQuery q(_db);
        q.prepare(
            "SELECT HashID,InputLength,`SHA1`,`MD5` FROM pmp_hash"
            " WHERE HashID > ? "
            "ORDER BY HashID"
        );
        q.addBindValue(largerThanID);

        QList<QPair<uint,HashID> > result;

        if (!executeQuery(q)) { /* error */
            qDebug() << "Database::getHashes : could not execute; "
                     << q.lastError().text() << endl;
            return result;
        }

        while (q.next()) {
            uint hashID = q.value(0).toUInt();
            uint length = q.value(1).toUInt();
            QByteArray sha1 = QByteArray::fromHex(q.value(2).toByteArray());
            QByteArray md5 = QByteArray::fromHex(q.value(3).toByteArray());
            result.append(QPair<uint,HashID>(hashID, HashID(length, sha1, md5)));
        }

        return result;
    }

    void Database::registerFilename(uint hashID, const QString& filenameWithoutPath) {
        /* We do not support extremely long file names.  Lookup for those files should be
           done by other means. */
        if (filenameWithoutPath.length() > 255) return;

        /* A race condition could cause duplicate records to be registered; that is
           tolerable however. */

        auto preparer =
            [=] (QSqlQuery& q) {
                q.prepare(
                    "SELECT EXISTS("
                    " SELECT * FROM pmp_filename"
                    "  WHERE `HashID`=? AND `FilenameWithoutDir`=? "
                    ")"
                );
                q.addBindValue(hashID);
                q.addBindValue(filenameWithoutPath);
            };

        bool exists;
        if (!executeScalar(preparer, exists, false)) {
            qDebug() << "Database::registerFilename : select failed!" << endl;
            return;
        }

        if (exists) return; /* already registered */

        auto preparer2 =
            [=] (QSqlQuery& q) {
                q.prepare(
                    "INSERT INTO pmp_filename(`HashID`,`FilenameWithoutDir`)"
                    " VALUES(?,?)"
                );
                q.addBindValue(hashID);
                q.addBindValue(filenameWithoutPath);
            };

        if (!executeVoid(preparer2)) {
            qDebug() << "Database::registerFilename : insert failed!" << endl;
            return;
        }
    }

    QList<QString> Database::getFilenames(uint hashID) {
        QSqlQuery q(_db);
        q.prepare(
            "SELECT `FilenameWithoutDir` FROM pmp_filename"
            " WHERE HashID=?"
        );
        q.addBindValue(hashID);

        QList<QString> result;

        if (!executeQuery(q)) { /* error */
            qDebug() << "Database::getFilenames : could not execute; "
                     << q.lastError().text() << endl;
            return result;
        }

        while (q.next()) {
            QString name = q.value(0).toString();
            result.append(name);
        }

        return result;
    }

    QList<User> Database::getUsers() {
        QSqlQuery q(_db);
        q.prepare("SELECT `UserID`,`Login`,`Salt`,`Password` FROM pmp_user");

        QList<User> result;

        if (!executeQuery(q)) { /* error */
            qDebug() << "Database::getUsers : could not execute; "
                     << q.lastError().text() << endl;
            return result;
        }

        while (q.next()) {
            quint32 userID = q.value(0).toUInt();
            QString login = q.value(1).toString();
            QString salt = q.value(2).toString();
            QString password = q.value(3).toString();

            result.append(User::fromDb(userID, login, salt, password));
        }

        qDebug() << "Database::getUsers() : user count:" << result.size();

        return result;
    }

    bool Database::checkUserExists(QString userName) {
        auto preparer =
            [=] (QSqlQuery& q) {
                q.prepare(
                    "SELECT EXISTS(SELECT * FROM pmp_user WHERE LOWER(`Login`)=LOWER(?))"
                );
                q.addBindValue(userName);
            };

        bool exists;
        if (!executeScalar(preparer, exists, false)) { /* error */
            qDebug() << "Database::checkUserExists : query failed!" << endl;
            return false; // FIXME ???
        }

        return exists;
    }

    quint32 Database::registerNewUser(User& user) {
        auto preparer =
            [=] (QSqlQuery& q) {
                q.prepare(
                    "INSERT INTO pmp_user(`Login`,`Salt`,`Password`) VALUES(?,?,?)"
                );
                q.addBindValue(user.login);
                q.addBindValue(QString::fromLatin1(user.salt.toBase64()));
                q.addBindValue(QString::fromLatin1(user.password.toBase64()));
            };

        if (!executeVoid(preparer)) { /* error */
            qDebug() << "Database::registerNewUser : insert failed!" << endl;
            return 0;
        }

        auto preparer2 = prepareSimple("SELECT LAST_INSERT_ID()");

        uint userId = 0;
        if (!executeScalar(preparer2, userId, 0)) {
            qDebug() << "Database::registerNewUser : select failed!" << endl;
            return 0;
        }

        return userId;
    }

    void Database::addToHistory(quint32 hashId, quint32 userId, QDateTime start,
                                QDateTime end, int permillage, bool validForScoring)
    {
        qDebug() << "  Database::addToHistory called\n"
                 << "   hash" << hashId << " user" << userId << " perm" << permillage
                 << " forScoring" << validForScoring << "\n"
                 << "   start" << start << "\n"
                 << "   end" << end;

        auto preparer =
            [=] (QSqlQuery& q) {
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

        if (!executeVoid(preparer)) { /* error */
            qDebug() << "Database::addToHistory : query FAILED!" << endl;
            return;
        }

        return;
    }

    bool Database::executeScalar(std::function<void (QSqlQuery&)> preparer, bool& b,
                                 bool defaultValue)
    {
        auto resultGetter =
            [&b, defaultValue] (QSqlQuery& q) {
                if (q.next()) {
                    QVariant v = q.value(0);
                    b = (v.isNull()) ? defaultValue : v.toBool();
                }
                else {
                    b = defaultValue;
                }
            };

        if (executeQuery(preparer, true, resultGetter))
            return true;

        b = defaultValue;
        return false;
    }

    /*
    bool Database::executeScalar(QSqlQuery& q, int& i, int defaultValue) {
        if (!executeQuery(q)) {
            i = defaultValue;
            return false;
        }

        if (q.next()) {
            QVariant v = q.value(0);
            i = (v.isNull()) ? defaultValue : v.toInt();
        }
        else {
            i = defaultValue;
        }

        return true;
    }
    */

    bool Database::executeScalar(std::function<void (QSqlQuery&)> preparer, int& i,
                                 int defaultValue)
    {
        auto resultGetter =
            [&i, defaultValue] (QSqlQuery& q) {
                if (q.next()) {
                    QVariant v = q.value(0);
                    i = (v.isNull()) ? defaultValue : v.toInt();
                }
                else {
                    i = defaultValue;
                }
            };

        if (executeQuery(preparer, true, resultGetter))
            return true;

        i = defaultValue;
        return false;
    }

    /*
    bool Database::executeScalar(QSqlQuery& q, uint& i, uint defaultValue) {
        if (!executeQuery(q)) {
            i = defaultValue;
            return false;
        }

        if (q.next()) {
            QVariant v = q.value(0);
            i = (v.isNull()) ? defaultValue : v.toUInt();
        }
        else {
            i = defaultValue;
        }

        return true;
    }
    */

    bool Database::executeScalar(std::function<void (QSqlQuery&)> preparer, uint& i,
                                 uint defaultValue)
    {
        auto resultGetter =
            [&i, defaultValue] (QSqlQuery& q) {
                if (q.next()) {
                    QVariant v = q.value(0);
                    i = (v.isNull()) ? defaultValue : v.toUInt();
                }
                else {
                    i = defaultValue;
                }
            };

        if (executeQuery(preparer, true, resultGetter))
            return true;

        i = defaultValue;
        return false;
    }

    /*
    bool Database::executeScalar(QSqlQuery& q, QString& s, const QString& defaultValue) {
        if (!executeQuery(q)) {
            s = defaultValue; // NECESSARY?
            return false;
        }

        if (q.next()) {
            QVariant v = q.value(0);
            s = (v.isNull()) ? defaultValue : v.toString();
        }
        else {
            s = defaultValue;
        }

        return true;
    }
    */

    std::function<void (QSqlQuery&)> Database::prepareSimple(QString sql) {
        return [=] (QSqlQuery& q) { q.prepare(sql); };
    }

    bool Database::executeVoid(std::function<void (QSqlQuery&)> preparer) {
        return executeQuery(preparer, false, std::function<void (QSqlQuery&)>());
    }

    bool Database::executeQuery(std::function<void (QSqlQuery&)> preparer,
                                bool processResult,
                                std::function<void (QSqlQuery&)> resultFetcher)
    {
        QSqlQuery q(_db);
        preparer(q);
        if (q.exec()) {
            if (processResult) resultFetcher(q);
            return true;
        }

        /* something went wrong */

        QSqlError error = q.lastError();
        qDebug() << "Database: query failed:" << error.text();
        qDebug() << " error type:" << error.type();
        qDebug() << " error number:" << error.number();
        qDebug() << " db error:" << error.databaseText();
        qDebug() << " driver error:" << error.driverText();
        qDebug() << " sql:" << q.lastQuery();

        if (!_db.isOpen()) {
            qDebug() << " connection not open!";
        }

        if (!_db.isValid()) {
            qDebug() << " connection not valid!";
        }

        if (error.number() != 2006) return false;

        /* An extended sleep period followed by a resume will result in the error "MySQL
           server has gone away", error code 2006. In that case we try to reopen the
           connection and re-execute the query. */

        qDebug() << " will try to re-establish a connection and re-execute the query";
        _db.close();
        _db.setDatabaseName("pmp");
        if (!_db.open()) {
            qDebug() << "  FAILED.  Connection not reopened.";
            return false;
        }

        preparer(q);

        if (!q.exec()) {
            QSqlError error2 = q.lastError();
            qDebug() << "  FAILED to re-execute query:" << error2.text();
            qDebug() << "  error type:" << error2.type();
            qDebug() << "  error number:" << error2.number();
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

    bool Database::executeQuery(QSqlQuery& q) {
        if (q.exec()) return true;

        /* something went wrong */

        QSqlError error = q.lastError();
        qDebug() << "Database: query failed:" << error.text();
        qDebug() << " error type:" << error.type();
        qDebug() << " error number:" << error.number();
        qDebug() << " db error:" << error.databaseText();
        qDebug() << " driver error:" << error.driverText();
        qDebug() << " sql:" << q.lastQuery();

        if (!_db.isOpen()) {
            qDebug() << " connection not open!";
        }

        if (!_db.isValid()) {
            qDebug() << " connection not valid!";
        }

        /* An extended sleep period followed by a resume will result in the error "MySQL
           server has gone away", error code 2006. In that case we try to reopen the
           connection and re-execute the query. */
        if (error.number() == 2006) {
            qDebug() << " will try to re-establish a connection and re-execute the query";
            _db.close();
            _db.setDatabaseName("pmp");
            if (!_db.open()) {
                qDebug() << "  FAILED.  Connection not reopened.";
                return false;
            }

            if (!q.exec()) {
                QSqlError error2 = q.lastError();
                qDebug() << "  FAILED to re-execute query:" << error2.text();
                qDebug() << "  error type:" << error2.type();
                qDebug() << "  error number:" << error2.number();
                qDebug() << "  db error:" << error2.databaseText();
                qDebug() << "  driver error:" << error2.driverText();
                qDebug() << "  sql:" << q.lastQuery();
                qDebug() << "  BAILING OUT.";
                return false;
            }

            qDebug() << "  SUCCESS!";
            return true;
        }

        return false;
    }

}

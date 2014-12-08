/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName(hostname.toString());
        db.setUserName(user.toString());
        db.setPassword(password.toString());
        if (!db.open()) {
            out << " ERROR: could not connect to database: " << db.lastError().text() << endl << endl;
            return false;
        }

        /* create schema if needed */
        QSqlQuery q(db);
        q.prepare("CREATE DATABASE IF NOT EXISTS pmp; USE pmp;");
        if (!q.exec()) {
            out << " database initialization problem: " << db.lastError().text() << endl << endl;
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
            out << " database initialization problem: " << db.lastError().text() << endl << endl;
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
            out << " database initialization problem: " << db.lastError().text() << endl << endl;
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
            out << " database initialization problem: " << db.lastError().text() << endl << endl;
            return false;
        }

        _instance = new Database();

        out << " database initialization completed successfully" << endl << endl;
        return true;
    }

    void Database::registerHash(const HashID& hash) {
        QString sha1 = hash.SHA1().toHex();
        QString md5 = hash.MD5().toHex();

        QSqlQuery q;
        q.prepare(
            "INSERT IGNORE INTO pmp_hash(InputLength, `SHA1`, `MD5`) "
            "VALUES(?,?,?)"
        );
        q.addBindValue(hash.length());
        q.addBindValue(sha1);
        q.addBindValue(md5);

        if (!q.exec()) { /* error */
            qDebug() << "Database::registerHash : could not execute; " << q.lastError().text() << endl;
        }
    }

    uint Database::getHashID(const HashID& hash) {
        QString sha1 = hash.SHA1().toHex();
        QString md5 = hash.MD5().toHex();

        QSqlQuery q;
        q.prepare(
            "SELECT HashID FROM pmp_hash"
            " WHERE InputLength=? AND `SHA1`=? AND `MD5`=?"
        );
        q.addBindValue(hash.length());
        q.addBindValue(sha1);
        q.addBindValue(md5);

        if (!q.exec()) { /* error */
            qDebug() << "Database::getHashID : could not execute; " << q.lastError().text() << endl;
            return 0;
        }
        else if (!q.next()){
            qDebug() << "Database::getHashID : no result" << endl;
            return 0;
        }

        return q.value(0).toUInt();
    }

    QList<QPair<uint,HashID> > Database::getHashes(uint largerThanID) {
        QSqlQuery q;
        q.prepare(
            "SELECT HashID,InputLength,`SHA1`,`MD5` FROM pmp_hash"
            " WHERE HashID > ? "
            "ORDER BY HashID"
        );
        q.addBindValue(largerThanID);

        QList<QPair<uint,HashID> > result;

        if (!q.exec()) { /* error */
            qDebug() << "Database::getHashes : could not execute; " << q.lastError().text() << endl;
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

        QSqlQuery q;
        q.prepare(
            "SELECT EXISTS("
            " SELECT * FROM pmp_filename WHERE `HashID`=? AND `FilenameWithoutDir`=? "
            ")"
        );
        q.addBindValue(hashID);
        q.addBindValue(filenameWithoutPath);
        int i;
        if (!executeScalar(q, i)) {
            qDebug() << "Database::registerFilename : could not execute; " << q.lastError().text() << endl;
            return;
        }

        if (i != 0) return; /* already registered */

        q.prepare(
            "INSERT INTO pmp_filename(`HashID`,`FilenameWithoutDir`)"
            " VALUES(?,?)"
        );
        q.addBindValue(hashID);
        q.addBindValue(filenameWithoutPath);
        if (!q.exec()) {
            qDebug() << "Database::registerFilename : could not execute; " << q.lastError().text() << endl;
            return;
        }
    }

    QList<QString> Database::getFilenames(uint hashID) {
        QSqlQuery q;
        q.prepare(
            "SELECT `FilenameWithoutDir` FROM pmp_filename"
            " WHERE HashID=?"
        );
        q.addBindValue(hashID);

        QList<QString> result;

        if (!q.exec()) { /* error */
            qDebug() << "Database::getFilenames : could not execute; " << q.lastError().text() << endl;
            return result;
        }

        while (q.next()) {
            QString name = q.value(0).toString();
            result.append(name);
        }

        return result;
    }

    bool Database::executeScalar(QSqlQuery& q, int& i, const int& defaultValue) {
        if (!q.exec()) {
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

    bool Database::executeScalar(QSqlQuery& q, QString& s, const QString& defaultValue) {
        if (!q.exec()) {
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

}

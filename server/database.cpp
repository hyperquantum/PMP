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

#include <QCoreApplication>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>
#include <QUuid>

namespace PMP {

    Database* Database::_instance = 0;

    bool Database::init(QTextStream& out) {
        out << "initializing database" << endl;

        QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                           QCoreApplication::organizationName(),
                           QCoreApplication::applicationName());
        settings.setIniCodec("UTF-8");

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
        QSqlQuery q;
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
            qDebug() << "Database::registerHashID : could not execute; " << q.lastError().text() << endl;
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

}

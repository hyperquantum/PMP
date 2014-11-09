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

#include <QCoreApplication>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace PMP {

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

        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName(hostname.toString());
        db.setUserName(user.toString());
        db.setPassword(password.toString());
        bool dbOk = db.open();

        if (!dbOk) {
            out << " ERROR: could not connect to database: " << db.lastError().text() << endl << endl;
            return false;
        }

        QSqlQuery q;
        q.prepare("CREATE DATABASE IF NOT EXISTS pmp; USE pmp;");
        if (!q.exec()) {
            out << " database initialization problem: " << db.lastError().text() << endl << endl;
            return false;
        }

        q.prepare("CREATE TABLE IF NOT EXISTS pmp_misc(`Key` VARCHAR(63) NOT NULL, `Value` VARCHAR(255), PRIMARY KEY(`Key`) )");
        if (!q.exec()) {
            out << " database initialization problem: " << db.lastError().text() << endl << endl;
            return false;
        }

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

        out << " database initialization completed successfully" << endl << endl;
        return true;
    }

}

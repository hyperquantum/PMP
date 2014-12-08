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

#ifndef PMP_DATABASE_H
#define PMP_DATABASE_H

#include "common/hashid.h"

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QSqlQuery)
QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace PMP {

    class FileData;

    class Database : public QObject {
        Q_OBJECT
    public:
        static bool init(QTextStream& out);

        static Database* instance() { return _instance; }

        void registerHash(const HashID& hash);
        uint getHashID(const HashID& hash);
        QList<QPair<uint,HashID> > getHashes(uint largerThanID = 0);

        void registerFilename(uint hashID, const QString& filenameWithoutPath);
        QList<QString> getFilenames(uint hashID);

    private:
        bool executeScalar(QSqlQuery& q, int& i, const int& defaultValue = 0);
        bool executeScalar(QSqlQuery& q, QString& s, const QString& defaultValue = "");

        static Database* _instance;
    };
}
#endif

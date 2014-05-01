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

#ifndef PMP_RESOLVER_H
#define PMP_RESOLVER_H

#include "common/hashid.h"

#include <QMultiHash>
#include <QObject>
#include <QString>

namespace PMP {

    class FileData;

    class Resolver : public QObject {
        Q_OBJECT
    public:
        Resolver();

        void registerData(const FileData& data);
        void registerFile(const FileData& file, const QString& filename);

        QString findPath(const HashID& hash);
        const FileData* findData(const HashID& hash);

    private:
        QMultiHash<HashID, const FileData*> _tagCache;
        QMultiHash<HashID, QString> _pathCache;

    };
}
#endif

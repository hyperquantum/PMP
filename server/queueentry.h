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

#ifndef PMP_QUEUEENTRY_H
#define PMP_QUEUEENTRY_H

#include <QObject>
#include <QString>

namespace PMP {

    class FileData;
    class HashID;
    class Resolver;

    class QueueEntry : public QObject {
        Q_OBJECT
    public:
        QueueEntry(uint queueID, QString const& filename);
        QueueEntry(uint queueID, FileData const& filedata);
        QueueEntry(uint queueID, HashID const& hash);
        ~QueueEntry();

        uint queueID() const { return _queueID; }

        HashID const* hash() const;

        void setFilename(QString const& filename);
        QString const* filename() const;
        bool checkValidFilename(Resolver& resolver, QString* outFilename = 0);

        void checkTrackData(Resolver& resolver);

        /** Length in seconds. Is negative when unknown. */
        int lengthInSeconds() const;

        QString artist() const;
        QString title() const;



    private:
        uint _queueID;
        QString _filename;
        bool _haveFilename;
        const FileData* _fileData;
        const HashID* _hash;
    };
}
#endif

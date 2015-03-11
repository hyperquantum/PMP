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

#ifndef PMP_QUEUEENTRY_H
#define PMP_QUEUEENTRY_H

#include "common/audiodata.h"
#include "common/hashid.h"
#include "common/tagdata.h"

#include <QObject>

namespace PMP {

    class FileData;
    class Queue;
    class Resolver;

    class QueueEntry : public QObject {
        Q_OBJECT
    public:
        QueueEntry(Queue* parent, QString const& filename);
        QueueEntry(Queue* parent, FileData const& filedata);
        QueueEntry(Queue* parent, HashID const& hash);
        ~QueueEntry();

        uint queueID() const { return _queueID; }

        HashID const* hash() const;
        bool checkHash(Resolver& resolver);

        void setFilename(QString const& filename);
        QString const* filename() const;
        bool checkValidFilename(Resolver& resolver, bool fast, QString* outFilename = 0);

        void checkAudioData(Resolver& resolver);
        void checkTrackData(Resolver& resolver);

        /** Length in seconds. Is negative when unknown. */
        int lengthInSeconds() const;

        QString artist() const;
        QString title() const;

        int& fileFinderBackoff() { return _fileFinderBackoff; }
        int& fileFinderFailedCount() { return _fileFinderFailedCount; }

    private:
        uint const _queueID;
        HashID _hash;
        //bool _fetchedAudioInfo;
        AudioData _audioInfo;
        QString _filename;
        bool _haveFilename;
        bool _fetchedTagData;
        TagData _tagData;
        int _fileFinderBackoff;
        int _fileFinderFailedCount;
    };
}
#endif

/*
    Copyright (C) 2014-2016, Kevin Andre <hyperquantum@gmail.com>

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
#include "common/filehash.h"
#include "common/tagdata.h"

#include <QDateTime>
#include <QObject>

namespace PMP {

    class FileData;
    class Queue;
    class Resolver;

    enum class QueueEntryType {
        Track = 0,
        Break
    };

    class QueueEntry : public QObject {
        Q_OBJECT
    public:
        QueueEntry(Queue* parent, QString const& filename);
        QueueEntry(Queue* parent, FileData const& filedata);
        QueueEntry(Queue* parent, FileHash const& hash);

        static QueueEntry* createBreak(Queue* parent);

        ~QueueEntry();

        uint queueID() const { return _queueID; }
        QueueEntryType type() const { return _type; }
        bool isTrack() const { return _type == QueueEntryType::Track; }

        FileHash const* hash() const;
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

        QDateTime started() const { return _started; }
        QDateTime ended() const { return _ended; }
        void setStartedNow();
        void setEndedNow();

    private:
        QueueEntry(Queue* parent, QueueEntryType type);

        uint const _queueID;
        QueueEntryType _type;
        FileHash _hash;
        //bool _fetchedAudioInfo;
        AudioData _audioInfo;
        QString _filename;
        bool _haveFilename;
        bool _fetchedTagData;
        TagData _tagData;
        int _fileFinderBackoff;
        int _fileFinderFailedCount;
        QDateTime _started;
        QDateTime _ended;
    };
}
#endif

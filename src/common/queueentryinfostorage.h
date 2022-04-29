/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_QUEUEENTRYINFOSTORAGE_H
#define PMP_QUEUEENTRYINFOSTORAGE_H

#include "filehash.h"
#include "queueentrytype.h"
#include "tribool.h"

#include <QObject>

namespace PMP
{
    class QueueEntryInfo : public QObject
    {
        Q_OBJECT
    public:
        QueueEntryInfo(quint32 queueID);

        quint32 queueID() const { return _queueID; }

        TriBool isTrack() const
        {
            if (_type == QueueEntryType::Track)
                return true;

            if (_type == QueueEntryType::Unknown)
                return TriBool::unknown;

            return false;
        }

        QueueEntryType type() const { return _type; }
        const FileHash& hash() const { return _hash; }
        int lengthInMilliseconds() const { return _lengthMilliseconds; }
        QString artist() const { return _artist; }
        QString title() const { return _title; }

        bool needFilename() const;
        QString informativeFilename() const { return _informativeFilename; }

        void setHash(QueueEntryType type, const FileHash& hash);
        void setInfo(QueueEntryType type, qint64 lengthInMilliseconds,
                     QString const& title, QString const& artist);
        bool setPossibleFilenames(QList<QString> const& names);

    private:
        quint32 _queueID;
        QueueEntryType _type;
        FileHash _hash;
        qint64 _lengthMilliseconds;
        QString _title;
        QString _artist;
        QString _informativeFilename;
    };

    class QueueEntryInfoStorage : public QObject
    {
        Q_OBJECT
    public:
        virtual ~QueueEntryInfoStorage() {}

        virtual QueueEntryInfo* entryInfoByQueueId(quint32 queueId) = 0;

        virtual void fetchEntry(quint32 queueId) = 0;
        virtual void fetchEntries(QList<quint32> queueIds) = 0;
        virtual void refetchEntries(QList<quint32> queueIds) = 0;

    public Q_SLOTS:
        virtual void dropInfoFor(quint32 queueId) = 0;

    Q_SIGNALS:
        void tracksChanged(QList<quint32> queueIDs);

    protected:
        explicit QueueEntryInfoStorage(QObject* parent) {}
    };
}
#endif

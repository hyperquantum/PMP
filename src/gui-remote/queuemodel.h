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

#ifndef PMP_QUEUEMODEL_H
#define PMP_QUEUEMODEL_H

#include "common/filehash.h"
#include "common/playermode.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QList>
#include <QSet>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP
{
    class ClientServerInterface;
    class QueueEntryInfo;
    class QueueEntryInfoStorage;
    class QueueMediator;
    class UserDataFetcher;

    class RegularUiRefresher : public QObject
    {
        Q_OBJECT
    public:
        explicit RegularUiRefresher(QObject* parent);

        void setRefresh(uint id, qint64 intervalMilliseconds);
        void stopRefresh(uint id);

    Q_SIGNALS:
        void timeout();

    private Q_SLOTS:
        void timerTimeout(QTimer* timer, quint64 interval);

    private:
        bool isAlreadyRegistered(uint id, qint64 intervalMilliseconds);
        void removeId(uint id);
        void removeAllForInterval(quint64 interval);
        void createTimerIfNotExists(quint64 interval);
        void destroyTimer(QTimer* timer, quint64 interval);

        QHash<qint64, QSet<uint>> _intervalsToIds;
        QHash<uint, qint64> _idsToInterval;
        QSet<qint64> _intervalTimers;
    };

    class QueueTrack
    {
    public:
        QueueTrack() : _id(0)/*, _real()*/ {}

        QueueTrack(quint32 queueId, bool realTrack)
         : _id(queueId), _real(realTrack)
        {
            //
        }

        QueueTrack(quint32 queueId, const FileHash& hash)
         : _id(queueId), _hash(hash), _real(true)
        {
            //
        }

        quint32 queueId() const { return _id; }
        FileHash hash() const { return _hash; }

        bool isNull() const { return _id == 0; }
        bool isRealTrack() const { return _real; }

    private:
        quint32 _id;
        FileHash _hash;
        bool _real;
    };

    class QueueModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        QueueModel(QObject* parent, ClientServerInterface* clientServerInterface,
                   QueueMediator* source, QueueEntryInfoStorage* trackInfoStorage);

        int rowCount(const QModelIndex& parent = QModelIndex()) const;
        int columnCount(const QModelIndex& parent = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex& index) const;
        Qt::DropActions supportedDragActions() const;
        Qt::DropActions supportedDropActions() const;
        QMimeData* mimeData(const QModelIndexList& indexes) const;
        QStringList mimeTypes() const;
        bool dropMimeData(const QMimeData* data, Qt::DropAction action,
                          int row, int column, const QModelIndex& parent);
        bool canDropMimeData(const QMimeData* data, Qt::DropAction action,
                             int row, int column, const QModelIndex& parent) const;

        quint32 trackIdAt(const QModelIndex& index) const;
        QueueTrack trackAt(const QModelIndex& index) const;

    private Q_SLOTS:
        void playerModeChanged(PlayerMode playerMode, quint32 personalModeUserId,
                               QString personalModeUserLogin);
        void userDataReceivedForUser(quint32 userId);

        void queueResetted(int queueLength);
        void entriesReceived(int index, QList<quint32> entries);
        void tracksChanged(QList<quint32> queueIDs);
        void trackAdded(int index, quint32 queueID);
        void trackRemoved(int index, quint32 queueID);
        void trackMoved(int fromIndex, int toIndex, quint32 queueID);
//        void tracksInserted(int firstIndex, int lastIndex);
//        void tracksRemoved(int firstIndex, int lastIndex);
//        void tracksChanged(int firstIndex, int lastIndex);

    private:
        struct Track
        {
            quint32 _queueID;
            QString _title;
            QString _artist;
            int _lengthSeconds;

            Track(quint32 queueID)
             : _queueID(queueID), _lengthSeconds(-1)
            {
                //
            }
        };

        QVariant trackModelData(QueueEntryInfo* info, int col, int role) const;
        bool dropQueueItemMimeData(const QMimeData* data, Qt::DropAction action, int row);
        bool dropFileHashMimeData(const QMimeData* data, int row);

        void markLastHeardColumnAsChanged();
        void markUserDataColumnsAsChanged();

        //Track* trackAt(const QModelIndex& index) const;

        UserDataFetcher* _userDataFetcher;
        QueueMediator* _source;
        QueueEntryInfoStorage* _infoStorage;
        RegularUiRefresher* _lastHeardRefresher;
        qint64 _clientClockTimeOffsetMs;
        PlayerMode _playerMode;
        quint32 _personalModeUserId;
        int _modelRows;
        QList<Track*> _tracks;
    };
}
#endif

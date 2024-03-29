/*
    Copyright (C) 2017-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_PLAYERHISTORYMODEL_H
#define PMP_PLAYERHISTORYMODEL_H

#include "common/playerhistorytrackinfo.h"

#include "client/localhashid.h"

#include <QAbstractTableModel>
#include <QList>
#include <QSharedPointer>
#include <QVector>

namespace PMP::Client
{
    class LocalHashIdRepository;
    class QueueEntryInfoStorage;
    class ServerInterface;
}

namespace PMP
{
    class PlayerHistoryModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        PlayerHistoryModel(QObject* parent, Client::ServerInterface* serverInterface);

        Client::LocalHashId trackHashAt(int rowIndex) const;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const override;
        QVariant data(const QModelIndex& index,
                      int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        Qt::DropActions supportedDragActions() const override;
        Qt::DropActions supportedDropActions() const override;
        QMimeData* mimeData(const QModelIndexList& indexes) const override;

    private Q_SLOTS:
        void onReceivedPlayerHistoryEntry(PMP::PlayerHistoryTrackInfo track);
        void onReceivedPlayerHistory(QVector<PMP::PlayerHistoryTrackInfo> tracks);

        void onTracksChanged(QList<quint32> queueIDs);
        void markStartedEndedColumnsAsChanged();

    private:
        int _historySizeGoal;
        Client::LocalHashIdRepository* _hashIdRepository;
        Client::QueueEntryInfoStorage* _infoStorage;
        qint64 _clientClockTimeOffsetMs;
        QList<QSharedPointer<PlayerHistoryTrackInfo>> _list;
    };
}
#endif

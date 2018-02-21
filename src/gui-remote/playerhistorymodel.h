/*
    Copyright (C) 2017-2018, Kevin Andre <hyperquantum@gmail.com>

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

#include <QAbstractTableModel>
#include <QList>
#include <QSharedPointer>
#include <QVector>

namespace PMP {

    class QueueEntryInfoFetcher;
    class ServerConnection;

    class PlayerHistoryModel : public QAbstractTableModel {
        Q_OBJECT
    public:
        PlayerHistoryModel(QObject* parent, QueueEntryInfoFetcher* trackInfoFetcher);

        void setConnection(ServerConnection* connection);

        int rowCount(const QModelIndex& parent = QModelIndex()) const;
        int columnCount(const QModelIndex& parent = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex& index) const;
        Qt::DropActions supportedDragActions() const;
        Qt::DropActions supportedDropActions() const;
        QMimeData* mimeData(const QModelIndexList& indexes) const;

    private slots:
        void onReceivedPlayerHistoryEntry(PMP::PlayerHistoryTrackInfo track);
        void onReceivedPlayerHistory(QVector<PMP::PlayerHistoryTrackInfo> tracks);

        void onTracksChanged(QList<quint32> queueIDs);

    private:
        int _historySizeGoal;
        QueueEntryInfoFetcher* _infoFetcher;
        QList<QSharedPointer<PlayerHistoryTrackInfo>> _list;
    };
}
#endif

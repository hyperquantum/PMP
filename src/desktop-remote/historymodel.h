/*
    Copyright (C) 2024, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_HISTORYMODEL_H
#define PMP_HISTORYMODEL_H

#include "common/playerhistorytrackinfo.h"

#include "client/historyentry.h"
#include "client/localhashid.h"

#include <QAbstractTableModel>
#include <QVector>

#include <deque>

namespace PMP::Client
{
    class HistoryController;
    class ServerInterface;
}

namespace PMP
{
    class HistoryModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        HistoryModel(QObject* parent, uint userId, Client::LocalHashId hashId,
                     Client::ServerInterface* serverInterface);

        uint userId() const;
        void setUserId(uint userId);

        Client::LocalHashId track() const;
        void setTrack(Client::LocalHashId hashId);

        int countTotal() const { return _countTotal; }
        int countForScore() const { return _countForScore; }

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;

    Q_SIGNALS:
        void countsChanged();

    private:
        void reload();
        void sendInitialRequest();
        void onConnectedChanged();
        void handleNewPlayerHistoryEntry(PMP::PlayerHistoryTrackInfo track);
        void handleHistoryRequestResult(Client::HistoryFragment fragment,
                                        uint stateExpected);
        void addToCounts(Client::HistoryEntry const& entry);
        void addToCounts(QVector<Client::HistoryEntry> const& entries);

        static const int _fragmentSizeLimit = 20;

        Client::ServerInterface* _serverInterface;
        uint _stateAtLastRequest { 0 };
        uint _userId;
        Client::LocalHashId _hashId;
        std::deque<Client::HistoryEntry> _entries;
        int _countTotal { 0 };
        int _countForScore { 0 };
    };
}
#endif

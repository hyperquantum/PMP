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

#include "historymodel.h"

#include "client/historycontroller.h"
#include "client/queueentryinfostorage.h"
#include "client/serverinterface.h"

#include <QtDebug>

#include <algorithm>

using namespace PMP::Client;

namespace PMP
{
    HistoryModel::HistoryModel(QObject* parent, uint userId, LocalHashId hashId,
                               ServerInterface* serverInterface)
     : QAbstractTableModel(parent),
       _serverInterface(serverInterface),
       _userId(userId),
       _hashId(hashId)
    {
        qDebug() << "HistoryModel: created with user ID" << userId
                 << "and hash ID" << hashId;

        connect(_serverInterface, &ServerInterface::connectedChanged,
                this, &HistoryModel::onConnectedChanged);

        auto* historyController = &_serverInterface->historyController();
        connect(historyController, &HistoryController::receivedPlayerHistoryEntry,
                this, &HistoryModel::handleNewPlayerHistoryEntry);

        onConnectedChanged();
    }

    uint PMP::HistoryModel::userId() const
    {
        return _userId;
    }

    void HistoryModel::setUserId(uint userId)
    {
        if (_userId == userId)
            return;

        qDebug() << "HistoryModel: user ID changing from" << _userId << "to" << userId;

        _stateAtLastRequest++;
        _userId = userId;

        reload();
    }

    LocalHashId HistoryModel::track() const
    {
        return _hashId;
    }

    void HistoryModel::setTrack(Client::LocalHashId hashId)
    {
        if (_hashId == hashId)
            return;

        qDebug() << "HistoryModel: track ID changing from" << _hashId << "to" << hashId;

        _stateAtLastRequest++;
        _hashId = hashId;

        reload();
    }

    int HistoryModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent)

        return static_cast<int>(_entries.size());
    }

    int HistoryModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent)

        return 3;
    }

    QVariant HistoryModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case 0: return QString(tr("Started"));
            case 1: return QString(tr("Ended"));
            case 2: return QString(tr("Affects score"));
            }
        }

        return {};
    }

    QVariant HistoryModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= _entries.size())
            return {};

        auto& entry = _entries[index.row()];

        if (role == Qt::DisplayRole)
        {
            int col = index.column();

            switch (col)
            {
                case 0:
                {
                    auto started = entry.started()/*.addMSecs(_clientClockTimeOffsetMs)*/;
                    return started.toLocalTime();
                }
                case 1:
                {
                    auto ended = entry.ended()/*.addMSecs(_clientClockTimeOffsetMs)*/;
                    return ended.toLocalTime();
                }
                case 2:
                    return entry.validForScoring()
                               ? QString(tr("Yes"))
                               : QString(tr("No"));
            }
        }

        return {};
    }

    Qt::ItemFlags HistoryModel::flags(const QModelIndex& index) const
    {
        Q_UNUSED(index);

        Qt::ItemFlags f(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return f;
    }

    void HistoryModel::reload()
    {
        if (!_entries.empty())
        {
            beginRemoveRows({}, 0, static_cast<int>(_entries.size() - 1));
            _entries.clear();
            endRemoveRows();

            _countTotal = 0;
            _countForScore = 0;
            Q_EMIT countsChanged();
        }

        if (_serverInterface->connected())
            sendInitialRequest();
    }

    void HistoryModel::sendInitialRequest()
    {
        auto& historyController = _serverInterface->historyController();

        auto state = _stateAtLastRequest;
        auto future =
            historyController.getPersonalTrackHistory(_hashId, _userId,
                                                      _fragmentSizeLimit);

        future.addResultListener(
            this,
            [this, state](HistoryFragment fragment)
            {
                handleHistoryRequestResult(fragment, state);
            }
        );
        //future.addFailureListener(this, [this](AnyResultMessageCode code) {});
    }

    void HistoryModel::onConnectedChanged()
    {
        bool isConnected = _serverInterface->connected();

        if (isConnected)
        {
            reload();
        }
        else
        {
            //
        }
    }

    void HistoryModel::handleNewPlayerHistoryEntry(PlayerHistoryTrackInfo track)
    {
        if (!_entries.empty() && _entries[0].ended() > track.started())
            return;

        if (track.user() != _userId)
            return;

        auto* queueEntryInfo =
            _serverInterface->queueEntryInfoStorage().entryInfoByQueueId(track.queueID());

        if (queueEntryInfo->hashId() != _hashId)
            return;

        HistoryEntry newEntry(_hashId, _userId, track.started(), track.ended(),
                              track.permillage(), track.validForScoring());

        beginInsertRows({}, 0, 0);
        _entries.push_front(newEntry);
        endInsertRows();

        addToCounts(newEntry);
    }

    void HistoryModel::handleHistoryRequestResult(Client::HistoryFragment fragment,
                                                  uint stateExpected)
    {
        if (stateExpected != _stateAtLastRequest)
        {
            return;
        }

        auto entries = fragment.entries();
        if (entries.isEmpty())
        {
            return; /* we got everything */
        }

        /* make sure we have the entries ordered descending, so most recent first */
        if (entries.size() >= 2 && entries.first().started() < entries.last().started())
        {
            std::reverse(entries.begin(), entries.end());
        }

        auto existingRowsCount = static_cast<int>(_entries.size());

        beginInsertRows({},
                        existingRowsCount,
                        existingRowsCount + static_cast<int>(entries.size() - 1));
        _entries.insert(_entries.end(), entries.begin(), entries.end());
        endInsertRows();

        addToCounts(entries);

        auto& historyController = _serverInterface->historyController();

        auto state = _stateAtLastRequest;
        auto future =
            historyController.getPersonalTrackHistory(_hashId, _userId,
                                                      _fragmentSizeLimit,
                                                      fragment.nextStartId());

        future.addResultListener(
            this,
            [this, state](HistoryFragment fragment)
            {
                handleHistoryRequestResult(fragment, state);
            }
        );
        //future.addFailureListener(this, [this](AnyResultMessageCode code) {});
    }

    void HistoryModel::addToCounts(const Client::HistoryEntry& entry)
    {
        _countTotal++;

        if (entry.validForScoring())
            _countForScore++;

        Q_EMIT countsChanged();
    }

    void HistoryModel::addToCounts(const QVector<Client::HistoryEntry>& entries)
    {
        auto extraCountTotal = entries.size();
        auto extraCountForScore =
            std::count_if(
                entries.begin(), entries.end(),
                [](HistoryEntry const& entry) { return entry.validForScoring(); }
            );

        _countTotal += extraCountTotal;
        _countForScore += extraCountForScore;
        Q_EMIT countsChanged();
    }
}

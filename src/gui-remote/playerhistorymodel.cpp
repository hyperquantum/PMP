/*
    Copyright (C) 2017-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "playerhistorymodel.h"

#include "common/clientserverinterface.h"
#include "common/historycontroller.h"
#include "common/queueentryinfofetcher.h"
#include "common/util.h"

#include <QBrush>
#include <QBuffer>
#include <QDataStream>
#include <QMimeData>
#include <QVector>

namespace PMP
{
    PlayerHistoryModel::PlayerHistoryModel(QObject* parent,
                                           ClientServerInterface* clientServerInterface)
     : QAbstractTableModel(parent),
       _historySizeGoal(20),
       _infoFetcher(&clientServerInterface->queueEntryInfoFetcher())
    {
        connect(
            _infoFetcher, &QueueEntryInfoFetcher::tracksChanged,
            this, &PlayerHistoryModel::onTracksChanged
        );

        auto* historyController = &clientServerInterface->historyController();
        connect(
            historyController, &HistoryController::receivedPlayerHistoryEntry,
            this, &PlayerHistoryModel::onReceivedPlayerHistoryEntry
        );
        connect(
            historyController, &HistoryController::receivedPlayerHistory,
            this, &PlayerHistoryModel::onReceivedPlayerHistory
        );

        connect(
            clientServerInterface, &ClientServerInterface::connectedChanged,
            this,
            [this, clientServerInterface, historyController]()
            {
                if (clientServerInterface->connected())
                {
                    historyController->sendPlayerHistoryRequest(_historySizeGoal);
                }
                else
                {
                    beginRemoveRows({}, 0, _list.size() - 1);
                    _list.clear();
                    endRemoveRows();
                }
            }
        );

        if (clientServerInterface->connected())
            historyController->sendPlayerHistoryRequest(_historySizeGoal);
    }

    void PlayerHistoryModel::onReceivedPlayerHistoryEntry(PlayerHistoryTrackInfo track)
    {
        int index = _list.size();

        beginInsertRows({}, index, index);
        auto entry = QSharedPointer<PlayerHistoryTrackInfo>::create(track);
        _list.append(entry);
        endInsertRows();

        /* trim the history list if it gets too big */
        while (_list.size() > _historySizeGoal && _historySizeGoal >= 0)
        {
            auto oldestEntry = _list.first();

            beginRemoveRows({}, 0, 0);
            _list.removeFirst();
            endRemoveRows();

            _infoFetcher->dropInfoFor(oldestEntry->queueID());
        }
    }

    void PlayerHistoryModel::onReceivedPlayerHistory(
                                                   QVector<PlayerHistoryTrackInfo> tracks)
    {
        if (_list.size() > 0)
        {
            beginRemoveRows({}, 0, _list.size() - 1);
            _list.clear();
            endRemoveRows();
        }

        /* if we received more than we want, discard the rest */
        if (tracks.size() > _historySizeGoal && _historySizeGoal >= 0)
        {
            tracks = tracks.mid(tracks.size() - _historySizeGoal, _historySizeGoal);
        }

        if (tracks.empty())
            return;

        beginInsertRows({}, 0, tracks.size() - 1);
        for (auto& track : qAsConst(tracks))
        {
            _list.append(QSharedPointer<PlayerHistoryTrackInfo>::create(track));
        }
        endInsertRows();
    }

    void PlayerHistoryModel::onTracksChanged(QList<quint32> queueIDs)
    {
        Q_UNUSED(queueIDs);

        /* we don't know the indexes, so we say everything changed */
        Q_EMIT dataChanged(createIndex(0, 0), createIndex(_list.size() - 1, 2));
    }

    int PlayerHistoryModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);

        return _list.size();
    }

    int PlayerHistoryModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);

        /* Title, Artist, Length, Started, Ended */
        return 5;
    }

    QVariant PlayerHistoryModel::headerData(int section, Qt::Orientation orientation,
                                            int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
                case 0: return QString(tr("Title"));
                case 1: return QString(tr("Artist"));
                case 2: return QString(tr("Length"));
                case 3: return QString(tr("Started"));
                case 4: return QString(tr("Ended"));
            }
        }

        return {};
    }

    QVariant PlayerHistoryModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= _list.size())
            return {};

        auto item = _list[index.row()];

        if (role == Qt::ForegroundRole)
            return item->hadError() ? QBrush(Qt::red) : QVariant();

        if (role == Qt::BackgroundRole)
            return item->hadError() ? QBrush(Qt::white) : QVariant();

        if (role == Qt::DisplayRole)
        {
            int col = index.column();

            auto queueID = _list[index.row()]->queueID();
            auto info = _infoFetcher->entryInfoByQID(queueID);

            switch (col)
            {
                case 0:
                {
                    if (!info) return {};

                    QString title = info->title();
                    return (title != "") ? title : info->informativeFilename();
                }
                case 1:
                {
                    if (!info) return {};

                    return info->artist();
                }
                case 2:
                {
                    if (!info) return {};

                    int lengthInMilliseconds = info->lengthInMilliseconds();
                    if (lengthInMilliseconds < 0) { return "?"; }

                    return Util::millisecondsToShortDisplayTimeText(lengthInMilliseconds);
                }
                case 3: return _list[index.row()]->started().toLocalTime();
                case 4: return _list[index.row()]->ended().toLocalTime();
            }
        }

        return {};
    }

    Qt::ItemFlags PlayerHistoryModel::flags(const QModelIndex& index) const
    {
        Q_UNUSED(index);

        Qt::ItemFlags f(Qt::ItemIsSelectable | Qt::ItemIsEnabled
                        | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        return f;
    }

    Qt::DropActions PlayerHistoryModel::supportedDragActions() const
    {
        return Qt::CopyAction;
    }

    Qt::DropActions PlayerHistoryModel::supportedDropActions() const
    {
        return Qt::CopyAction;
    }

    FileHash PlayerHistoryModel::trackHashAt(int rowIndex) const
    {
        if (rowIndex < 0 || rowIndex >= _list.size()) return {};

        auto queueID = _list[rowIndex]->queueID();
        auto info = _infoFetcher->entryInfoByQID(queueID);
        if (!info) return {};

        return info->hash();
    }

    QMimeData* PlayerHistoryModel::mimeData(const QModelIndexList& indexes) const
    {
        qDebug() << "mimeData called; indexes count =" << indexes.size();

        if (indexes.isEmpty()) return nullptr;

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QDataStream stream(&buffer);
        stream.setVersion(QDataStream::Qt_5_2);

        QVector<FileHash> hashes;
        int prevRow = -1;
        for (auto& index : indexes)
        {
            int row = index.row();
            if (row == prevRow) continue;
            prevRow = row;

            auto queueID = _list[row]->queueID();
            auto info = _infoFetcher->entryInfoByQID(queueID);
            if (!info)
            {
                qDebug() << " ignoring track without info";
                continue;
            }

            auto& hash = info->hash();
            if (hash.isNull())
            {
                qDebug() << " ignoring empty hash";
                continue;
            }

            qDebug() << " row" << row << "; col" << index.column()
                     << "; hash" << hash.dumpToString();
            hashes.append(hash);
        }

        if (hashes.empty()) return nullptr;

        stream << (quint32)hashes.size();
        for (int i = 0; i < hashes.size(); ++i)
        {
            stream << (quint64)hashes[i].length();
            stream << hashes[i].SHA1();
            stream << hashes[i].MD5();
        }

        buffer.close();

        QMimeData* data = new QMimeData();

        data->setData("application/x-pmp-filehash", buffer.data());
        return data;
    }
}

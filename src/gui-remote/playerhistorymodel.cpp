/*
    Copyright (C) 2017, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/serverconnection.h"

#include "queueentryinfofetcher.h"

namespace PMP {

    PlayerHistoryModel::PlayerHistoryModel(QObject* parent,
                                           QueueEntryInfoFetcher* trackInfoFetcher)
     : QAbstractTableModel(parent), _historySizeGoal(15), _infoFetcher(trackInfoFetcher)
    {
        connect(
            _infoFetcher, &QueueEntryInfoFetcher::tracksChanged,
            this, &PlayerHistoryModel::onTracksChanged
        );
    }

    void PlayerHistoryModel::setConnection(ServerConnection* connection) {
        connect(
            connection, &ServerConnection::receivedPlayerHistoryEntry,
            this, &PlayerHistoryModel::onReceivedPlayerHistoryEntry
        );
        connect(
            connection, &ServerConnection::receivedPlayerHistory,
            this, &PlayerHistoryModel::onReceivedPlayerHistory
        );

        connection->sendPlayerHistoryRequest(_historySizeGoal);
    }

    void PlayerHistoryModel::onReceivedPlayerHistoryEntry(PlayerHistoryTrackInfo track) {
        int index = _list.size();

        beginInsertRows(QModelIndex(), index, index);
        auto entry = QSharedPointer<PlayerHistoryTrackInfo>::create(track);
        _list.append(entry);
        endInsertRows();

        /* trim the history list if it gets too big */
        while (_list.size() > _historySizeGoal && _historySizeGoal >= 0) {
            beginRemoveRows(QModelIndex(), 0, 0);
            _list.removeFirst();
            endRemoveRows();
        }
    }

    void PlayerHistoryModel::onReceivedPlayerHistory(
                                                   QVector<PlayerHistoryTrackInfo> tracks)
    {
        if (_list.size() > 0) {
            beginRemoveRows(QModelIndex(), 0, _list.size() - 1);
            _list.clear();
            endRemoveRows();
        }

        /* if we received more than we want, discard the rest */
        if (tracks.size() > _historySizeGoal && _historySizeGoal >= 0) {
            tracks = tracks.mid(tracks.size() - _historySizeGoal, _historySizeGoal);
        }

        if (tracks.empty())
            return;

        beginInsertRows(QModelIndex(), 0, tracks.size() - 1);
        Q_FOREACH(auto const& track, tracks) {
            _list.append(QSharedPointer<PlayerHistoryTrackInfo>::create(track));
        }
        endInsertRows();
    }

    void PlayerHistoryModel::onTracksChanged(QList<quint32> queueIDs) {
        /* we don't know the indexes, so we say everything changed */
        emit dataChanged(createIndex(0, 0), createIndex(_list.size() - 1, 2));
    }

    int PlayerHistoryModel::rowCount(const QModelIndex& parent) const {
        return _list.size();
    }

    int PlayerHistoryModel::columnCount(const QModelIndex& parent) const {
        /* Title, Artist, Length, Started, Ended */
        return 5;
    }

    QVariant PlayerHistoryModel::headerData(int section, Qt::Orientation orientation,
                                            int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            switch (section) {
                case 0: return QString(tr("Title"));
                case 1: return QString(tr("Artist"));
                case 2: return QString(tr("Length"));
                case 3: return QString(tr("Started"));
                case 4: return QString(tr("Ended"));
            }
        }

        return QVariant();
    }

    QVariant PlayerHistoryModel::data(const QModelIndex& index, int role) const {
        if (role == Qt::DisplayRole && index.row() < _list.size()) {
            int col = index.column();

            auto queueID = _list[index.row()]->queueID();
            auto info = _infoFetcher->entryInfoByQID(queueID);

            switch (col) {
                case 0:
                {
                    if (!info) return QVariant();

                    QString title = info->title();
                    return (title != "") ? title : info->informativeFilename();
                }
                case 1:
                {
                    if (!info) return QVariant();
                    return info->artist();
                }
                    return QString(tr("<Artist>"));
                case 2:
                {
                    if (!info) return QVariant();

                    int lengthInSeconds = info->lengthInSeconds();
                    if (lengthInSeconds < 0) { return "?"; }

                    int sec = lengthInSeconds % 60;
                    int min = (lengthInSeconds / 60) % 60;
                    int hrs = lengthInSeconds / 3600;

                    return QString::number(hrs).rightJustified(2, '0')
                        + ":" + QString::number(min).rightJustified(2, '0')
                        + ":" + QString::number(sec).rightJustified(2, '0');
                }
                case 3: return _list[index.row()]->started().toLocalTime();
                case 4: return _list[index.row()]->ended().toLocalTime();
            }
        }

        return QVariant();
    }

}

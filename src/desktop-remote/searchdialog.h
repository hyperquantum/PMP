/*
    Copyright (C) 2025, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_SEARCHDIALOG_H
#define PMP_SEARCHDIALOG_H

#include "common/playerstate.h"

#include "client/collectiontrackinfo.h"
#include "client/localhashid.h"

#include <QAbstractTableModel>
#include <QDialog>
#include <QHash>
#include <QList>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Ui
{
    class SearchDialog;
}

namespace PMP::Client
{
    class CollectionWatcher;
    class LocalHashIdRepository;
    class QueueHashesMonitor;
    class ServerInterface;
}

namespace PMP
{
    class SearchData;
    class UserForStatisticsDisplay;

    class SearchResultsTableModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        SearchResultsTableModel(QObject* parent,
                                Client::ServerInterface* serverInterface,
                                Client::QueueHashesMonitor* queueHashesMonitor);

        void setTracksList(QList<Client::LocalHashId> tracks);

        Client::LocalHashId trackAt(const QModelIndex& index) const;

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
        void onPlayerStateChanged(PlayerState playerState);
        void onCurrentTrackInfoChanged();
        void onTrackAvailabilityChanged(Client::LocalHashId hashId, bool isAvailable);
        void onTrackDataChanged(Client::CollectionTrackInfo track);
        void onHashInQueuePresenceChanged(Client::LocalHashId hashId);

    private:
        int getTrackIndex(Client::LocalHashId trackId) const;
        QVariant trackData(Client::LocalHashId trackId, int column, int role) const;
        void markLeftColumnAsChanged(int index);
        void markRowAsChanged(int index);

        Client::ServerInterface* _serverInterface;
        Client::LocalHashIdRepository* _hashIdRepository;
        Client::CollectionWatcher* _collectionWatcher;
        Client::QueueHashesMonitor* _queueHashesMonitor;
        Client::LocalHashId _nowPlayingTrackHash;
        PlayerState _playerState { PlayerState::Unknown };
        QList<Client::LocalHashId> _tracks;
        QHash<Client::LocalHashId, int> _hashIdToIndex;
    };

    class SearchDialog : public QDialog
    {
        Q_OBJECT

    public:
        SearchDialog(QWidget* parent, SearchData* searchData,
                     Client::ServerInterface* serverInterface,
                     Client::QueueHashesMonitor* queueHashesMonitor,
                     UserForStatisticsDisplay* userForStatisticsDisplay);
        ~SearchDialog();

    private Q_SLOTS:
        void resultsContextMenuRequested(const QPoint& position);

    private:
        void onTextEdited();
        void onEditingFinished();
        void onTypingTimerTimeout();
        void setSearchText(QString text);

        Ui::SearchDialog* _ui;
        QTimer* _typingTimer;
        SearchData* _searchData;
        Client::ServerInterface* _serverInterface;
        UserForStatisticsDisplay* _userStatisticsDisplay;
        Client::CollectionWatcher* _collectionWatcher;
        SearchResultsTableModel* _searchResultsModel;
        QMenu* _resultsContextMenu { nullptr };
        QString _searchText;
    };
}
#endif

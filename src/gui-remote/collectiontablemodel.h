/*
    Copyright (C) 2016-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COLLECTIONTABLEMODEL_H
#define PMP_COLLECTIONTABLEMODEL_H

#include "common/playerstate.h"

#include "client/collectiontrackinfo.h"

#include "trackjudge.h"

#include <QAbstractTableModel>
#include <QCollator>
#include <QHash>
#include <QList>
#include <QSortFilterProxyModel>
#include <QVector>

namespace PMP::Client
{
    class LocalHashIdRepository;
    class QueueHashesMonitor;
    class ServerInterface;
    class UserDataFetcher;
}

namespace PMP
{
    class UserForStatisticsDisplay;

    class SortedCollectionTableModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        SortedCollectionTableModel(QObject* parent,
                                   Client::ServerInterface* serverInterface,
                                   Client::QueueHashesMonitor* queueHashesMonitor,
                                   UserForStatisticsDisplay* userForStatisticsDisplay);

        void setHighlightCriterium(TrackCriterium criterium);
        int highlightColorIndex() const;

        void sortByTitle();
        void sortByArtist();
        virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
        int sortColumn() const;
        Qt::SortOrder sortOrder() const;

        Client::CollectionTrackInfo const* trackAt(const QModelIndex& index) const;
        Client::CollectionTrackInfo const* trackAt(int rowIndex) const;

        int rowCount(const QModelIndex& parent = QModelIndex()) const;
        int columnCount(const QModelIndex& parent = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex& index) const;
        Qt::DropActions supportedDragActions() const;
        Qt::DropActions supportedDropActions() const;
        QMimeData* mimeData(const QModelIndexList& indexes) const;

    public Q_SLOTS:
        void setHighlightColorIndex(int colorIndex);

    private Q_SLOTS:
        void onNewTrackReceived(Client::CollectionTrackInfo track);
        void onTrackAvailabilityChanged(Client::LocalHashId hashId, bool isAvailable);
        void onTrackDataChanged(Client::CollectionTrackInfo track);
        void onUserTrackDataChanged(quint32 userId, Client::LocalHashId hashId);
        void currentTrackInfoChanged(Client::LocalHashId hashId);
        void onHashInQueuePresenceChanged(Client::LocalHashId hashId);

    private:
        void updateTrackAvailability(Client::LocalHashId hashId, bool isAvailable);
        template<class T> void addWhenModelEmpty(T trackCollection);
        void addOrUpdateTrack(Client::CollectionTrackInfo const& track);
        void addTrack(Client::CollectionTrackInfo const& track);
        void updateTrack(int innerIndex, Client::CollectionTrackInfo const& newTrackData);
        void buildIndexMaps();
        void rebuildInnerMap(int outerStartIndex = 0);
        void rebuildInnerMap(int outerStartIndex, int outerEndIndex);
        int findOuterIndexMapIndexForInsert(Client::CollectionTrackInfo const& track);
        int findOuterIndexMapIndexForInsert(Client::CollectionTrackInfo const& track,
                                            int searchRangeBegin, int searchRangeEnd);
        int findOuterIndexForHash(Client::LocalHashId hashId);
        void markLeftColumnAsChanged();
        void markEverythingAsChanged();

        bool lessThan(int index1, int index2) const;
        bool lessThan(Client::CollectionTrackInfo const& track1,
                      Client::CollectionTrackInfo const& track2) const;

        int compareStrings(const QString& s1, const QString& s2,
                           Qt::SortOrder sortOrder) const;

        int compareTracks(const Client::CollectionTrackInfo& track1,
                          const Client::CollectionTrackInfo& track2) const;

        int compareTitles(const Client::CollectionTrackInfo& track1,
                          const Client::CollectionTrackInfo& track2,
                          Qt::SortOrder sortOrder) const;

        int compareArtists(const Client::CollectionTrackInfo& track1,
                           const Client::CollectionTrackInfo& track2,
                           Qt::SortOrder sortOrder) const;

        int compareLengths(const Client::CollectionTrackInfo& track1,
                           const Client::CollectionTrackInfo& track2,
                           Qt::SortOrder sortOrder) const;

        int compareAlbums(const Client::CollectionTrackInfo& track1,
                          const Client::CollectionTrackInfo& track2,
                          Qt::SortOrder sortOrder) const;

        Client::LocalHashIdRepository* _hashIdRepository;
        QVector<Client::CollectionTrackInfo*> _tracks;
        QHash<Client::LocalHashId, int> _hashesToInnerIndexes;
        QVector<int> _innerToOuterIndexMap;
        QVector<int> _outerToInnerIndexMap;
        QCollator _collator;
        int _highlightColorIndex;
        int _sortBy;
        Qt::SortOrder _sortOrder;
        TrackJudge _highlightingTrackJudge;
        Client::LocalHashId _currentTrackHash;
        PlayerState _playerState;
        Client::QueueHashesMonitor* _queueHashesMonitor;
    };

    class FilteredCollectionTableModel : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        FilteredCollectionTableModel(QObject* parent,
                                     SortedCollectionTableModel* source,
                                     Client::ServerInterface* serverInterface,
                                     Client::QueueHashesMonitor* queueHashesMonitor,
                                     UserForStatisticsDisplay* userForStatisticsDisplay);

        void setTrackFilters(TrackCriterium criterium1, TrackCriterium criterium2,
                             TrackCriterium criterium3);

        virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

        Client::CollectionTrackInfo const* trackAt(const QModelIndex& index) const;

    public Q_SLOTS:
        void setSearchText(QString search);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    private:
        SortedCollectionTableModel* _source;
        QStringList _searchParts;
        TrackJudge _filteringTrackJudge;
    };
}

Q_DECLARE_METATYPE(PMP::TrackCriterium)

#endif

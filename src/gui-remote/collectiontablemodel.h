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

#include "common/collectiontrackinfo.h"
#include "common/playerstate.h"
#include "common/tribool.h"

#include <QAbstractTableModel>
#include <QCollator>
#include <QHash>
#include <QList>
#include <QSortFilterProxyModel>
#include <QVector>

#include <functional>

namespace PMP::Client
{
    class QueueHashesMonitor;
    class ServerInterface;
    class UserDataFetcher;
}

namespace PMP
{
    enum class TrackCriterium
    {
        None = 0,
        NeverHeard,
        LastHeardNotInLast1000Days,
        LastHeardNotInLast365Days,
        LastHeardNotInLast180Days,
        LastHeardNotInLast90Days,
        LastHeardNotInLast30Days,
        LastHeardNotInLast10Days,
        WithoutScore,
        ScoreMaximum30,
        ScoreAtLeast85,
        ScoreAtLeast90,
        ScoreAtLeast95,
        LengthMaximumOneMinute,
        LengthAtLeastFiveMinutes,
    };

    class TrackJudge
    {
    public:
        TrackJudge(Client::UserDataFetcher& userDataFetcher)
         : _criterium(TrackCriterium::None),
           _userId(0),
           _haveUserId(false),
           _userDataFetcher(userDataFetcher)
        {
            //
        }

        void setUserId(quint32 userId);
        bool isUserIdSetTo(quint32 userId) const
        {
            return _userId == userId && _haveUserId;
        }

        void setCriterium(TrackCriterium mode) { _criterium = mode; }
        TrackCriterium getCriterium() const { return _criterium; }

        TriBool trackSatisfiesCriterium(CollectionTrackInfo const& track,
                                        bool resultForNone) const;

    private:
        TriBool trackSatisfiesScoreCriterium(CollectionTrackInfo const& track,
                              std::function<TriBool(int)> scorePermillageEvaluator) const;

        TriBool trackSatisfiesLastHeardDateCriterium(CollectionTrackInfo const& track,
                                   std::function<TriBool(QDateTime)> dateEvaluator) const;

        TriBool trackSatisfiesNotHeardInTheLastXDaysCriterium(
                                                         CollectionTrackInfo const& track,
                                                         int days) const;

        TrackCriterium _criterium;
        quint32 _userId;
        bool _haveUserId;
        Client::UserDataFetcher& _userDataFetcher;
    };

    class CollectionViewContext : public QObject
    {
        Q_OBJECT
    public:
        CollectionViewContext(QObject* parent, Client::ServerInterface* serverInterface);

        quint32 userId() const { return _userId; }

    Q_SIGNALS:
        void userIdChanged();

    private:
        quint32 _userId;
    };

    class SortedCollectionTableModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        SortedCollectionTableModel(QObject* parent,
                                   Client::ServerInterface* serverInterface,
                                   CollectionViewContext* collectionViewContext);

        void setHighlightCriterium(TrackCriterium criterium);
        int highlightColorIndex() const;

        void sortByTitle();
        void sortByArtist();
        virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
        int sortColumn() const;
        Qt::SortOrder sortOrder() const;

        CollectionTrackInfo* trackAt(const QModelIndex& index) const;
        CollectionTrackInfo* trackAt(int rowIndex) const;

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
        void onNewTrackReceived(CollectionTrackInfo track);
        void onTrackAvailabilityChanged(FileHash hash, bool isAvailable);
        void onTrackDataChanged(CollectionTrackInfo track);
        void onUserTrackDataChanged(quint32 userId, FileHash hash);
        void currentTrackInfoChanged(FileHash hash);
        void onHashInQueuePresenceChanged(FileHash hash);

    private:
        void updateTrackAvailability(FileHash hash, bool isAvailable);
        template<class T> void addWhenModelEmpty(T trackCollection);
        void addOrUpdateTrack(CollectionTrackInfo const& track);
        void addTrack(CollectionTrackInfo const& track);
        void updateTrack(int innerIndex, CollectionTrackInfo const& newTrackData);
        void buildIndexMaps();
        void rebuildInnerMap(int outerStartIndex = 0);
        void rebuildInnerMap(int outerStartIndex, int outerEndIndex);
        int findOuterIndexMapIndexForInsert(CollectionTrackInfo const& track);
        int findOuterIndexMapIndexForInsert(CollectionTrackInfo const& track,
                                            int searchRangeBegin, int searchRangeEnd);
        int findOuterIndexForHash(FileHash const& hash);
        void markLeftColumnAsChanged();
        void markEverythingAsChanged();

        static bool usesUserData(TrackCriterium mode);

        bool lessThan(int index1, int index2) const;
        bool lessThan(CollectionTrackInfo const& track1,
                      CollectionTrackInfo const& track2) const;

        int compareStrings(const QString& s1, const QString& s2,
                           Qt::SortOrder sortOrder) const;

        int compareTracks(const CollectionTrackInfo& track1,
                          const CollectionTrackInfo& track2) const;

        int compareTitles(const CollectionTrackInfo& track1,
                          const CollectionTrackInfo& track2,
                          Qt::SortOrder sortOrder) const;

        int compareArtists(const CollectionTrackInfo& track1,
                           const CollectionTrackInfo& track2,
                           Qt::SortOrder sortOrder) const;

        int compareLengths(const CollectionTrackInfo& track1,
                           const CollectionTrackInfo& track2,
                           Qt::SortOrder sortOrder) const;

        int compareAlbums(const CollectionTrackInfo& track1,
                          const CollectionTrackInfo& track2,
                          Qt::SortOrder sortOrder) const;

        QVector<CollectionTrackInfo*> _tracks;
        QHash<FileHash, int> _hashesToInnerIndexes;
        QVector<int> _innerToOuterIndexMap;
        QVector<int> _outerToInnerIndexMap;
        QCollator _collator;
        int _highlightColorIndex;
        int _sortBy;
        Qt::SortOrder _sortOrder;
        TrackJudge _highlightingTrackJudge;
        FileHash _currentTrackHash;
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
                                     CollectionViewContext* collectionViewContext);

        void setTrackFilter(TrackCriterium criterium);

        virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

        CollectionTrackInfo* trackAt(const QModelIndex& index) const;

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

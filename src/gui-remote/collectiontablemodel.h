/*
    Copyright (C) 2016-2020, Kevin Andre <hyperquantum@gmail.com>

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
#include "common/playermode.h"
#include "common/tribool.h"

#include <QAbstractTableModel>
#include <QCollator>
#include <QHash>
#include <QList>
#include <QSortFilterProxyModel>
#include <QVector>

#include <functional>

namespace PMP {

    enum class TrackHighlightMode {
        None = 0,
        NeverHeard,
        LastHeardNotInLast365Days,
        LastHeardNotInLast180Days,
        LastHeardNotInLast90Days,
        LastHeardNotInLast30Days,
        LastHeardNotInLast10Days,
        WithoutScore,
        ScoreAtLeast85,
        ScoreAtLeast90,
        ScoreAtLeast95,
        LengthMaximumOneMinute,
        LengthAtLeastFiveMinutes,
    };

    class UserDataFetcher;

    class TrackHighlighter {
    public:
        TrackHighlighter(UserDataFetcher& userDataFetcher)
         : _mode(TrackHighlightMode::None), _userId(0), _haveUserId(false),
           _userDataFetcher(userDataFetcher)
        {
            //
        }

        void setUserId(quint32 userId);

        bool isUserIdSetTo(quint32 userId) {
            return _userId == userId && _haveUserId;
        }

        void setMode(TrackHighlightMode mode) { _mode = mode; }
        TrackHighlightMode getMode() { return _mode; }

        TriBool shouldHighlightTrack(CollectionTrackInfo const& track) const;

    private:
        TriBool shouldHighlightBasedOnScore(CollectionTrackInfo const& track,
                              std::function<TriBool(int)> scorePermillageEvaluator) const;

        TriBool shouldHighlightBasedOnHeardDate(CollectionTrackInfo const& track,
                                   std::function<TriBool(QDateTime)> dateEvaluator) const;

        TriBool shouldHighlightBasedOnNotHeardInTheLastXDays(
                                                         CollectionTrackInfo const& track,
                                                         int days) const;

        TrackHighlightMode _mode;
        quint32 _userId;
        bool _haveUserId;
        UserDataFetcher& _userDataFetcher;
    };

    class ClientServerInterface;

    class SortedCollectionTableModel : public QAbstractTableModel {
        Q_OBJECT
    public:
        SortedCollectionTableModel(QObject* parent,
                                   ClientServerInterface* clientServerInterface);

        void setHighlightMode(TrackHighlightMode mode);
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
        void onDataReceivedForUser(quint32 userId);
        void onPlayerModeChanged(PMP::PlayerMode playerMode, quint32 personalModeUserId,
                                 QString personalModeUserLogin);

    private:
        void updateTrackAvailability(FileHash hash, bool isAvailable);
        template<class T> void addWhenModelEmpty(T trackCollection);
        void addOrUpdateTrack(CollectionTrackInfo const& track);
        void addTrack(CollectionTrackInfo const& track);
        void updateTrack(CollectionTrackInfo const& track);
        void buildIndexMaps();
        void rebuildInnerMap(int outerStartIndex = 0);
        int findOuterIndexMapIndexForInsert(CollectionTrackInfo const& track);
        int findOuterIndexMapIndexForInsert(CollectionTrackInfo const& track,
                                            int searchRangeBegin, int searchRangeEnd);
        void markEverythingAsChanged();

        static bool usesUserData(TrackHighlightMode mode);

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
        TrackHighlighter _highlighter;
    };

    class FilteredCollectionTableModel : public QSortFilterProxyModel {
        Q_OBJECT
    public:
        FilteredCollectionTableModel(SortedCollectionTableModel* source,
                                     QObject* parent = 0);

        virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

        CollectionTrackInfo* trackAt(const QModelIndex& index) const;

    public slots:
        void setSearchText(QString search);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    private:
        SortedCollectionTableModel* _source;
        QStringList _searchParts;
    };
}

Q_DECLARE_METATYPE(PMP::TrackHighlightMode)

#endif

/*
    Copyright (C) 2016-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "collectiontablemodel.h"

#include "common/filehash.h"
#include "common/util.h"

#include "client/collectionwatcher.h"
#include "client/currenttrackmonitor.h"
#include "client/localhashidrepository.h"
#include "client/playercontroller.h"
#include "client/queuehashesmonitor.h"
#include "client/serverinterface.h"
#include "client/userdatafetcher.h"

#include "colors.h"
#include "userforstatisticsdisplay.h"

#include <QBrush>
#include <QBuffer>
#include <QDataStream>
#include <QIcon>
#include <QMimeData>
#include <QtDebug>

#include <algorithm>
#include <functional>

using namespace PMP::Client;

namespace PMP
{
    namespace
    {

    class Comparisons
    {
    public:

        template <typename T>
        static int compare(T const& first, T const& second)
        {
            if (first < second) return -1;
            if (second < first) return 1;
            return 0;
        }

        template <typename T>
        static int compare(T const& first, T const& second, bool descending)
        {
            return descending ? compare(second, first) : compare(first, second);
        }

        template <typename T>
        static int compare(T const& first, T const& second, Qt::SortOrder sortOrder)
        {
            return compare(first, second, sortOrder == Qt::DescendingOrder);
        }

        template <typename T>
        static int compare(T const& first, T const& second,
                           std::function<int(T const& first, T const& second)> comparer,
                           bool descending)
        {
            return descending ? comparer(second, first) : comparer(first, second);
        }

        template <typename T>
        static int compare(T const& first, T const& second,
                           std::function<int(T const& first, T const& second)> comparer,
                           Qt::SortOrder sortOrder)
        {
            return compare(first, second, comparer, sortOrder == Qt::DescendingOrder);
        }
    };

    }

    // ============================================================================ //

    SortedCollectionTableModel::SortedCollectionTableModel(QObject* parent,
                                             ServerInterface* serverInterface,
                                             QueueHashesMonitor* queueHashesMonitor,
                                       UserForStatisticsDisplay* userForStatisticsDisplay)
     : QAbstractTableModel(parent),
       _hashIdRepository(serverInterface->hashIdRepository()),
       _highlightColorIndex(0),
       _sortBy(0),
       _sortOrder(Qt::AscendingOrder),
       _highlightingTrackJudge(serverInterface->userDataFetcher(), *queueHashesMonitor),
       _queueHashesMonitor(queueHashesMonitor)
    {
        _collator.setCaseSensitivity(Qt::CaseInsensitive);
        _collator.setNumericMode(true);

        /* we need to ignore symbols such as quotes, spaces and parentheses */
        _collator.setIgnorePunctuation(true);

        auto* playerController = &serverInterface->playerController();
        _playerState = playerController->playerState();
        connect(
            playerController, &PlayerController::playerStateChanged,
            this,
            [this, playerController]()
            {
                _playerState = playerController->playerState();

                if (!_currentTrackHash.isZero())
                    markLeftColumnAsChanged();
            }
        );

        _highlightingTrackJudge.setCriteria(TrackCriterium::NoTracks,
                                            TrackCriterium::AllTracks,
                                            TrackCriterium::AllTracks);
        _highlightingTrackJudge.setUserId(userForStatisticsDisplay->userId().valueOr(0));
        connect(
            userForStatisticsDisplay, &UserForStatisticsDisplay::userChanged,
            this,
            [this, userForStatisticsDisplay]()
            {
                _highlightingTrackJudge.setUserId(
                                           userForStatisticsDisplay->userId().valueOr(0));

                if (_highlightingTrackJudge.criteriumUsesUserData())
                {
                    markEverythingAsChanged();
                }
            }
        );

        auto& collectionWatcher = serverInterface->collectionWatcher();
        collectionWatcher.enableCollectionDownloading();

        connect(
            &collectionWatcher, &CollectionWatcher::newTrackReceived,
            this, &SortedCollectionTableModel::onNewTrackReceived
        );
        connect(
            &collectionWatcher, &CollectionWatcher::trackAvailabilityChanged,
            this, &SortedCollectionTableModel::onTrackAvailabilityChanged
        );
        connect(
            &collectionWatcher, &CollectionWatcher::trackDataChanged,
            this, &SortedCollectionTableModel::onTrackDataChanged
        );

        auto& userDataFetcher = serverInterface->userDataFetcher();

        connect(
            &userDataFetcher, &UserDataFetcher::userTrackDataChanged,
            this, &SortedCollectionTableModel::onUserTrackDataChanged
        );

        auto* currentTrackMonitor = &serverInterface->currentTrackMonitor();
        _currentTrackHash = currentTrackMonitor->currentTrackHash();
        connect(
            currentTrackMonitor, &CurrentTrackMonitor::currentTrackInfoChanged,
            this,
            [this, currentTrackMonitor]()
            {
                currentTrackInfoChanged(currentTrackMonitor->currentTrackHash());
            }
        );

        connect(_queueHashesMonitor, &QueueHashesMonitor::hashInQueuePresenceChanged,
                this, &SortedCollectionTableModel::onHashInQueuePresenceChanged);

        addWhenModelEmpty(collectionWatcher.getCollection().values());
    }

    void SortedCollectionTableModel::setHighlightCriterium(TrackCriterium criterium)
    {
        _highlightingTrackJudge.setCriteria(criterium,
                                            TrackCriterium::AllTracks,
                                            TrackCriterium::AllTracks);

        /* notify the outside world that potentially everything has changed */
        markEverythingAsChanged();
    }

    int SortedCollectionTableModel::highlightColorIndex() const
    {
        return _highlightColorIndex;
    }

    bool SortedCollectionTableModel::lessThan(int index1, int index2) const
    {
        //qDebug() << "lessThan called:" << index1 << "," << index2;
        return compareTracks(*_tracks.at(index1), *_tracks.at(index2)) < 0;
    }

    bool SortedCollectionTableModel::lessThan(const CollectionTrackInfo& track1,
                                              const CollectionTrackInfo& track2) const
    {
        return compareTracks(track1, track2) < 0;
    }

    void SortedCollectionTableModel::sortByTitle()
    {
        sort(0);
    }

    void SortedCollectionTableModel::sortByArtist()
    {
        sort(1);
    }

    int SortedCollectionTableModel::sortColumn() const
    {
        return _sortBy;
    }

    Qt::SortOrder SortedCollectionTableModel::sortOrder() const
    {
        return _sortOrder;
    }

    int SortedCollectionTableModel::compareStrings(const QString& s1, const QString& s2,
                                                   Qt::SortOrder sortOrder) const
    {
        return sortOrder == Qt::DescendingOrder
                ? _collator.compare(s2, s1)
                : _collator.compare(s1, s2);
    }

    int SortedCollectionTableModel::compareTracks(const CollectionTrackInfo& track1,
                                                  const CollectionTrackInfo& track2) const
    {
        switch (_sortBy)
        {
            case 0:
            default:
                return compareTitles(track1, track2, _sortOrder);
            case 1:
                return compareArtists(track1, track2, _sortOrder);
            case 2:
                return compareLengths(track1, track2, _sortOrder);
            case 3:
                return compareAlbums(track1, track2, _sortOrder);
        }
    }

    int SortedCollectionTableModel::compareTitles(const CollectionTrackInfo& track1,
                                                  const CollectionTrackInfo& track2,
                                                  Qt::SortOrder sortOrder) const
    {
        bool empty1 = track1.titleAndArtistUnknown();
        bool empty2 = track2.titleAndArtistUnknown();

        if (empty1 || empty2)
        {
            if (!empty1) return -1; /* track 1 goes first */
            if (!empty2) return 1; /* track 2 goes first */

            /* both are empty; compare other properties */
        }
        else
        {
            auto title1 = track1.title();
            auto title2 = track2.title();
            int titleComparison = compareStrings(title1, title2, sortOrder);
            if (titleComparison != 0) return titleComparison;

            auto artist1 = track1.artist();
            auto artist2 = track2.artist();
            int artistComparison = compareStrings(artist1, artist2, sortOrder);
            if (artistComparison != 0) return artistComparison;
        }

        return Comparisons::compare(track1.hashId(), track2.hashId(), sortOrder);
    }

    int SortedCollectionTableModel::compareArtists(const CollectionTrackInfo& track1,
                                                   const CollectionTrackInfo& track2,
                                                   Qt::SortOrder sortOrder) const
    {
        bool empty1 = track1.titleAndArtistUnknown();
        bool empty2 = track2.titleAndArtistUnknown();

        if (empty1 || empty2)
        {
            if (!empty1) return -1; /* track 1 goes first */
            if (!empty2) return 1; /* track 2 goes first */

            /* both are empty; compare other properties */
        }
        else
        {
            auto artist1 = track1.artist();
            auto artist2 = track2.artist();
            int artistComparison = compareStrings(artist1, artist2, sortOrder);
            if (artistComparison != 0) return artistComparison;

            auto title1 = track1.title();
            auto title2 = track2.title();
            int titleComparison = compareStrings(title1, title2, sortOrder);
            if (titleComparison != 0) return titleComparison;
        }

        return Comparisons::compare(track1.hashId(), track2.hashId());
    }

    int SortedCollectionTableModel::compareLengths(const CollectionTrackInfo& track1,
                                                   const CollectionTrackInfo& track2,
                                                   Qt::SortOrder sortOrder) const
    {
        auto length1 = track1.lengthInMilliseconds();
        auto length2 = track2.lengthInMilliseconds();

        if (length1 < 0 || length2 < 0)
        {
            if (length1 >= 0) return -1; /* track 1 goes first */
            if (length2 >= 0) return 1; /* track 2 goes first */

            /* both are empty; compare other properties */
        }
        else
        {
            int comparison = Comparisons::compare(length1, length2, sortOrder);
            if (comparison != 0) return comparison;
        }

        auto title1 = track1.title();
        auto title2 = track2.title();
        int titleComparison = compareStrings(title1, title2, sortOrder);
        if (titleComparison != 0) return titleComparison;

        auto artist1 = track1.artist();
        auto artist2 = track2.artist();
        int artistComparison = compareStrings(artist1, artist2, sortOrder);
        if (artistComparison != 0) return artistComparison;

        return Comparisons::compare(track1.hashId(), track2.hashId());
    }

    int SortedCollectionTableModel::compareAlbums(const CollectionTrackInfo& track1,
                                                  const CollectionTrackInfo& track2,
                                                  Qt::SortOrder sortOrder) const
    {
        auto album1 = track1.album();
        auto album2 = track2.album();

        bool noAlbum1 = album1.isEmpty();
        bool noAlbum2 = album2.isEmpty();

        if (noAlbum1 || noAlbum2)
        {
            if (!noAlbum1) return -1; /* track 1 goes first */
            if (!noAlbum2) return 1; /* track 2 goes first */

            /* both are empty; compare other properties */
        }
        else
        {
            int comparison = compareStrings(album1, album2, sortOrder);
            if (comparison != 0) return comparison;
        }

        auto title1 = track1.title();
        auto title2 = track2.title();
        int titleComparison = compareStrings(title1, title2, sortOrder);
        if (titleComparison != 0) return titleComparison;

        return Comparisons::compare(track1.hashId(), track2.hashId());
    }

    void SortedCollectionTableModel::onNewTrackReceived(CollectionTrackInfo track)
    {
        addOrUpdateTrack(track);
    }

    void SortedCollectionTableModel::onTrackAvailabilityChanged(LocalHashId hashId,
                                                                bool isAvailable)
    {
        updateTrackAvailability(hashId, isAvailable);
    }

    void SortedCollectionTableModel::onTrackDataChanged(CollectionTrackInfo track)
    {
        addOrUpdateTrack(track);
    }

    void SortedCollectionTableModel::onUserTrackDataChanged(quint32 userId,
                                                            LocalHashId hashId)
    {
        Q_UNUSED(userId)
        /* ignore the user ID for change notifications */

        auto outerIndex = findOuterIndexForHash(hashId);
        if (outerIndex < 0)
            return; /* track is not in the list */

        Q_EMIT dataChanged(createIndex(outerIndex, 0),
                           createIndex(outerIndex, 4 - 1));
    }

    void SortedCollectionTableModel::currentTrackInfoChanged(LocalHashId hashId)
    {
        if (_currentTrackHash == hashId)
            return;

        _currentTrackHash = hashId;
        markLeftColumnAsChanged();
    }

    void SortedCollectionTableModel::onHashInQueuePresenceChanged(LocalHashId hashId)
    {
        qDebug() << "hash in queue presence changed: " << hashId
                 << "present?"
                 << (_queueHashesMonitor->isPresentInQueue(hashId) ? "yes" : "no");

        auto outerIndex = findOuterIndexForHash(hashId);
        if (outerIndex < 0)
            return; /* track is not in the list */

        Q_EMIT dataChanged(createIndex(outerIndex, 0),
                           createIndex(outerIndex, 0));
    }

    int SortedCollectionTableModel::findOuterIndexMapIndexForInsert(
            const CollectionTrackInfo& track)
    {
        return findOuterIndexMapIndexForInsert(track, 0, _outerToInnerIndexMap.size());
    }

    int SortedCollectionTableModel::findOuterIndexMapIndexForInsert(
            const CollectionTrackInfo& track,
            int searchRangeBegin, int searchRangeEnd /* end is not part of the range */)
    {
        if (searchRangeBegin > searchRangeEnd) return -1 /* problem */;

        if (searchRangeEnd - searchRangeBegin <= 50) /* do linear search */
        {
            for (int index = searchRangeBegin; index < searchRangeEnd; ++index)
            {
                auto const& current = *_tracks.at(_outerToInnerIndexMap.at(index));

                if (lessThan(track, current)) return index;
            }

            return searchRangeEnd;
        }

        /* binary search */

        int middleIndex = searchRangeBegin / 2 + searchRangeEnd / 2;
        auto const& middleTrack = *_tracks.at(_outerToInnerIndexMap.at(middleIndex));

        if (lessThan(track, middleTrack))
        {
            return findOuterIndexMapIndexForInsert(track, searchRangeBegin, middleIndex);
        }
        else
        {
            return findOuterIndexMapIndexForInsert(track, middleIndex, searchRangeEnd);
        }
    }

    int SortedCollectionTableModel::findOuterIndexForHash(LocalHashId hashId)
    {
        auto innerIndex = _hashesToInnerIndexes.value(hashId, -1);
        if (innerIndex < 0)
            return -1; /* track is not in the list */

        auto outerIndex = _innerToOuterIndexMap[innerIndex];
        return outerIndex;
    }

    void SortedCollectionTableModel::markRowAsChanged(int index)
    {
        Q_EMIT dataChanged(createIndex(index, 0), createIndex(index, 4 - 1));
    }

    void SortedCollectionTableModel::markLeftColumnAsChanged()
    {
        Q_EMIT dataChanged(
            createIndex(0, 0), createIndex(rowCount() - 1, 0)
        );
    }

    void SortedCollectionTableModel::updateTrackAvailability(LocalHashId hashId,
                                                             bool isAvailable)
    {
        auto hashIterator = _hashesToInnerIndexes.find(hashId);
        if (hashIterator == _hashesToInnerIndexes.end())
            return; /* not supposed to happen, ignore it */

        int innerIndex = hashIterator.value();
        int outerIndex = _innerToOuterIndexMap[innerIndex];

        _tracks[innerIndex]->setAvailable(isAvailable);
        Q_EMIT dataChanged(createIndex(outerIndex, 0), createIndex(outerIndex, 4 - 1));
    }

    template <class T>
    void SortedCollectionTableModel::addWhenModelEmpty(T trackCollection)
    {
        if (trackCollection.size() == 0)
            return;

        QVector<CollectionTrackInfo*> trackList;
        QHash<LocalHashId, int> hashIndexer;
        trackList.reserve(trackCollection.size());
        hashIndexer.reserve(trackCollection.size());

        for (int i = 0; i < trackCollection.size(); ++i)
        {
            CollectionTrackInfo const& track = trackCollection[i];

            if (hashIndexer.contains(track.hashId()))
                continue; /* already present */

            if (!track.isAvailable() && track.titleAndArtistUnknown())
                continue; /* not interesting enough to add */

            auto trackObj = new CollectionTrackInfo(track);
            hashIndexer.insert(track.hashId(), trackList.size());
            trackList.append(trackObj);
        }

        qDebug() << " addWhenModelEmpty: inserting" << trackList.size() << "tracks";

        if (!trackList.empty())
        {
            _outerToInnerIndexMap.reserve(trackList.size());
            _innerToOuterIndexMap.reserve(trackList.size());

            beginInsertRows(QModelIndex(), 0, trackList.size() - 1);
            _tracks = trackList;
            _hashesToInnerIndexes = hashIndexer;
            buildIndexMaps();
            endInsertRows();
        }
    }

    void SortedCollectionTableModel::addOrUpdateTrack(CollectionTrackInfo const& track)
    {
        /* hash already present? */
        auto hashIterator = _hashesToInnerIndexes.find(track.hashId());
        if (hashIterator != _hashesToInnerIndexes.end())
        {
            auto innerIndex = hashIterator.value();
            updateTrack(innerIndex, track);
            return;
        }

        if (!track.isAvailable() && track.titleAndArtistUnknown())
            return; /* not interesting enough to add */

        addTrack(track);
    }

    void SortedCollectionTableModel::addTrack(const CollectionTrackInfo& track)
    {
        int indexToInsertAt = findOuterIndexMapIndexForInsert(track);

        beginInsertRows(QModelIndex(), indexToInsertAt, indexToInsertAt);
        auto trackObj = new CollectionTrackInfo(track);
        int innerIndex = _tracks.size();
        _tracks.append(trackObj);
        _hashesToInnerIndexes.insert(track.hashId(), innerIndex);
        _outerToInnerIndexMap.insert(indexToInsertAt, innerIndex);
        _innerToOuterIndexMap.append(indexToInsertAt);

        /* all elements that were pushed down by the insert got a new outer index;
         * update the inner to outer map to reflect this. */
        rebuildInnerMap(indexToInsertAt + 1);

        endInsertRows();
    }

    void SortedCollectionTableModel::updateTrack(int innerIndex,
                                                 const CollectionTrackInfo& newTrackData)
    {
        auto& track = *_tracks[innerIndex];

        qDebug() << "collection track update:"
                   << "title:" << newTrackData.title()
                   << "; artist:" << newTrackData.artist()
                   << "; album:" << newTrackData.album()
                   << "; available:" << (newTrackData.isAvailable() ? "yes" : "no")
                   << "; hash ID:" << newTrackData.hashId();

        int oldOuterIndex = _innerToOuterIndexMap[innerIndex];
        int insertionIndex = findOuterIndexMapIndexForInsert(newTrackData);

        if (insertionIndex >= oldOuterIndex && insertionIndex - 1 <= oldOuterIndex)
        {
            /* no row move necessary, just update the row */
            track = newTrackData;

            Q_EMIT dataChanged(createIndex(oldOuterIndex, 0),
                               createIndex(oldOuterIndex, 4 - 1));
            return;
        }

        qDebug() << "track update causing row move:"
                 << " old index:" << oldOuterIndex
                 << " insertion index:" << insertionIndex;

        bool moving = beginMoveRows(QModelIndex(), oldOuterIndex, oldOuterIndex,
                                    QModelIndex(), insertionIndex);
        Q_ASSERT_X(moving, "SortedCollectionTableModel::updateTrack", "row move failed");

        int outerIndexAfterMove =
            (insertionIndex > oldOuterIndex) ? insertionIndex - 1 : insertionIndex;

        track = newTrackData;
        _outerToInnerIndexMap.move(oldOuterIndex, outerIndexAfterMove);

        /* elements between the old and new index got a new outer index;
               update the inner to outer map to reflect this */
        rebuildInnerMap(qMin(oldOuterIndex, outerIndexAfterMove),
                        qMax(oldOuterIndex, outerIndexAfterMove) + 1);

        endMoveRows();

        /* not sure if this is really necessary */
        markRowAsChanged(outerIndexAfterMove);
    }

    void SortedCollectionTableModel::sort(int column, Qt::SortOrder order)
    {
        if (_sortBy == column && _sortOrder == order) return;

        _sortBy = column;
        _sortOrder = order;

        if (_outerToInnerIndexMap.empty()) return;

        /* sort outer index map */
        std::sort(
            _outerToInnerIndexMap.begin(), _outerToInnerIndexMap.end(),
            [this](int index1, int index2) { return lessThan(index1, index2); }
        );

        /* construct inner map from outer map */
        rebuildInnerMap();

        /* notify the outside world that potentially everything has changed */
        markEverythingAsChanged();
    }

    void SortedCollectionTableModel::markEverythingAsChanged()
    {
        Q_EMIT dataChanged(
            createIndex(0, 0), createIndex(rowCount() - 1, columnCount() - 1)
        );
    }

    void SortedCollectionTableModel::buildIndexMaps()
    {
        /* generate unsorted maps */
        _innerToOuterIndexMap.resize(_tracks.size());
        _outerToInnerIndexMap.resize(_tracks.size());
        for (int i = 0; i < _tracks.size(); ++i)
        {
            _innerToOuterIndexMap[i] = i;
            _outerToInnerIndexMap[i] = i;
        }

        /* sort outer index map */
        std::sort(
            _outerToInnerIndexMap.begin(), _outerToInnerIndexMap.end(),
            [this](int index1, int index2) { return lessThan(index1, index2); }
        );

        /* construct inner map from outer map */
        rebuildInnerMap();
    }

    void SortedCollectionTableModel::rebuildInnerMap(int outerStartIndex)
    {
        rebuildInnerMap(outerStartIndex, _outerToInnerIndexMap.size());
    }

    void SortedCollectionTableModel::rebuildInnerMap(int outerStartIndex,
                                                     int outerEndIndex)
    {
        /* (re)construct inner map from outer map */
        for (int i = outerStartIndex; i < outerEndIndex; ++i)
        {
            _innerToOuterIndexMap[_outerToInnerIndexMap[i]] = i;
        }
    }

    CollectionTrackInfo const* SortedCollectionTableModel::trackAt(
                                                           const QModelIndex& index) const
    {
        return trackAt(index.row());
    }

    CollectionTrackInfo const* SortedCollectionTableModel::trackAt(int rowIndex) const
    {
        if (rowIndex < 0 || rowIndex >= _tracks.size()) { return nullptr; }

        return _tracks.at(_outerToInnerIndexMap.at(rowIndex));
    }

    int SortedCollectionTableModel::trackIndex(Client::LocalHashId hashId) const
    {
        auto it = _hashesToInnerIndexes.constFind(hashId);
        if (it == _hashesToInnerIndexes.constEnd())
            return -1;

        auto innerIndex = it.value();
        return _innerToOuterIndexMap[innerIndex];
    }

    int SortedCollectionTableModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent)
        return _outerToInnerIndexMap.size();
    }

    int SortedCollectionTableModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent)
        return 4;
    }

    QVariant SortedCollectionTableModel::headerData(int section,
                                                    Qt::Orientation orientation,
                                                    int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
                case 0: return QString(tr("Title"));
                case 1: return QString(tr("Artist"));
                case 2: return QString(tr("Length"));
                case 3: return QString(tr("Album"));
            }
        }

        return {};
    }

    namespace
    {
        inline QVariant toQVariant(QFlags<Qt::Alignment::enum_type> alignment)
        {
            return static_cast<Qt::Alignment::Int>(alignment);
        }
    }

    QVariant SortedCollectionTableModel::data(const QModelIndex& index, int role) const
    {
        switch (role)
        {
            case Qt::TextAlignmentRole:
                switch (index.column())
                {
                    case 2: return toQVariant(Qt::AlignRight | Qt::AlignVCenter);
                }
                break;
            case Qt::DisplayRole:
                if (index.row() < _tracks.size())
                {
                    auto track = trackAt(index);

                    switch (index.column())
                    {
                        case 0: return track->title();
                        case 1: return track->artist();
                        case 2:
                        {
                            qint64 lengthInMilliseconds = track->lengthInMilliseconds();
                            if (lengthInMilliseconds < 0) { return "?"; }

                            return Util::millisecondsToShortDisplayTimeText(
                                                                    lengthInMilliseconds);
                        }
                        case 3: return track->album();
                    }
                }
                break;
            case Qt::DecorationRole:
                if (index.column() == 0 && index.row() < _tracks.size())
                {
                    auto track = trackAt(index);

                    if (track->hashId() == _currentTrackHash)
                    {
                        switch (_playerState)
                        {
                            case PlayerState::Playing:
                                return QIcon(":/mediabuttons/play.svg");

                            case PlayerState::Paused:
                                return QIcon(":/mediabuttons/pause.svg");

                            default:
                                break;
                        }
                    }
                    else if (_queueHashesMonitor->isPresentInQueue(track->hashId()))
                    {
                        return QIcon(":/mediabuttons/queue.svg");
                    }
                }
                break;
            case Qt::ForegroundRole:
                if (index.row() < _tracks.size())
                {
                    auto track = trackAt(index);
                    if (!track->isAvailable())
                        return QBrush(Colors::instance().inactiveItemForeground);
                }
                break;
            case Qt::BackgroundRole:
                if (index.row() < _tracks.size())
                {
                    auto track = trackAt(index);
                    auto judgement =
                           _highlightingTrackJudge.trackSatisfiesCriteria(*track);

                    if (judgement.isTrue())
                    {
                        auto& colors = Colors::instance().itemBackgroundHighlightColors;

                        auto colorIndex =
                                qBound(0, _highlightColorIndex, colors.size() - 1);

                        auto color = colors[colorIndex];

                        return QBrush(color);
                    }
                }
                break;
        }

        return {};
    }

    Qt::ItemFlags SortedCollectionTableModel::flags(const QModelIndex& index) const
    {
        Q_UNUSED(index)
        Qt::ItemFlags f(Qt::ItemIsSelectable | Qt::ItemIsEnabled
                        | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        return f;
    }

    Qt::DropActions SortedCollectionTableModel::supportedDragActions() const
    {
        return Qt::CopyAction;
    }

    Qt::DropActions SortedCollectionTableModel::supportedDropActions() const
    {
        return Qt::CopyAction;
    }

    QMimeData* SortedCollectionTableModel::mimeData(const QModelIndexList& indexes) const
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

            auto hashId = trackAt(row)->hashId();
            auto hash = _hashIdRepository->getHash(hashId);

            qDebug() << " row" << row << "; col" << index.column()
                     << "; hash ID" << hashId << "; hash" << hash;
            hashes.append(hash);
        }

        if (hashes.empty()) return nullptr;

        stream << quint32(hashes.size());
        for (int i = 0; i < hashes.size(); ++i)
        {
            stream << quint64(hashes[i].length());
            stream << hashes[i].SHA1();
            stream << hashes[i].MD5();
        }

        buffer.close();

        QMimeData* data = new QMimeData();

        data->setData("application/x-pmp-filehash", buffer.data());
        return data;
    }

    void SortedCollectionTableModel::setHighlightColorIndex(int colorIndex)
    {
        if (_highlightColorIndex == colorIndex)
            return;

        _highlightColorIndex = colorIndex;

        /* ensure that the model is repainted */
        markEverythingAsChanged();
    }

    // ============================================================================ //

    FilteredCollectionTableModel::FilteredCollectionTableModel(QObject* parent,
                                    SortedCollectionTableModel* source,
                                    ServerInterface* serverInterface,
                                    SearchData* searchData,
                                    QueueHashesMonitor* queueHashesMonitor,
                                    UserForStatisticsDisplay* userForStatisticsDisplay)
     : _serverInterface(serverInterface),
       _source(source),
       _searchData(searchData),
       _filteringTrackJudge(serverInterface->userDataFetcher(), *queueHashesMonitor)
    {
        Q_UNUSED(parent)
        setFilterCaseSensitivity(Qt::CaseInsensitive);

        _filteringTrackJudge.setUserId(userForStatisticsDisplay->userId().valueOr(0));
        connect(
            userForStatisticsDisplay, &UserForStatisticsDisplay::userChanged,
            this,
            [this, userForStatisticsDisplay]()
            {
                _filteringTrackJudge.setUserId(
                                           userForStatisticsDisplay->userId().valueOr(0));
                invalidateFilter();
            }
        );

        auto& collectionWatcher = serverInterface->collectionWatcher();
        connect(
            &collectionWatcher, &CollectionWatcher::newTrackReceived,
            this, &FilteredCollectionTableModel::onNewTrackReceived
        );

        setSourceModel(source);
    }

    void FilteredCollectionTableModel::setTrackFilters(TrackCriterium criterium1,
                                                       TrackCriterium criterium2,
                                                       TrackCriterium criterium3)
    {
        bool changed =
            _filteringTrackJudge.setCriteria(criterium1, criterium2, criterium3);

        if (changed)
            invalidateFilter();
    }

    void FilteredCollectionTableModel::sort(int column, Qt::SortOrder order)
    {
        _source->sort(column, order);
    }

    CollectionTrackInfo const* FilteredCollectionTableModel::trackAt(
                                                           const QModelIndex& index) const
    {
        return _source->trackAt(mapToSource(index));
    }

    void FilteredCollectionTableModel::setSearchText(QString search)
    {
        auto fileHash = FileHash::tryParse(search.trimmed());
        if (!fileHash.isNull())
        {
            // id will be zero when not found
            auto hashId = _serverInterface->hashIdRepository()->getId(fileHash);

            if (hashId.isZero())
            {
                // trigger server lookup of the hash
                (void)_serverInterface->collectionWatcher().getTrackInfo(fileHash);
            }

            _searchQuery.clear();
            _searchFileHash = fileHash;
            _searchHashId = hashId;
        }
        else
        {
            _searchQuery = SearchQuery(search);
            _searchFileHash = FileHash();
            _searchHashId = null;
        }

        invalidateFilter();
    }

    bool FilteredCollectionTableModel::filterAcceptsRow(int sourceRow,
                                                    const QModelIndex& sourceParent) const
    {
        Q_UNUSED(sourceParent)

        if (_searchQuery.isEmpty() && _searchHashId == null
            && _filteringTrackJudge.criteriumResultsInAllTracks())
        {
            return true; /* not filtered */
        }

        auto* track = _source->trackAt(sourceRow);

        if (!_searchQuery.isEmpty())
        {
            if (!_searchData->isFileMatchForQuery(track->hashId(), _searchQuery))
                return false;
        }
        else if (_searchHashId != null)
        {
            if (track->hashId() != _searchHashId.value())
                return false;
        }

        return _filteringTrackJudge.trackSatisfiesCriteria(*track).isTrue();
    }

    void FilteredCollectionTableModel::onNewTrackReceived(CollectionTrackInfo track)
    {
        /* See if we can finally get the LocalHashId of the FileHash that is used as the
           search query */
        if (_searchHashId != null && _searchHashId.value().isZero()
            && !_searchFileHash.isNull())
        {
            auto fileHashOfNewTrack =
                _serverInterface->hashIdRepository()->getHash(track.hashId());

            if (fileHashOfNewTrack == _searchFileHash)
            {
                _searchHashId = track.hashId();
                invalidateFilter();
            }
        }
    }
}

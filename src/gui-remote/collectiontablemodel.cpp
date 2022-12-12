/*
    Copyright (C) 2016-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/util.h"

#include "client/clientserverinterface.h"
#include "client/collectionwatcher.h"
#include "client/currenttrackmonitor.h"
#include "client/playercontroller.h"
#include "client/userdatafetcher.h"

#include "colors.h"

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
    void TrackJudge::setUserId(quint32 userId)
    {
        _userId = userId;
        _haveUserId = true;

        _userDataFetcher.enableAutoFetchForUser(userId);
    }

    TriBool TrackJudge::trackSatisfiesCriterium(CollectionTrackInfo const& track,
                                                bool resultForNone) const
    {
        switch (_criterium)
        {
            case TrackCriterium::None:
                return resultForNone;

            case TrackCriterium::NeverHeard:
            {
                auto evaluator = [](QDateTime prevHeard) { return !prevHeard.isValid(); };
                return trackSatisfiesLastHeardDateCriterium(track, evaluator);
            }
            case TrackCriterium::LastHeardNotInLast1000Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 1000);

            case TrackCriterium::LastHeardNotInLast365Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 365);

            case TrackCriterium::LastHeardNotInLast180Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 180);

            case TrackCriterium::LastHeardNotInLast90Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 90);

            case TrackCriterium::LastHeardNotInLast30Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 30);

            case TrackCriterium::LastHeardNotInLast10Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 10);

            case TrackCriterium::WithoutScore:
            {
                auto evaluator = [](int permillage) { return permillage < 0; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreMaximum30:
            {
                auto evaluator =
                    [](int permillage) { return permillage >= 0 && permillage <= 300; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreAtLeast85:
            {
                auto evaluator = [](int permillage) { return permillage >= 850; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreAtLeast90:
            {
                auto evaluator = [](int permillage) { return permillage >= 900; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreAtLeast95:
            {
                auto evaluator = [](int permillage) { return permillage >= 950; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::LengthMaximumOneMinute:
                if (!track.lengthIsKnown()) return TriBool::unknown;
                return track.lengthInMilliseconds() <= 60 * 1000;

            case TrackCriterium::LengthAtLeastFiveMinutes:
                if (!track.lengthIsKnown()) return TriBool::unknown;
                return track.lengthInMilliseconds() >= 5 * 60 * 1000;
        }

        return false;
    }

    TriBool TrackJudge::trackSatisfiesScoreCriterium(
                               CollectionTrackInfo const& track,
                               std::function<TriBool(int)> scorePermillageEvaluator) const
    {
        if (!_haveUserId) return TriBool::unknown;

        auto hashDataForUser = _userDataFetcher.getHashDataForUser(_userId, track.hash());

        if (!hashDataForUser || !hashDataForUser->scoreReceived)
            return TriBool::unknown;

        return scorePermillageEvaluator(hashDataForUser->scorePermillage);
    }

    TriBool TrackJudge::trackSatisfiesLastHeardDateCriterium(
                                    CollectionTrackInfo const& track,
                                    std::function<TriBool(QDateTime)> dateEvaluator) const
    {
        if (!_haveUserId) return TriBool::unknown;

        auto hashDataForUser = _userDataFetcher.getHashDataForUser(_userId, track.hash());

        if (!hashDataForUser || !hashDataForUser->previouslyHeardReceived)
            return TriBool::unknown;

        return dateEvaluator(hashDataForUser->previouslyHeard);
    }

    TriBool TrackJudge::trackSatisfiesNotHeardInTheLastXDaysCriterium(
                                                         const CollectionTrackInfo& track,
                                                         int days) const
    {
        auto evaluator =
            [days](QDateTime prevHeard)
            {
                return !prevHeard.isValid()
                        || prevHeard <= QDateTime::currentDateTimeUtc().addDays(-days);
            };

        return trackSatisfiesLastHeardDateCriterium(track, evaluator);
    }

    // ============================================================================ //

    CollectionViewContext::CollectionViewContext(QObject* parent,
                                             ClientServerInterface* clientServerInterface)
     : QObject(parent),
       _userId(0)
    {
        auto* playerController = &clientServerInterface->playerController();

        connect(
            playerController, &PlayerController::playerModeChanged,
            this,
            [this](PlayerMode playerMode, quint32 personalModeUserId)
            {
                qDebug() << "CollectionViewContext: received player mode" << playerMode
                         << "with user ID" << personalModeUserId;

                if (playerMode == PlayerMode::Unknown
                        || _userId == personalModeUserId)
                {
                    return;
                }

                _userId = personalModeUserId;
                Q_EMIT userIdChanged();
            }
        );

        auto playerMode = playerController->playerMode();
        if (playerMode != PlayerMode::Unknown)
        {
            _userId = playerController->personalModeUserId();
        }
    }

    // ============================================================================ //

    namespace {

    class Comparisons {
    public:

        template <typename T>
        static int compare(T const& first, T const& second) {
            if (first < second) return -1;
            if (second < first) return 1;
            return 0;
        }

        template <typename T>
        static int compare(T const& first, T const& second, bool descending) {
            return descending ? compare(second, first) : compare(first, second);
        }

        template <typename T>
        static int compare(T const& first, T const& second, Qt::SortOrder sortOrder) {
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

        /*static int compare<>(QString const& first, QString const& second) {
            return first.compare(second);
        }*/

    };

    }

    // ============================================================================ //

    SortedCollectionTableModel::SortedCollectionTableModel(QObject* parent,
                                             ClientServerInterface* clientServerInterface,
                                             CollectionViewContext* collectionViewContext)
     : QAbstractTableModel(parent),
       _highlightColorIndex(0),
       _sortBy(0),
       _sortOrder(Qt::AscendingOrder),
       _highlightingTrackJudge(clientServerInterface->userDataFetcher())
    {
        _collator.setCaseSensitivity(Qt::CaseInsensitive);
        _collator.setNumericMode(true);

        /* we need to ignore symbols such as quotes, spaces and parentheses */
        _collator.setIgnorePunctuation(true);

        auto* playerController = &clientServerInterface->playerController();
        _playerState = playerController->playerState();
        connect(
            playerController, &PlayerController::playerStateChanged,
            this,
            [this, playerController]() {
                _playerState = playerController->playerState();

                if (!_currentTrackHash.isNull())
                    markLeftColumnAsChanged();
            }
        );

        _highlightingTrackJudge.setUserId(collectionViewContext->userId());
        connect(
            collectionViewContext, &CollectionViewContext::userIdChanged,
            this,
            [this, collectionViewContext]()
            {
                _highlightingTrackJudge.setUserId(collectionViewContext->userId());

                if (usesUserData(_highlightingTrackJudge.getCriterium()))
                {
                    markEverythingAsChanged();
                }
            }
        );

        auto& collectionWatcher = clientServerInterface->collectionWatcher();
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

        auto& userDataFetcher = clientServerInterface->userDataFetcher();

        connect(
            &userDataFetcher, &UserDataFetcher::userTrackDataChanged,
            this, &SortedCollectionTableModel::onUserTrackDataChanged
        );

        auto* currentTrackMonitor = &clientServerInterface->currentTrackMonitor();
        _currentTrackHash = currentTrackMonitor->currentTrackHash();
        connect(
            currentTrackMonitor, &CurrentTrackMonitor::currentTrackInfoChanged,
            this,
            [this, currentTrackMonitor]() {
                currentTrackInfoChanged(currentTrackMonitor->currentTrackHash());
            }
        );

        addWhenModelEmpty(collectionWatcher.getCollection().values());
    }

    void SortedCollectionTableModel::setHighlightCriterium(TrackCriterium criterium)
    {
        _highlightingTrackJudge.setCriterium(criterium);

        /* notify the outside world that potentially everything has changed */
        markEverythingAsChanged();
    }

    int SortedCollectionTableModel::highlightColorIndex() const
    {
        return _highlightColorIndex;
    }

    bool SortedCollectionTableModel::usesUserData(TrackCriterium mode)
    {
        switch (mode) {
            case TrackCriterium::NeverHeard:
            case TrackCriterium::LastHeardNotInLast1000Days:
            case TrackCriterium::LastHeardNotInLast365Days:
            case TrackCriterium::LastHeardNotInLast180Days:
            case TrackCriterium::LastHeardNotInLast90Days:
            case TrackCriterium::LastHeardNotInLast30Days:
            case TrackCriterium::LastHeardNotInLast10Days:
            case TrackCriterium::WithoutScore:
            case TrackCriterium::ScoreMaximum30:
            case TrackCriterium::ScoreAtLeast85:
            case TrackCriterium::ScoreAtLeast90:
            case TrackCriterium::ScoreAtLeast95:
                return true;

            case TrackCriterium::None:
            case TrackCriterium::LengthMaximumOneMinute:
            case TrackCriterium::LengthAtLeastFiveMinutes:
                break;
        }

        return false;
    }

    bool SortedCollectionTableModel::lessThan(int index1, int index2) const {
        //qDebug() << "lessThan called:" << index1 << "," << index2;
        return compareTracks(*_tracks.at(index1), *_tracks.at(index2)) < 0;
    }

    bool SortedCollectionTableModel::lessThan(const CollectionTrackInfo& track1,
                                              const CollectionTrackInfo& track2) const
    {
        return compareTracks(track1, track2) < 0;
    }

    void SortedCollectionTableModel::sortByTitle() {
        sort(0);
    }

    void SortedCollectionTableModel::sortByArtist() {
        sort(1);
    }

    int SortedCollectionTableModel::sortColumn() const {
        return _sortBy;
    }

    Qt::SortOrder SortedCollectionTableModel::sortOrder() const {
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
        switch (_sortBy) {
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

        if (empty1 || empty2) {
            if (!empty1) return -1; /* track 1 goes first */
            if (!empty2) return 1; /* track 2 goes first */

            /* both are empty; compare other properties */
        }
        else {
            auto title1 = track1.title();
            auto title2 = track2.title();
            int titleComparison = compareStrings(title1, title2, sortOrder);
            if (titleComparison != 0) return titleComparison;

            auto artist1 = track1.artist();
            auto artist2 = track2.artist();
            int artistComparison = compareStrings(artist1, artist2, sortOrder);
            if (artistComparison != 0) return artistComparison;
        }

        return Comparisons::compare(
            track1.hash(), track2.hash(),
            std::function<int(FileHash const&, FileHash const&)>(&PMP::compare),
            sortOrder
        );
    }

    int SortedCollectionTableModel::compareArtists(const CollectionTrackInfo& track1,
                                                   const CollectionTrackInfo& track2,
                                                   Qt::SortOrder sortOrder) const
    {
        bool empty1 = track1.titleAndArtistUnknown();
        bool empty2 = track2.titleAndArtistUnknown();

        if (empty1 || empty2) {
            if (!empty1) return -1; /* track 1 goes first */
            if (!empty2) return 1; /* track 2 goes first */

            /* both are empty; compare other properties */
        }
        else {
            auto artist1 = track1.artist();
            auto artist2 = track2.artist();
            int artistComparison = compareStrings(artist1, artist2, sortOrder);
            if (artistComparison != 0) return artistComparison;

            auto title1 = track1.title();
            auto title2 = track2.title();
            int titleComparison = compareStrings(title1, title2, sortOrder);
            if (titleComparison != 0) return titleComparison;
        }

        return PMP::compare(track1.hash(), track2.hash());
    }

    int SortedCollectionTableModel::compareLengths(const CollectionTrackInfo& track1,
                                                   const CollectionTrackInfo& track2,
                                                   Qt::SortOrder sortOrder) const
    {
        auto length1 = track1.lengthInMilliseconds();
        auto length2 = track2.lengthInMilliseconds();

        if (length1 < 0 || length2 < 0) {
            if (length1 >= 0) return -1; /* track 1 goes first */
            if (length2 >= 0) return 1; /* track 2 goes first */

            /* both are empty; compare other properties */
        }
        else {
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

        return PMP::compare(track1.hash(), track2.hash());
    }

    int SortedCollectionTableModel::compareAlbums(const CollectionTrackInfo& track1,
                                                  const CollectionTrackInfo& track2,
                                                  Qt::SortOrder sortOrder) const
    {
        auto album1 = track1.album();
        auto album2 = track2.album();

        bool noAlbum1 = album1.isEmpty();
        bool noAlbum2 = album2.isEmpty();

        if (noAlbum1 || noAlbum2) {
            if (!noAlbum1) return -1; /* track 1 goes first */
            if (!noAlbum2) return 1; /* track 2 goes first */

            /* both are empty; compare other properties */
        }
        else {
            int comparison = compareStrings(album1, album2, sortOrder);
            if (comparison != 0) return comparison;
        }

        auto title1 = track1.title();
        auto title2 = track2.title();
        int titleComparison = compareStrings(title1, title2, sortOrder);
        if (titleComparison != 0) return titleComparison;

        return PMP::compare(track1.hash(), track2.hash());
    }

    void SortedCollectionTableModel::onNewTrackReceived(CollectionTrackInfo track)
    {
        addOrUpdateTrack(track);
    }

    void SortedCollectionTableModel::onTrackAvailabilityChanged(FileHash hash,
                                                                bool isAvailable)
    {
        updateTrackAvailability(hash, isAvailable);
    }

    void SortedCollectionTableModel::onTrackDataChanged(CollectionTrackInfo track)
    {
        addOrUpdateTrack(track);
    }

    void SortedCollectionTableModel::onUserTrackDataChanged(quint32 userId, FileHash hash)
    {
        Q_UNUSED(userId)
        /* ignore the user ID for change notifications */

        auto innerIndex = _hashesToInnerIndexes.value(hash, -1);
        if (innerIndex < 0)
            return; /* track is not in the list */

        auto outerIndex = _innerToOuterIndexMap[innerIndex];

        Q_EMIT dataChanged(createIndex(outerIndex, 0),
                           createIndex(outerIndex, 4 - 1));
    }

    void SortedCollectionTableModel::currentTrackInfoChanged(FileHash hash)
    {
        if (_currentTrackHash == hash)
            return;

        _currentTrackHash = hash;
        markLeftColumnAsChanged();
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

        if (searchRangeEnd - searchRangeBegin <= 50) { /* do linear search */
            for (int index = searchRangeBegin; index < searchRangeEnd; ++index) {
                auto const& current = *_tracks.at(_outerToInnerIndexMap.at(index));

                if (lessThan(track, current)) return index;
            }

            return searchRangeEnd;
        }

        /* binary search */

        int middleIndex = searchRangeBegin / 2 + searchRangeEnd / 2;
        auto const& middleTrack = *_tracks.at(_outerToInnerIndexMap.at(middleIndex));

        if (lessThan(track, middleTrack)) {
            return findOuterIndexMapIndexForInsert(track, searchRangeBegin, middleIndex);
        }
        else {
            return findOuterIndexMapIndexForInsert(track, middleIndex, searchRangeEnd);
        }
    }

    void SortedCollectionTableModel::markLeftColumnAsChanged()
    {
        Q_EMIT dataChanged(
            createIndex(0, 0), createIndex(rowCount() - 1, 0)
        );
    }

    void SortedCollectionTableModel::updateTrackAvailability(FileHash hash,
                                                             bool isAvailable)
    {
        auto hashIterator = _hashesToInnerIndexes.find(hash);
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
        QHash<FileHash, int> hashIndexer;
        trackList.reserve(trackCollection.size());
        hashIndexer.reserve(trackCollection.size());

        for (int i = 0; i < trackCollection.size(); ++i) {
            CollectionTrackInfo const& track = trackCollection[i];

            if (hashIndexer.contains(track.hash()))
                continue; /* already present */

            if (!track.isAvailable() && track.titleAndArtistUnknown())
                continue; /* not interesting enough to add */

            auto trackObj = new CollectionTrackInfo(track);
            hashIndexer.insert(track.hash(), trackList.size());
            trackList.append(trackObj);
        }

        qDebug() << " addWhenModelEmpty: inserting" << trackList.size() << "tracks";

        if (!trackList.empty()) {
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
        auto hashIterator = _hashesToInnerIndexes.find(track.hash());
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
        _hashesToInnerIndexes.insert(track.hash(), innerIndex);
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
                   << "; hash:" << newTrackData.hash();

        int oldOuterIndex = _innerToOuterIndexMap[innerIndex];
        int newOuterIndex = findOuterIndexMapIndexForInsert(newTrackData);

        /* moving the track after itself means we need to subtract one from the
           destination index */
        if (newOuterIndex > oldOuterIndex)
            newOuterIndex--;

        if (newOuterIndex != oldOuterIndex)
        {
            beginMoveRows(QModelIndex(), oldOuterIndex, oldOuterIndex,
                          QModelIndex(), newOuterIndex);

            _outerToInnerIndexMap.move(oldOuterIndex, newOuterIndex);

            /* elements between the old and new index got a new outer index;
               update the inner to outer map to reflect this */
            rebuildInnerMap(qMin(oldOuterIndex, newOuterIndex),
                            qMax(oldOuterIndex, newOuterIndex) + 1);

            endMoveRows();
        }

        track = newTrackData;

        Q_EMIT dataChanged(createIndex(newOuterIndex, 0),
                           createIndex(newOuterIndex, 4 - 1));
    }

    void SortedCollectionTableModel::sort(int column, Qt::SortOrder order) {
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

    void SortedCollectionTableModel::markEverythingAsChanged() {
        Q_EMIT dataChanged(
            createIndex(0, 0), createIndex(rowCount() - 1, columnCount() - 1)
        );
    }

    void SortedCollectionTableModel::buildIndexMaps() {
        /* generate unsorted maps */
        _innerToOuterIndexMap.resize(_tracks.size());
        _outerToInnerIndexMap.resize(_tracks.size());
        for (int i = 0; i < _tracks.size(); ++i) {
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

    CollectionTrackInfo* SortedCollectionTableModel::trackAt(
                                                           const QModelIndex& index) const
    {
        return trackAt(index.row());
    }

    CollectionTrackInfo* SortedCollectionTableModel::trackAt(int rowIndex) const {
        if (rowIndex < 0 || rowIndex >= _tracks.size()) { return nullptr; }

        return _tracks.at(_outerToInnerIndexMap.at(rowIndex));
    }

    int SortedCollectionTableModel::rowCount(const QModelIndex& parent) const {
        Q_UNUSED(parent)
        return _outerToInnerIndexMap.size();
    }

    int SortedCollectionTableModel::columnCount(const QModelIndex& parent) const {
        Q_UNUSED(parent)
        return 4;
    }

    QVariant SortedCollectionTableModel::headerData(int section,
                                                    Qt::Orientation orientation,
                                                    int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            switch (section) {
                case 0: return QString(tr("Title"));
                case 1: return QString(tr("Artist"));
                case 2: return QString(tr("Length"));
                case 3: return QString(tr("Album"));
            }
        }

        return QVariant();
    }

    QVariant SortedCollectionTableModel::data(const QModelIndex& index, int role) const {
        switch (role) {
            case Qt::TextAlignmentRole:
                switch (index.column()) {
                    case 2: return Qt::AlignRight + Qt::AlignVCenter;
                }
                break;
            case Qt::DisplayRole:
                if (index.row() < _tracks.size()) {
                    auto track = trackAt(index);

                    switch (index.column()) {
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
                if (index.column() == 0 && index.row() < _tracks.size()) {
                    auto track = trackAt(index);

                    if (track->hash() == _currentTrackHash) {
                        switch (_playerState) {
                            case PlayerState::Playing:
                                return QIcon(":/mediabuttons/Play.png");

                            case PlayerState::Paused:
                                return QIcon(":/mediabuttons/Pause.png");

                            default:
                                break;
                        }
                    }
                }
                break;
            case Qt::ForegroundRole:
                if (index.row() < _tracks.size()) {
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
                           _highlightingTrackJudge.trackSatisfiesCriterium(*track, false);

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

    Qt::ItemFlags SortedCollectionTableModel::flags(const QModelIndex& index) const {
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

            auto& hash = trackAt(row)->hash();
            qDebug() << " row" << row << "; col" << index.column()
                     << "; hash" << hash.dumpToString();
            hashes.append(hash);
        }

        if (hashes.empty()) return nullptr;

        stream << quint32(hashes.size());
        for (int i = 0; i < hashes.size(); ++i) {
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
                                             ClientServerInterface* clientServerInterface,
                                             CollectionViewContext* collectionViewContext)
     : _source(source),
       _filteringTrackJudge(clientServerInterface->userDataFetcher())
    {
        Q_UNUSED(parent)
        setFilterCaseSensitivity(Qt::CaseInsensitive);

        _filteringTrackJudge.setUserId(collectionViewContext->userId());
        connect(
            collectionViewContext, &CollectionViewContext::userIdChanged,
            this,
            [this, collectionViewContext]()
            {
                _filteringTrackJudge.setUserId(collectionViewContext->userId());
                invalidateFilter();
            }
        );

        setSourceModel(source);
    }

    void FilteredCollectionTableModel::setTrackFilter(TrackCriterium criterium)
    {
        _filteringTrackJudge.setCriterium(criterium);
        invalidateFilter();
    }

    void FilteredCollectionTableModel::sort(int column, Qt::SortOrder order)
    {
        _source->sort(column, order);
    }

    CollectionTrackInfo* FilteredCollectionTableModel::trackAt(
                                                           const QModelIndex& index) const
    {
        return _source->trackAt(mapToSource(index));
    }

    void FilteredCollectionTableModel::setSearchText(QString search)
    {
        _searchParts = search.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        invalidateFilter();
    }

    bool FilteredCollectionTableModel::filterAcceptsRow(int sourceRow,
                                                    const QModelIndex& sourceParent) const
    {
        (void)sourceParent;

        if (_searchParts.empty()
                && _filteringTrackJudge.getCriterium() == TrackCriterium::None)
        {
            return true; /* not filtered */
        }

        CollectionTrackInfo* track = _source->trackAt(sourceRow);

        if (!_searchParts.empty())
        {
            for (QString const& searchPart : _searchParts)
            {
                if (!track->title().contains(searchPart, Qt::CaseInsensitive)
                        && !track->artist().contains(searchPart, Qt::CaseInsensitive)
                        && !track->album().contains(searchPart, Qt::CaseInsensitive))
                    return false;
            }
        }

        return _filteringTrackJudge.trackSatisfiesCriterium(*track, true).isTrue();
    }

}

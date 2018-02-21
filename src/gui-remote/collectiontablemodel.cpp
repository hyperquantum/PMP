/*
    Copyright (C) 2016-2018, Kevin Andre <hyperquantum@gmail.com>

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

#include <QBrush>
#include <QBuffer>
#include <QDataStream>
#include <QMimeData>
#include <QtDebug>
#include <QVector>

#include <algorithm>
#include <functional>

namespace PMP {

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

    SortedCollectionTableModel::SortedCollectionTableModel(QObject* parent)
     : QAbstractTableModel(parent), _sortBy(0), _sortOrder(Qt::AscendingOrder)
    {
        _collator.setCaseSensitivity(Qt::CaseInsensitive);
        _collator.setNumericMode(true);

        /* we need to ignore symbols such as quotes, spaces and parentheses */
        _collator.setIgnorePunctuation(true);
    }

    void SortedCollectionTableModel::setConnection(ServerConnection* connection) {
        connect(
            connection, &ServerConnection::collectionTracksChanged,
            this, &SortedCollectionTableModel::onCollectionTracksChanged
        );

        auto fetcher = new CollectionTableFetcher(this);
        connection->fetchCollection(fetcher);
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

    void SortedCollectionTableModel::addOrUpdateTracks(QList<CollectionTrackInfo> tracks)
    {
        qDebug() << "addOrUpdateTracks called for" << tracks.size() << "tracks";

        if (_hashesToInnerIndexes.empty()) {
            addWhenModelEmpty(tracks);
            return;
        }

        Q_FOREACH(CollectionTrackInfo const& track, tracks) {
            /* hash already present? */
            auto hashIterator = _hashesToInnerIndexes.find(track.hash());
            if (hashIterator != _hashesToInnerIndexes.end()) {
                // TODO: update
                continue;
            }

            if (!track.isAvailable() && track.titleAndArtistUnknown())
                continue; /* not interesting enough to add */

            int indexToInsertAt =
                findOuterIndexMapIndexForInsert(track, 0, _outerToInnerIndexMap.size());

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
    }

    void SortedCollectionTableModel::onCollectionTracksChanged(
                                                       QList<CollectionTrackInfo> changes)
    {
        addOrUpdateTracks(changes);
    }

    int SortedCollectionTableModel::findOuterIndexMapIndexForInsert(
            const CollectionTrackInfo& track,
            int searchRangeBegin, int searchRangeEnd /* end is not part of the range */)
    {
        if (searchRangeBegin >= searchRangeEnd) return -1 /* problem */;

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

    void SortedCollectionTableModel::addWhenModelEmpty(QList<CollectionTrackInfo> tracks)
    {
        QVector<CollectionTrackInfo*> trackList;
        QHash<FileHash, int> hashIndexer;
        trackList.reserve(tracks.size());
        hashIndexer.reserve(tracks.size());

        Q_FOREACH(CollectionTrackInfo const& track, tracks) {
            if (hashIndexer.contains(track.hash()))
                continue; /* already present */

            if (!track.isAvailable() && track.titleAndArtistUnknown())
                continue; /* not interesting enough to add */

            auto trackObj = new CollectionTrackInfo(track);
            hashIndexer.insert(track.hash(), trackList.size());
            trackList.append(trackObj);
        }

        qDebug() << " addFirstTime: inserting" << trackList.size() << "tracks";

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

        emit dataChanged(
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

    void SortedCollectionTableModel::rebuildInnerMap(int outerStartIndex) {
        /* (re)construct inner map from outer map */
        for (int i = outerStartIndex; i < _outerToInnerIndexMap.size(); ++i) {
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
        return _outerToInnerIndexMap.size();
    }

    int SortedCollectionTableModel::columnCount(const QModelIndex& parent) const {
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
                    switch (index.column()) {
                        case 0: return trackAt(index)->title();
                        case 1: return trackAt(index)->artist();
                        case 2:
                        {
                            int lengthInSeconds = trackAt(index)->lengthInSeconds();

                            if (lengthInSeconds < 0) { return "?"; }
                            return Util::secondsToHoursMinuteSecondsText(lengthInSeconds);
                        }
                        case 3: return trackAt(index)->album();
                    }
                }
                break;
            case Qt::ForegroundRole:
                if (index.row() < _tracks.size()) {
                    if (!trackAt(index)->isAvailable()) return QBrush(Qt::gray);
                }
                break;
        }

        return QVariant();
    }

    Qt::ItemFlags SortedCollectionTableModel::flags(const QModelIndex& index) const {
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
        Q_FOREACH(const QModelIndex& index, indexes) {
            int row = index.row();
            if (row == prevRow) continue;
            prevRow = row;

            auto& hash = trackAt(row)->hash();
            qDebug() << " row" << row << "; col" << index.column()
                     << "; hash" << hash.dumpToString();
            hashes.append(hash);
        }

        if (hashes.empty()) return nullptr;

        stream << (quint32)hashes.size();
        for (int i = 0; i < hashes.size(); ++i) {
            stream << (quint64)hashes[i].length();
            stream << hashes[i].SHA1();
            stream << hashes[i].MD5();
        }

        buffer.close();

        QMimeData* data = new QMimeData();

        data->setData("application/x-pmp-filehash", buffer.data());
        return data;
    }

    // ============================================================================ //

    FilteredCollectionTableModel::FilteredCollectionTableModel(
            SortedCollectionTableModel* source, QObject* parent)
     : _source(source)
    {
        setFilterCaseSensitivity(Qt::CaseInsensitive);

        setSourceModel(source);
    }

    void FilteredCollectionTableModel::sort(int column, Qt::SortOrder order) {
        _source->sort(column, order);
    }

    CollectionTrackInfo* FilteredCollectionTableModel::trackAt(
                                                           const QModelIndex& index) const
    {
        return _source->trackAt(mapToSource(index));
    }

    void FilteredCollectionTableModel::setSearchText(QString search) {
        _searchParts = search.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        invalidateFilter();
    }

    bool FilteredCollectionTableModel::filterAcceptsRow(int sourceRow,
                                                    const QModelIndex& sourceParent) const
    {
        if (_searchParts.empty()) return true; /* not filtered */

        CollectionTrackInfo* track = _source->trackAt(sourceRow);

        Q_FOREACH(QString searchPart, _searchParts) {
            if (!track->title().contains(searchPart, Qt::CaseInsensitive)
                    && !track->artist().contains(searchPart, Qt::CaseInsensitive))
                return false;
        }

        return true;
    }

    // ============================================================================ //

    CollectionTableFetcher::CollectionTableFetcher(SortedCollectionTableModel* parent)
     : AbstractCollectionFetcher(parent), _model(parent)//, _tracksReceivedCount(0)
    {
        //
    }

    void CollectionTableFetcher::receivedData(QList<CollectionTrackInfo> data) {
        //_tracksReceivedCount += data.size();
        _tracksReceived += data;
        //_model->addFirstTime(data);
    }

    void CollectionTableFetcher::completed() {
        qDebug() << "CollectionTableFetcher: fetch completed.  Tracks received:"
                 << _tracksReceived.size(); //_tracksReceivedCount;
        _model->addOrUpdateTracks(_tracksReceived);
        this->deleteLater();
    }

    void CollectionTableFetcher::errorOccurred() {
        qDebug() << "CollectionTableFetcher::errorOccurred() called!";
        // TODO
    }
}

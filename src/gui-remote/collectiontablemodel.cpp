/*
    Copyright (C) 2016-2017, Kevin Andre <hyperquantum@gmail.com>

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

#include <QBuffer>
#include <QDataStream>
#include <QMimeData>
#include <QtDebug>
#include <QVector>

namespace PMP {

    CollectionTableModel::CollectionTableModel(QObject* parent)
     : QAbstractTableModel(parent)
    {
        //
    }

    void CollectionTableModel::setConnection(ServerConnection* connection) {
        connect(
            connection, &ServerConnection::collectionTracksChanged,
            this, &CollectionTableModel::onCollectionTracksChanged
        );

        auto fetcher = new CollectionTableFetcher(this);
        connection->fetchCollection(fetcher);
    }

    void CollectionTableModel::addFirstTime(QList<CollectionTrackInfo> tracks) {
        qDebug() << "CollectionTableModel::addFirstTime called for" << tracks.size()
                 << "tracks";

        if (_hashes.empty()) { /* fast path: insert all at once */
            QHash<FileHash, CollectionTrackInfo*> hashes;
            hashes.reserve(tracks.size());

            Q_FOREACH(CollectionTrackInfo const& track, tracks) {
                if (hashes.contains(track.hash()))
                    continue; /* already present */

                if (!track.isAvailable() && track.titleAndArtistUnknown())
                    continue; /* not interesting enough to add */

                auto trackObj = new CollectionTrackInfo(track);
                hashes.insert(track.hash(), trackObj);
            }

            auto tracks = hashes.values();

            qDebug() << " addFirstTime: inserting" << tracks.size() << "tracks";

            if (!tracks.empty()) {
                beginInsertRows(QModelIndex(), 0, tracks.size() - 1);
                _hashes = hashes;
                _tracks = tracks;
                endInsertRows();
            }

            return;
        }

        qDebug() << " addFirstTime: will have to insert/update one by one";

        Q_FOREACH(CollectionTrackInfo const& track, tracks) {
            if (_hashes.contains(track.hash()))
                continue; /* already present */

            if (!track.isAvailable() && track.titleAndArtistUnknown())
                continue; /* not interesting enough to add */

            int index = tracks.size();
            auto trackObj = new CollectionTrackInfo(track);

            beginInsertRows(QModelIndex(), index, index);
            _hashes.insert(track.hash(), trackObj);
            _tracks.insert(index, trackObj);
            endInsertRows();
        }
    }

    void CollectionTableModel::onCollectionTracksChanged(
                                                       QList<CollectionTrackInfo> changes)
    {
        qDebug() << "CollectionTableModel: got tracks changed event; count:"
                 << changes.size();

        Q_FOREACH(auto const& track, changes) {
            if (!_hashes.contains(track.hash())) {
                /* add only if interesting enough */
                if (!track.isAvailable() && track.titleAndArtistUnknown())
                    continue;

                /* add new item */

                int index = _tracks.size();
                auto trackObj = new CollectionTrackInfo(track);

                beginInsertRows(QModelIndex(), index, index);
                _hashes.insert(track.hash(), trackObj);
                _tracks.insert(index, trackObj);
                endInsertRows();
                continue;
            }

            /* update an existing item */

            // TODO : update an existing item

//            auto const* existing = _hashes[track.hash()];
//            if (track == *existing) continue; /* nothing changed for this track */

//            qDebug() << "CollectionTableModel: updating existing track: "
//                     << track.title() << "-" << track.artist();

//            int oldIndex = findIndexOf(*existing);
//            if (oldIndex < 0) {
//                qWarning() << "  OOPS: index not found of track to update!!";
//                continue;
//            }
//            auto trackObj = new CollectionTrackInfo(track);
//            int newIndex = findInsertIndexFor(*trackObj);

//            if (oldIndex != newIndex) {
//                /* the track is moved from the old to the new position */

//                /* the to-index has a slightly different meaning for Qt, so we need
//                 *  adjustment */
//                int qtToIndex = (oldIndex < newIndex) ? newIndex + 1 : newIndex;
//                beginMoveRows(QModelIndex(), oldIndex, oldIndex, QModelIndex(), qtToIndex);
//                _tracks.removeAt(oldIndex);
//                _tracks.insert(newIndex, trackObj);
//                _hashes[track.hash()] = trackObj;
//                endMoveRows();
//            }
//            else {
//                _hashes[track.hash()] = trackObj;
//                // TODO: how to notify the view that the row has changed?
//            }

//            delete existing;
        }
    }

    CollectionTrackInfo* CollectionTableModel::trackAt(const QModelIndex& index) const {
        return trackAt(index.row());
    }

    CollectionTrackInfo* CollectionTableModel::trackAt(int rowIndex) const {
        if (rowIndex < 0 || rowIndex >= _tracks.size()) {
            return nullptr;
        }

        return _tracks[rowIndex];
    }

    int CollectionTableModel::rowCount(const QModelIndex& parent) const {
        //qDebug() << "CollectionTableModel::rowCount returning" << _modelRows;
        return _tracks.size();
    }

    int CollectionTableModel::columnCount(const QModelIndex& parent) const {
        return 2;
    }

    QVariant CollectionTableModel::headerData(int section, Qt::Orientation orientation,
                                              int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            switch (section) {
                case 0: return QString(tr("Title"));
                case 1: return QString(tr("Artist"));
            }
        }

        return QVariant();
    }

    QVariant CollectionTableModel::data(const QModelIndex& index, int role) const {
        //qDebug() << "CollectionTableModel::data called with role" << role;

        if (role == Qt::DisplayRole && index.row() < _tracks.size()) {
            int col = index.column();
            switch (col) {
                case 0: return _tracks[index.row()]->title();
                case 1: return _tracks[index.row()]->artist();
            }
        }

        return QVariant();
    }

    Qt::ItemFlags CollectionTableModel::flags(const QModelIndex& index) const {
        Qt::ItemFlags f(Qt::ItemIsSelectable | Qt::ItemIsEnabled
                        | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        return f;
    }

    Qt::DropActions CollectionTableModel::supportedDragActions() const
    {
        return Qt::CopyAction;
    }

    Qt::DropActions CollectionTableModel::supportedDropActions() const
    {
        return Qt::CopyAction;
    }

    QMimeData* CollectionTableModel::mimeData(const QModelIndexList& indexes) const {
        if (indexes.isEmpty()) return 0;

        qDebug() << "CollectionTableModel::mimeData called; indexes count ="
                 << indexes.size();

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

            auto& hash = _tracks[row]->hash();
            qDebug() << " row" << row << "; col" << index.column()
                     << "; hash" << hash.dumpToString();
            hashes.append(hash);
        }

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

    SortedFilteredCollectionTableModel::SortedFilteredCollectionTableModel(
            CollectionTableModel* source, QObject* parent)
     : _source(source)
    {
        setFilterCaseSensitivity(Qt::CaseInsensitive);

        _collator.setCaseSensitivity(Qt::CaseInsensitive);
        _collator.setNumericMode(true);

        /* we need to ignore symbols such as quotes, spaces and parentheses */
        _collator.setIgnorePunctuation(true);

        setSourceModel(source);
    }

    CollectionTrackInfo* SortedFilteredCollectionTableModel::trackAt(
                                                           const QModelIndex& index) const
    {
        return _source->trackAt(mapToSource(index));
    }

    void SortedFilteredCollectionTableModel::sortByTitle() {
        sort(0);
    }

    void SortedFilteredCollectionTableModel::sortByArtist() {
        sort(1);
    }

    void SortedFilteredCollectionTableModel::setSearchText(QString search) {
        _searchParts = search.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        invalidateFilter();
    }

    bool SortedFilteredCollectionTableModel::lessThan(const QModelIndex& left,
                                              const QModelIndex& right) const
    {
        CollectionTrackInfo* leftTrack = _source->trackAt(left);
        CollectionTrackInfo* rightTrack = _source->trackAt(right);

        switch (left.column()) {
            case 1:
                return compareArtists(*leftTrack, *rightTrack) < 0;
            case 0:
            default:
                return compareTitles(*leftTrack, *rightTrack) < 0;
        }
    }

    bool SortedFilteredCollectionTableModel::filterAcceptsRow(int sourceRow,
                                                    const QModelIndex &sourceParent) const
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

    int SortedFilteredCollectionTableModel::compareTitles(
            const CollectionTrackInfo &track1, const CollectionTrackInfo &track2) const
    {
        bool empty1 = track1.titleAndArtistUnknown();
        bool empty2 = track2.titleAndArtistUnknown();

        if (empty1 || empty2) {
            if (!empty1) {
                return -1; /* track 1 goes first */
            }
            else if (!empty2) {
                return 1; /* track 2 goes first */
            }

            /* both are empty */

            return PMP::compare(track1.hash(), track2.hash());
        }

        QString title1 = track1.title();
        QString title2 = track2.title();
        int titleComparison = _collator.compare(title1, title2);
        if (titleComparison != 0) return titleComparison;

        QString artist1 = track1.artist();
        QString artist2 = track2.artist();
        int artistComparison = _collator.compare(artist1, artist2);
        if (artistComparison != 0) return artistComparison;

        return PMP::compare(track1.hash(), track2.hash());
    }

    int SortedFilteredCollectionTableModel::compareArtists(
            const CollectionTrackInfo &track1, const CollectionTrackInfo &track2) const
    {
        bool empty1 = track1.titleAndArtistUnknown();
        bool empty2 = track2.titleAndArtistUnknown();

        if (empty1 || empty2) {
            if (!empty1) {
                return -1; /* track 1 goes first */
            }
            else if (!empty2) {
                return 1; /* track 2 goes first */
            }

            /* both are empty */

            return PMP::compare(track1.hash(), track2.hash());
        }

        QString artist1 = track1.artist();
        QString artist2 = track2.artist();
        int artistComparison = _collator.compare(artist1, artist2);
        if (artistComparison != 0) return artistComparison;

        QString title1 = track1.title();
        QString title2 = track2.title();
        int titleComparison = _collator.compare(title1, title2);
        if (titleComparison != 0) return titleComparison;

        return PMP::compare(track1.hash(), track2.hash());
    }

    // ============================================================================ //

    CollectionTableFetcher::CollectionTableFetcher(CollectionTableModel* parent)
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
        _model->addFirstTime(_tracksReceived);
        this->deleteLater();
    }

    void CollectionTableFetcher::errorOccurred() {
        qDebug() << "CollectionTableFetcher::errorOccurred() called!";
        // TODO
    }

}

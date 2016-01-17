/*
    Copyright (C) 2016, Kevin Andre <hyperquantum@gmail.com>

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

#include <QtDebug>

#include <algorithm>

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
            // TODO: put sort into another thread
            std::sort(
                tracks.begin(),
                tracks.end(),
                [=](CollectionTrackInfo* t1, CollectionTrackInfo* t2) {
                    return compareLessThan(t1, t2);
                }
            );

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

            int index = findInsertIndexFor(track);
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

                int index = findInsertIndexFor(track);
                auto trackObj = new CollectionTrackInfo(track);

                beginInsertRows(QModelIndex(), index, index);
                _hashes.insert(track.hash(), trackObj);
                _tracks.insert(index, trackObj);
                endInsertRows();
                continue;
            }

            /* update an existing item */

            auto const* existing = _hashes[track.hash()];
            if (track == *existing) continue; /* nothing changed for this track */

            qDebug() << "CollectionTableModel: updating existing track: "
                     << track.title() << "-" << track.artist();

            int oldIndex = findIndexOf(*existing);
            if (oldIndex < 0) {
                qDebug() << "  OOPS: index not found of track to update!!";
                continue;
            }
            auto trackObj = new CollectionTrackInfo(track);
            int newIndex = findInsertIndexFor(*trackObj);

            if (oldIndex != newIndex) {
                /* the track is moved from the old to the new position */

                /* the to-index has a slightly different meaning for Qt, so we need
                 *  adjustment */
                int qtToIndex = (oldIndex < newIndex) ? newIndex + 1 : newIndex;
                beginMoveRows(QModelIndex(), oldIndex, oldIndex, QModelIndex(), qtToIndex);
                _tracks.removeAt(oldIndex);
                _tracks.insert(newIndex, trackObj);
                _hashes[track.hash()] = trackObj;
                endMoveRows();
            }
            else {
                _hashes[track.hash()] = trackObj;
                // TODO: how to notify the view that the row has changed?
            }

            delete existing;
        }
    }

    CollectionTrackInfo* CollectionTableModel::trackAt(const QModelIndex& index) const {
        int row = index.row();
        if (row < 0 || row >= _tracks.size()) {
            return nullptr;
        }

        return _tracks[row];
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

    int CollectionTableModel::findInsertIndexFor(const CollectionTrackInfo& track) const {
        if (_tracks.empty()) return 0;

        int lower = 0;
        int upper = _tracks.size() - 1;
        if (compare(*_tracks[upper], track) < 0) return upper + 1;
        if (compare(track, *_tracks[0]) < 0) return 0;

        /* binary search */
        while (upper - lower > 5) {
            int middle = lower / 2 + upper / 2;

            int comparison = compare(track, *_tracks[middle]);
            if (comparison < 0) {
                upper = middle;
            }
            else if (comparison > 0) {
                lower = middle;
            }
            else return middle;
        }

        /* last part: linear search */
        for (int i = lower; i < upper; ++i) {
            int comparison = compare(track, *_tracks[i]);
            if (comparison <= 0) return i;
        }

        return upper;
    }

    int CollectionTableModel::findIndexOf(const CollectionTrackInfo& track) const {
        int index = findInsertIndexFor(track);
        if (index >= _tracks.size()) return -1; /* not found */

        if (track.hash() == _tracks[index]->hash()) return index;

        return -1;
    }

    int CollectionTableModel::compare(const CollectionTrackInfo &track1,
                                      const CollectionTrackInfo &track2)
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
        int titleComparison = title1.localeAwareCompare(title2);
        if (titleComparison != 0) return titleComparison;

        QString artist1 = track1.artist();
        QString artist2 = track2.artist();
        int artistComparison = artist1.localeAwareCompare(artist2);
        if (artistComparison != 0) return artistComparison;

        return PMP::compare(track1.hash(), track2.hash());
    }

    bool CollectionTableModel::compareLessThan(const CollectionTrackInfo* track1,
                                               const CollectionTrackInfo* track2)
    {
        return compare(*track1, *track2) < 0;
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

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

namespace PMP {

    CollectionTableModel::CollectionTableModel(QObject* parent)
     : QAbstractTableModel(parent)
    {
        //
    }

    void CollectionTableModel::setConnection(ServerConnection* connection) {
        auto fetcher = new CollectionTableFetcher(this);
        connection->fetchCollection(fetcher);
    }

    void CollectionTableModel::addFirstTime(QList<CollectionTrackInfo> tracks) {
        Q_FOREACH(auto track, tracks) {
            if (_hashes.contains(track.hash()))
                continue; /* already present */

            int index = findInsertIndexFor(track);
            auto trackObj = new CollectionTrackInfo(track);

            beginInsertRows(QModelIndex(), index, index);
            _hashes.insert(track.hash(), trackObj);
            _tracks.insert(index, trackObj);
            endInsertRows();
        }
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

        return _tracks.size(); /* FIXME: sorting */



    }

    /*int CollectionTableModel::getOrder(const CollectionTrackInfo& track1,
                                       const CollectionTrackInfo& track2) const
    {
        QString title1 = track1.title();
        QString title2 = track2.title();

        if (title1 < title2) return -1;
        if (title1 < title2) return -1;



    }*/

    // ============================================================================ //

    CollectionTableFetcher::CollectionTableFetcher(CollectionTableModel* parent)
     : AbstractCollectionFetcher(parent), _model(parent), _tracksReceivedCount(0)
    {
        //
    }

    void CollectionTableFetcher::receivedData(QList<CollectionTrackInfo> data) {
        _tracksReceivedCount += data.size();
        _model->addFirstTime(data);
    }

    void CollectionTableFetcher::completed() {
        qDebug() << "CollectionTableFetcher: fetch completed.  Tracks received:"
                 << _tracksReceivedCount;
        this->deleteLater();
    }

    void CollectionTableFetcher::errorOccurred() {
        qDebug() << "CollectionTableFetcher::errorOccurred() called!";
        // TODO
    }

}

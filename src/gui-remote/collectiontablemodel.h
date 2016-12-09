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

#ifndef PMP_COLLECTIONTABLEMODEL_H
#define PMP_COLLECTIONTABLEMODEL_H

#include "common/serverconnection.h"

#include <QAbstractTableModel>
#include <QCollator>
#include <QHash>
#include <QList>
#include <QSortFilterProxyModel>

namespace PMP {

    class CollectionTableModel : public QAbstractTableModel {
        Q_OBJECT
    public:
        CollectionTableModel(QObject* parent = 0);

        void setConnection(ServerConnection* connection);

        void addFirstTime(QList<CollectionTrackInfo> tracks);

        CollectionTrackInfo* trackAt(const QModelIndex& index) const;
        CollectionTrackInfo* trackAt(int rowIndex) const;

        int rowCount(const QModelIndex& parent = QModelIndex()) const;
        int columnCount(const QModelIndex& parent = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    private slots:
        void onCollectionTracksChanged(QList<PMP::CollectionTrackInfo> changes);

    private:
        QHash<FileHash, CollectionTrackInfo*> _hashes;
        QList<CollectionTrackInfo*> _tracks;
    };

    class SortedCollectionTableModel : public QSortFilterProxyModel {
        Q_OBJECT
    public:
        SortedCollectionTableModel(CollectionTableModel* source, QObject* parent = 0);

        CollectionTrackInfo* trackAt(const QModelIndex& index) const;

        void sortByTitle();
        void sortByArtist();

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    private:
        int compareTitles(const CollectionTrackInfo& track1,
                          const CollectionTrackInfo& track2) const;

        int compareArtists(const CollectionTrackInfo& track1,
                           const CollectionTrackInfo& track2) const;

        CollectionTableModel* _source;
        QCollator _collator;
    };

    class CollectionTableFetcher : public AbstractCollectionFetcher {
        Q_OBJECT
    public:
        CollectionTableFetcher(CollectionTableModel* parent);

    public slots:
        void receivedData(QList<CollectionTrackInfo> data);
        void completed();
        void errorOccurred();

    private:
        CollectionTableModel* _model;
        //uint _tracksReceivedCount;
        QList<CollectionTrackInfo> _tracksReceived;
    };

}
#endif

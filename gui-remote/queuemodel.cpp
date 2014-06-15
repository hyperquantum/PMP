/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#include "queuemodel.h"

#include "queuemonitor.h"

#include <QtDebug>

namespace PMP {

    QueueModel::QueueModel(QObject* parent, QueueMonitor* source)
     : QAbstractTableModel(parent), _source(source), _modelRows(0)
    {
        _modelRows = _source->queueLength();

        connect(_source, SIGNAL(tracksInserted(int, int)), this, SLOT(tracksInserted(int, int)));
        connect(_source, SIGNAL(tracksRemoved(int, int)), this, SLOT(tracksRemoved(int, int)));
        connect(_source, SIGNAL(tracksChanged(int, int)), this, SLOT(tracksChanged(int, int)));
    }

    int QueueModel::rowCount(const QModelIndex& parent) const {
        int rows = _modelRows;
        qDebug() << "QueueModel::rowCount returning" << rows;
        return rows;
    }

    int QueueModel::columnCount(const QModelIndex& parent) const {
        return 3;
    }

    QVariant QueueModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (role == Qt::DisplayRole) {
            if (orientation == Qt::Horizontal) {
                switch (section) {
                    case 0: return QString("Title");
                    case 1: return QString("Artist");
                    case 2: return QString("Length");
                }
            }
            else if (orientation == Qt::Vertical) {
                return section + 1;
            }
        }

        return QVariant();
    }

    QVariant QueueModel::data(const QModelIndex& index, int role) const {
        if (role == Qt::DisplayRole) {
            int row = index.row();
            quint32 queueID = _source->queueEntry(row);
            if (queueID == 0) { return QString("?"); }

            TrackMonitor* track = _source->trackFromID(queueID);
            if (track == 0) { return QString("?"); }

            int col = index.column();
            switch (col) {
                case 0: return track->title();
                case 1: return track->artist();
                case 2:
                {
                    int lengthInSeconds = track->lengthInSeconds();

                    if (lengthInSeconds < 0) { return "?"; }

                    int sec = lengthInSeconds % 60;
                    int min = (lengthInSeconds / 60) % 60;
                    int hrs = lengthInSeconds / 3600;

                    return QString::number(hrs).rightJustified(2, '0')
                        + ":" + QString::number(min).rightJustified(2, '0')
                        + ":" + QString::number(sec).rightJustified(2, '0');
                }
            }

            return QString("Foobar");
        }

        return QVariant();
    }

    void QueueModel::tracksInserted(int firstIndex, int lastIndex) {
        qDebug() << "QueueModel::tracksInserted " << firstIndex << lastIndex;

        beginInsertRows(QModelIndex(), firstIndex, lastIndex);

        _modelRows += (lastIndex - firstIndex + 1);
        //_trac

        endInsertRows();
    }

    void QueueModel::tracksRemoved(int firstIndex, int lastIndex) {
        qDebug() << "QueueModel::tracksRemoved " << firstIndex << lastIndex;

        beginRemoveRows(QModelIndex(), firstIndex, lastIndex);

        _modelRows -= (lastIndex - firstIndex + 1);

        endRemoveRows();
    }

    void QueueModel::tracksChanged(int firstIndex, int lastIndex) {
        qDebug() << "QueueModel::tracksChanged " << firstIndex << lastIndex;

        emit dataChanged(createIndex(firstIndex, 0), createIndex(lastIndex, 2));
    }

}

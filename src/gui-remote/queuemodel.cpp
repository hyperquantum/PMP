/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "queueentryinfofetcher.h"
#include "queuemediator.h"

#include <QtDebug>

namespace PMP {

    QueueModel::QueueModel(QObject* parent, QueueMediator* source,
                           QueueEntryInfoFetcher* trackInfoFetcher)
     : QAbstractTableModel(parent), _source(source), _infoFetcher(trackInfoFetcher),
        _modelRows(0)
    {
        _modelRows = _source->queueLength();

        connect(
            _source, &AbstractQueueMonitor::queueResetted,
            this, &QueueModel::queueResetted
        );
        connect(
            _source, &AbstractQueueMonitor::entriesReceived,
            this, &QueueModel::entriesReceived
        );
        connect(
            _infoFetcher, &QueueEntryInfoFetcher::tracksChanged,
            this, &QueueModel::tracksChanged
        );
        connect(
            _source, &AbstractQueueMonitor::trackAdded,
            this, &QueueModel::trackAdded
        );
        connect(
            _source, &AbstractQueueMonitor::trackRemoved,
            this, &QueueModel::trackRemoved
        );
        connect(
            _source, &AbstractQueueMonitor::trackMoved,
            this, &QueueModel::trackMoved
        );
    }

    int QueueModel::rowCount(const QModelIndex& parent) const {
        //qDebug() << "QueueModel::rowCount returning" << _modelRows;
        return _modelRows;
    }

    int QueueModel::columnCount(const QModelIndex& parent) const {
        return 3;
    }

    QVariant QueueModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
    {
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
        //qDebug() << "QueueModel::data called with role" << role;

        if (role == Qt::DisplayRole) {
            quint32 queueID = trackIdAt(index);
            if (queueID == 0) { return QString(); }

            QueueEntryInfo* info = _infoFetcher->entryInfoByQID(queueID);
            if (info == 0) { return QString("? ") + QString::number(queueID); }

            int col = index.column();
            switch (col) {
                case 0:
                {
                    QString title = info->title();
                    if (title == "") {
                        return info->informativeFilename();
                    }

                    return title;
                }
                case 1: return info->artist();
                case 2:
                {
                    int lengthInSeconds = info->lengthInSeconds();

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
        else if (role == Qt::TextAlignmentRole) {
            return (index.column() == 2 ? Qt::AlignRight : Qt::AlignLeft)
                + Qt::AlignVCenter;
        }

        return QVariant();
    }

    Qt::ItemFlags QueueModel::flags(const QModelIndex& index) const {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled
            | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }

    Qt::DropActions QueueModel::supportedDropActions() const
    {
        return Qt::MoveAction;
    }

    quint32 QueueModel::trackIdAt(const QModelIndex& index) const {
        int row = index.row();
        if (row >= _tracks.size()) {
            /* make sure the info will be fetched from the server */
            (void)(_source->queueEntry(row));

            /* but we HAVE TO return 0 here */
            return 0;
        }

        Track* t = _tracks[row];
        if (t == 0) return 0;

        return t->_queueID;
    }

    void QueueModel::queueResetted(int queueLength) {
        qDebug() << "QueueModel::queueResetted; len=" << queueLength;

        if (_modelRows > 0) {
            beginRemoveRows(QModelIndex(), 0, _modelRows - 1);
            _modelRows = 0;
            Q_FOREACH(Track* t, _tracks) {
                delete t;
            }
            _tracks.clear();
            endRemoveRows();
        }

        if (queueLength > 0) {
            beginInsertRows(QModelIndex(), 0, queueLength - 1);
            _modelRows = queueLength;
            endInsertRows();
        }
    }

    void QueueModel::entriesReceived(int index, QList<quint32> entries) {
        qDebug() << "QueueModel::entriesReceived; index=" << index << "; count=" << entries.size();

        /* expand tracks list */
        if (index + entries.size() > _tracks.size()) {
            qDebug() << " expanding model tracks list";
            _tracks.reserve(index + entries.size());
            do {
                _tracks.append(0);
            } while (_tracks.size() < index + entries.size());
        }

        /* create Track instances */
        for (int i = 0; i < entries.size(); ++i) {
            int currentIndex = index + i;

            quint32 qid = entries[i];

            Track* t = _tracks[currentIndex];
            if (t == 0) {
                t = new Track(qid);
                _tracks[currentIndex] = t;
            }
            else if (t->_queueID != qid) {
                qDebug() << " PROBLEM: inconsistency detected at index" << currentIndex;
                delete t;
                t = new Track(qid);
                _tracks[currentIndex] = t;
            }
        }

        emit dataChanged(
            createIndex(index, 0),
            createIndex(index + entries.size() - 1, 2)
        );
    }

    void QueueModel::tracksChanged(QList<quint32> queueIDs) {
        qDebug() << "QueueModel::tracksChanged; count=" << queueIDs.size();

        /* we don't know the indexes, so we say everything changed */
        emit dataChanged(
            createIndex(0, 0), createIndex(_modelRows, 2)
        );
    }

    void QueueModel::trackAdded(int index, quint32 queueID) {
        qDebug() << "QueueModel::trackAdded; index=" << index << "; QID=" << queueID;

        beginInsertRows(QModelIndex(), index, index);

        if (index > _tracks.size()) {
            qDebug() << " need to expand QueueModel tracks list before we can append";
            _tracks.reserve(index + 1);

            while (_tracks.size() < index) {
                _tracks.append(0);
            }

            _tracks.append(new Track(queueID));
        }
        else {
            _tracks.insert(index, new Track(queueID));
        }

        _modelRows++;
        endInsertRows();
    }

    void QueueModel::trackRemoved(int index, quint32 queueID) {
        qDebug() << "QueueModel::trackRemoved; index=" << index << "; QID=" << queueID;

        beginRemoveRows(QModelIndex(), index, index);

        if (index < _tracks.size()) {
            Track* t = _tracks[index];
            delete t;
            _tracks.removeAt(index);
        }

        _modelRows--;
        endRemoveRows();
    }

    void QueueModel::trackMoved(int fromIndex, int toIndex, quint32 queueID) {
        qDebug() << "QueueModel::trackMoved; from=" << fromIndex << "; to=" << toIndex << "; QID=" << queueID;

        beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), toIndex);

        Track* t = 0;
        if (fromIndex < _tracks.size()) {
            t = _tracks[fromIndex];
            _tracks.removeAt(fromIndex);
        }

        if (toIndex <= _tracks.size()) {
            if (t == 0) {
                t = new Track(queueID);
            }

            _tracks.insert(toIndex, t);
        }
        else {
            delete t;
        }

        endMoveRows();
    }
}

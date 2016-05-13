/*
    Copyright (C) 2014-2016, Kevin Andre <hyperquantum@gmail.com>

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
#include "userdatafetcher.h"

#include <QBuffer>
#include <QBrush>
#include <QColor>
#include <QDataStream>
#include <QFont>
#include <QList>
#include <QMimeData>
#include <QtDebug>

namespace PMP {

    QueueModel::QueueModel(QObject* parent, QueueMediator* source,
                           QueueEntryInfoFetcher* trackInfoFetcher,
                           UserDataFetcher* userDataFetcher)
     : QAbstractTableModel(parent), _source(source), _infoFetcher(trackInfoFetcher),
        _userDataFetcher(userDataFetcher), _userPlayingFor(0), _modelRows(0)
    {
        _modelRows = _source->queueLength();

        connect(
            _infoFetcher, &QueueEntryInfoFetcher::userPlayingForChanged,
            this, &QueueModel::onUserPlayingForChanged
        );
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
        return 4;
    }

    QVariant QueueModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
    {
        if (role == Qt::DisplayRole) {
            if (orientation == Qt::Horizontal) {
                switch (section) {
                    case 0: return QString(tr("Title"));
                    case 1: return QString(tr("Artist"));
                    case 2: return QString(tr("Length"));
                    case 3: return QString(tr("Prev. heard"));
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

        int col = index.column();

        quint32 queueID = trackIdAt(index);

        QueueEntryInfo* info =
            queueID ? _infoFetcher->entryInfoByQID(queueID) : nullptr;

        /* handle real track entries in a separate function */
        if (info && info->type() == QueueEntryType::Track)
            return trackModelData(info, col, role);

        /* handling for non-track queue entries */

        switch (role) {
            case Qt::DisplayRole:
                if (info == 0) { return QString(); }

                switch (info->type()) {
                    case QueueEntryType::Unknown:
                        break; /* default: empty */

                    case QueueEntryType::Track:
                        break; /* unreachable, see above */

                    case QueueEntryType::BreakPoint:
                        if (col <= 1) return "--- BREAK ---";
                        return "";
                }
                break;

            case Qt::TextAlignmentRole:
                if (info == 0) { return QVariant(); }

                return Qt::AlignCenter;

            case Qt::FontRole:
            {
                QFont font;
                font.setItalic(true);
                return font;
            }
            case Qt::ForegroundRole:
                return QBrush(QColor::fromRgb(30, 30, 30));

            case Qt::BackgroundRole:
                return QBrush(QColor::fromRgb(220, 220, 220));
        }

        return QVariant();
    }

    QVariant QueueModel::trackModelData(QueueEntryInfo* info, int col, int role) const {
        switch (role) {
            case Qt::TextAlignmentRole:
                return (col == 2 ? Qt::AlignRight : Qt::AlignLeft)
                    + Qt::AlignVCenter;

            case Qt::DisplayRole:
                break; /* handled below */

            default:
                return QVariant();
        }

        /* DisplayRole */

        switch (col) {
            case 0:
            {
                QString title = info->title();
                return (title != "") ? title : info->informativeFilename();
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
            case 3:
            {
                auto& hash = info->hash();
                if (hash.empty()) return QVariant(); /* unknown */

                auto hashData =
                    _userDataFetcher->getHashDataForUser(_userPlayingFor, hash);

                if (!hashData || !hashData->previouslyHeardKnown)
                    return QVariant();

                return hashData->previouslyHeard; // TODO: formatting
            }
        }

        return QString("Foobar");
    }

    Qt::ItemFlags QueueModel::flags(const QModelIndex& index) const {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled
            | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }

    Qt::DropActions QueueModel::supportedDropActions() const
    {
        return Qt::MoveAction;
    }

    QStringList QueueModel::mimeTypes() const {
        return QStringList(QString("application/x-pmp-queueitem"));
    }

    QMimeData* QueueModel::mimeData(const QModelIndexList& indexes) const {
        if (indexes.isEmpty()) return 0;

        qDebug() << "QueueModel::mimeData called; indexes count =" << indexes.size();

        QUuid serverUuid = _source->serverUuid();
        if (serverUuid.isNull()) return 0;

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QDataStream stream(&buffer);
        stream.setVersion(QDataStream::Qt_5_2);

        stream << serverUuid;

        QList<int> rows;
        QList<quint32> ids;
        int prevRow = -1;
        Q_FOREACH(const QModelIndex& index, indexes) {
            int row = index.row();
            if (row == prevRow) continue;
            prevRow = row;

            quint32 queueID = _source->queueEntry(row);
            qDebug() << " row" << row << "; col" << index.column() << ";  QID" << queueID;
            rows.append(row);
            ids.append(queueID);
        }

        stream << (quint32)rows.size();
        for (int i = 0; i < rows.size(); ++i) {
            stream << (quint32)(uint)rows[i];
            stream << ids[i];
        }

        buffer.close();

        QMimeData* data = new QMimeData();

        data->setData("application/x-pmp-queueitem", buffer.data());
        return data;
    }

    bool QueueModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                  int row, int column, const QModelIndex& parent)
    {
        qDebug() << "QueueModel::dropMimeData called; action=" << action
                 << "; row=" << row;

        if (row == -1) {
            row = parent.row();
            column = parent.column();
            //parent = parent.parent();
            qDebug() << " went up one level: row=" << row;
        }

        if (!data->hasFormat("application/x-pmp-queueitem")) return false;
        QDataStream stream(data->data("application/x-pmp-queueitem"));

        QUuid serverUuid;
        stream >> serverUuid;

        qint32 count;
        stream >> count;
        if (count > 1) return false; // FIXME

        quint32 oldIndex;
        quint32 queueID;
        stream >> oldIndex;
        stream >> queueID;

        int newIndex;
        if (row < 0) {
            newIndex = _modelRows - 1;
        }
        else if ((uint)row > oldIndex) {
            newIndex = row - 1;
        }
        else {
            newIndex = row;
        }

        qDebug() << " oldIndex=" << oldIndex << " ; QID=" << queueID
                 << "; newIndex=" << newIndex;

        if (oldIndex != (uint)newIndex) {
            _source->moveTrack(oldIndex, newIndex, queueID);
        }

        return true;
    }

    bool QueueModel::canDropMimeData(const QMimeData* data, Qt::DropAction action,
                                     int row, int column, const QModelIndex& parent) const
    {
        if (!QAbstractTableModel::canDropMimeData(data, action, row, column, parent)) {
            return false;
        }

        if (!data->hasFormat("application/x-pmp-queueitem")) return false;

        QDataStream stream(data->data("application/x-pmp-queueitem"));

        QUuid serverUuid;
        stream >> serverUuid;

        if (serverUuid.isNull() || serverUuid != _source->serverUuid()) return false;

        return true;
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

    void QueueModel::onUserPlayingForChanged(quint32 userId) {
        _userPlayingFor = userId;

        // TODO : invalidate the user-bound columns


    }

    void QueueModel::queueResetted(int queueLength) {
        qDebug() << "QueueModel::queueResetted; len=" << queueLength;

        if (_modelRows > 0) {
            beginRemoveRows(QModelIndex(), 0, _modelRows - 1);
            _modelRows = 0;
            qDeleteAll(_tracks);
            _tracks.clear();
            endRemoveRows();
        }

        if (queueLength > 0) {
            beginInsertRows(QModelIndex(), 0, queueLength - 1);
            _modelRows = queueLength;
            QList<quint32> knownQueue = _source->knownQueuePart();
            _tracks.reserve(knownQueue.size());
            Q_FOREACH(quint32 queueID, knownQueue) {
                _tracks.append(new Track(queueID));
            }
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
        qDebug() << "QueueModel::trackMoved; oldIndex=" << fromIndex
                 << "; newIndex=" << toIndex << "; QID=" << queueID;

        if (fromIndex == toIndex) return; /* should not happen */

        /* the to-index has a slightly different meaning for Qt, so we need adjustment */
        int qtToIndex = (fromIndex < toIndex) ? toIndex + 1 : toIndex;
        beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), qtToIndex);

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

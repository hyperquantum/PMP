/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/clientserverinterface.h"
#include "common/playercontroller.h"
#include "common/userdatafetcher.h"
#include "common/util.h"

#include "colors.h"
#include "queueentryinfofetcher.h"
#include "queuemediator.h"

#include <QBuffer>
#include <QBrush>
#include <QColor>
#include <QDataStream>
#include <QFont>
#include <QList>
#include <QMimeData>
#include <QtDebug>

namespace PMP {

    QueueModel::QueueModel(QObject* parent, ClientServerInterface* clientServerInterface,
                           QueueMediator* source, QueueEntryInfoFetcher* trackInfoFetcher)
     : QAbstractTableModel(parent),
       _clientServerInterface(clientServerInterface),
       _source(source),
       _infoFetcher(trackInfoFetcher),
       _playerMode(PlayerMode::Unknown),
       _personalModeUserId(0),
       _modelRows(0)
    {
        _modelRows = _source->queueLength();

        auto playerController = &clientServerInterface->playerController();
        auto userDataFetcher = &_clientServerInterface->userDataFetcher();

        connect(
            playerController, &PlayerController::playerModeChanged,
            this, &QueueModel::playerModeChanged
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
            userDataFetcher, &UserDataFetcher::dataReceivedForUser,
            this, &QueueModel::userDataReceivedForUser
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

        _playerMode = playerController->playerMode();
        _personalModeUserId = playerController->personalModeUserId();
    }

    int QueueModel::rowCount(const QModelIndex& parent) const {
        Q_UNUSED(parent)

        //qDebug() << "QueueModel::rowCount returning" << _modelRows;
        return _modelRows;
    }

    int QueueModel::columnCount(const QModelIndex& parent) const {
        Q_UNUSED(parent)

        return 5;
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
                    case 4: return QString(tr("Score"));
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
                if (info == nullptr) { return QString(); }

                switch (info->type()) {
                    case QueueEntryType::Unknown:
                        break; /* could be anything */

                    case QueueEntryType::Track:
                        break; /* unreachable, see above */

                    case QueueEntryType::BreakPoint:
                        if (col <= 1)
                            return Util::EmDash + QString(" BREAK ") + Util::EmDash;

                        return QString();

                    case QueueEntryType::UnknownSpecialType:
                        if (col <= 1)
                            return Util::EmDash + QString(" ????? ") + Util::EmDash;

                        return QString();
                }
                break;

            case Qt::TextAlignmentRole:
                if (info)
                    return Qt::AlignCenter;
                break;

            case Qt::FontRole:
                if (info) {
                    QFont font;
                    font.setItalic(true);
                    return font;
                }
                break;

            case Qt::ForegroundRole:
                if (info && info->type() != QueueEntryType::Unknown)
                    return QBrush(Colors::instance().specialQueueItemForeground);
                break;

            case Qt::BackgroundRole:
                if (info && info->type() != QueueEntryType::Unknown)
                    return QBrush(Colors::instance().specialQueueItemBackground);
                break;
        }

        return QVariant();
    }

    QVariant QueueModel::trackModelData(QueueEntryInfo* info, int col, int role) const {
        switch (role) {
            case Qt::TextAlignmentRole:
                switch (col) {
                    case 2:
                        return Qt::AlignRight + Qt::AlignVCenter;
                    case 4:
                    {
                        auto& hash = info->hash();
                        if (hash.isNull() || _playerMode == PlayerMode::Unknown)
                            return Qt::AlignLeft + Qt::AlignVCenter;

                        auto hashData =
                            _clientServerInterface->userDataFetcher().getHashDataForUser(
                                _personalModeUserId, hash
                            );

                        if (!hashData || !hashData->scoreReceived)
                            return Qt::AlignLeft + Qt::AlignVCenter;

                        if (hashData->scorePermillage >= 0)
                            return Qt::AlignRight + Qt::AlignVCenter;
                        else
                            return Qt::AlignLeft + Qt::AlignVCenter;
                    }
                    default:
                        return Qt::AlignLeft + Qt::AlignVCenter;
                }
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
                return Util::secondsToHoursMinuteSecondsText(lengthInSeconds);
            }
            case 3:
            {
                auto& hash = info->hash();
                if (hash.isNull() || _playerMode == PlayerMode::Unknown)
                    return QVariant(); /* unknown */

                auto hashData =
                        _clientServerInterface->userDataFetcher().getHashDataForUser(
                            _personalModeUserId, hash
                        );

                if (!hashData || !hashData->previouslyHeardReceived)
                    return QVariant();

                if (hashData->previouslyHeard.isNull())
                    return tr("Never");

                 // TODO: formatting?
                return hashData->previouslyHeard.toLocalTime();
            }
            case 4:
            {
                auto& hash = info->hash();
                if (hash.isNull() || _playerMode == PlayerMode::Unknown)
                    return QVariant(); /* unknown */

                auto hashData =
                        _clientServerInterface->userDataFetcher().getHashDataForUser(
                            _personalModeUserId, hash
                        );

                if (!hashData || !hashData->scoreReceived)
                    return QVariant();

                if (hashData->scorePermillage < 0)
                    return tr("N/A");

                return hashData->scorePermillage;
            }
        }

        return QVariant();
    }

    Qt::ItemFlags QueueModel::flags(const QModelIndex& index) const {
        Q_UNUSED(index)

        return Qt::ItemIsSelectable | Qt::ItemIsEnabled
            | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }

    Qt::DropActions QueueModel::supportedDragActions() const {
        return Qt::MoveAction; // TODO: add support for copying queue items
    }

    Qt::DropActions QueueModel::supportedDropActions() const {
        return Qt::MoveAction | Qt::CopyAction;
    }

    QStringList QueueModel::mimeTypes() const {
        return {
            QString("application/x-pmp-queueitem"),
            QString("application/x-pmp-filehash")
        };
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

    bool QueueModel::dropQueueItemMimeData(const QMimeData *data, Qt::DropAction action,
                                           int row)
    {
        Q_UNUSED(action)

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

    bool QueueModel::dropFileHashMimeData(const QMimeData *data, int row) {
        qDebug() << "QueueModel::dropFileHashMimeData called";

        QDataStream stream(data->data("application/x-pmp-filehash"));

        quint32 count;
        stream >> count;
        if (count > 1) return false; // FIXME

        quint64 hashLength;
        stream >> hashLength;
        QByteArray sha1, md5;
        stream >> sha1;
        stream >> md5;

        FileHash hash(hashLength, sha1, md5);
        if (hash.isNull()) return false;

        int newIndex = (row < 0) ? _modelRows : row;

        qDebug() << " inserting at index" << newIndex
                 << "filehash:" << hash.dumpToString();

        _source->insertFileAsync(newIndex, hash);
        return true;
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

        if (data->hasFormat("application/x-pmp-queueitem")) {
            return dropQueueItemMimeData(data, action, row);
        }

        if (data->hasFormat("application/x-pmp-filehash")) {
            return dropFileHashMimeData(data, row);
        }

        return false; /* format not supported */
    }

    bool QueueModel::canDropMimeData(const QMimeData* data, Qt::DropAction action,
                                     int row, int column, const QModelIndex& parent) const
    {
        qDebug() << "QueueModel::canDropMimeData called";

        if (!QAbstractTableModel::canDropMimeData(data, action, row, column, parent)) {
            return false;
        }

        if (data->hasFormat("application/x-pmp-queueitem")) {
            QDataStream stream(data->data("application/x-pmp-queueitem"));

            QUuid serverUuid;
            stream >> serverUuid;

            return !serverUuid.isNull() && serverUuid == _source->serverUuid();
        }

        if (data->hasFormat("application/x-pmp-filehash")) {
            qDebug() << "recognized application/x-pmp-filehash";

            QDataStream stream(data->data("application/x-pmp-filehash"));

            //

            return true; // FIXME
        }

        return false;
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
        if (t == nullptr) return 0;

        return t->_queueID;
    }

    QueueTrack QueueModel::trackAt(const QModelIndex& index) const
    {
        int row = index.row();
        if (row >= _tracks.size()) {
            /* make sure the info will be fetched from the server */
            (void)(_source->queueEntry(row));

            /* but we HAVE TO return something here */
            return QueueTrack();
        }

        Track* track = _tracks[row];
        if (track == nullptr) return QueueTrack();

        auto queueId = track->_queueID;

        QueueEntryInfo* info = queueId ? _infoFetcher->entryInfoByQID(queueId) : nullptr;

        if (info == nullptr)
            return QueueTrack();

        if (info->type() != QueueEntryType::Track)
            return QueueTrack(queueId, false);

        auto& hash = info->hash();

        return QueueTrack(queueId, hash);
    }

    void QueueModel::playerModeChanged(PlayerMode playerMode, quint32 personalModeUserId,
                                       QString personalModeUserLogin)
    {
        Q_UNUSED(personalModeUserLogin)

        _playerMode = playerMode;
        _personalModeUserId = personalModeUserId;

        /* the columns with user-bound data need to be refreshed */
        Q_EMIT dataChanged(createIndex(0, 3), createIndex(_modelRows, 3));
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
        qDebug() << "QueueModel::entriesReceived; index=" << index
                 << "; count=" << entries.size();

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

    void QueueModel::userDataReceivedForUser(quint32 userId) {
        qDebug() << "QueueModel::userDataReceivedForUser; user:" << userId;

        emit dataChanged(
            createIndex(0, 3), createIndex(_modelRows, 4)
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

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

#ifndef PMP_QUEUEMODEL_H
#define PMP_QUEUEMODEL_H

#include <QAbstractTableModel>
#include <QList>

namespace PMP {

    class QueueMonitor;

    class QueueModel : public QAbstractTableModel {
        Q_OBJECT
    public:
        QueueModel(QObject* parent, QueueMonitor* source);

        int rowCount(const QModelIndex& parent = QModelIndex()) const;
        int columnCount(const QModelIndex& parent = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex& index) const;
        Qt::DropActions supportedDropActions() const;

        uint trackAt(const QModelIndex& index) const;

    private slots:
        void tracksInserted(int firstIndex, int lastIndex);
        void tracksRemoved(int firstIndex, int lastIndex);
        void tracksChanged(int firstIndex, int lastIndex);

    private:
        struct Track {
            quint32 _queueID;
            QString _title;
            QString _artist;
            int _lengthSeconds;

            Track(quint32 queueID)
             : _queueID(queueID), _lengthSeconds(-1)
            {
                //
            }
        };

        QueueMonitor* _source;
        int _modelRows;
        QList<Track*> _tracks;
    };
}
#endif

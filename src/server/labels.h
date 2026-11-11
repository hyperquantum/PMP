/*
    Copyright (C) 2025-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_LABELS_H
#define PMP_LABELS_H

#include "common/future.h"
#include "common/resultorerror.h"

#include "result.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QSet>
#include <QString>

namespace PMP::Server
{
    class Labels : public QObject
    {
        Q_OBJECT
    public:
        explicit Labels(QObject* parent);

        SimpleFuture<Result> applyLabelToTrack(uint trackHashId, QString const& label);
        SimpleFuture<Result> removeLabelFromTrack(uint trackHashId, QString const& label);

        bool checkLabelExists(quint32 labelId);
        QList<quint32> getLabelsInActiveUse();
        QList<quint32> getLabelsOfTrack(uint trackHashId);
        ResultOrError<QHash<quint32, QString>, Error> getLabelNames(
                                                                QList<quint32> labelIds);
        QSet<uint> getTracksWithLabel(quint32 labelId);

        static bool isValidPotentialName(QString const& name);

    Q_SIGNALS:
        void trackLabelsAdded(uint trackHashId, QList<quint32> labelIds);
        void trackLabelsRemoved(uint trackHashId, QList<quint32> labelIds);

    private:
        void loadFromDatabase();

        struct LabelData
        {
            QString name;
            QSet<uint> hashes;
            bool inActiveUse { false };
        };

        QMutex _mutex;
        QHash<quint32, LabelData> _labelDataByLabelId;
        QHash<QString, quint32> _nameToId;
        QHash<uint, QSet<quint32>> _labelsByHashId;
    };
}
#endif

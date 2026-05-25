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

#ifndef PMP_CLIENT_TRACKLABELSCONTROLLER_H
#define PMP_CLIENT_TRACKLABELSCONTROLLER_H

#include "common/future.h"
#include "common/resultmessageerrorcode.h"

#include "client/labelidandname.h"

#include "localhashid.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

namespace PMP::Client
{
    class LabelsController;

    class TrackLabelsController : public QObject
    {
        Q_OBJECT
    public:
        TrackLabelsController(QObject* parent, LocalHashId hashId,
                              LabelsController* labelsController);

        QList<QString> getLabelNames();

        SimpleFuture<AnyResultMessageCode> addLabel(QString labelName);
        SimpleFuture<AnyResultMessageCode> removeLabel(QString labelName);

    Q_SIGNALS:
        void labelsAdded(QList<QString> labelNames);
        void labelsRemoved(QList<QString> labelNames);

    private Q_SLOTS:
        void onTrackLabelsChanged(LocalHashId hashId);

    private:
        void receivedCompleteList(QList<LabelIdAndName> labels);

        LocalHashId _hashId;
        LabelsController* _labelsController;
        QHash<quint32, LabelIdAndName> _labels;
    };
}
#endif

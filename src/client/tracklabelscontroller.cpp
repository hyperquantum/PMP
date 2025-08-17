/*
    Copyright (C) 2025, Kevin André <hyperquantum@gmail.com>

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

#include "tracklabelscontroller.h"

#include "labelscontroller.h"

namespace PMP::Client
{
    TrackLabelsController::TrackLabelsController(QObject* parent, LocalHashId hashId,
                                                 LabelsController* labelsController)
     : QObject(parent),
        _hashId(hashId),
        _labelsController(labelsController)
    {
        connect(
            labelsController, &LabelsController::trackLabelsChanged,
            this, &TrackLabelsController::onTrackLabelsChanged
        );

        auto namesFuture = labelsController->getLabelNamesByTrack(hashId);

        namesFuture.handleOnEventLoop(
            this,
            [this](ResultOrError<QList<QString>, AnyResultMessageCode> outcome)
            {
                if (outcome.failed())
                {
                    qWarning() << "TrackLabelsController: failed to fetch labels of track"
                               << _hashId
                               << "; error:" << errorCodeString(outcome.error());
                    return;
                }

                receivedCompleteList(outcome.result());
            }
        );
    }

    QList<QString> TrackLabelsController::getLabelNames()
    {
        return _labelNames;
    }

    SimpleFuture<AnyResultMessageCode> TrackLabelsController::addLabel(QString labelName)
    {
        return _labelsController->applyLabelToTrack(_hashId, labelName);
    }

    void TrackLabelsController::onTrackLabelsChanged(LocalHashId hashId,
                                                     QList<quint32> labelsAddedIds,
                                                     QList<quint32> labelsRemovedIds)
    {
        auto removedNamesFuture = _labelsController->getLabelNamesByIds(labelsRemovedIds);
        auto addedNamesFuture = _labelsController->getLabelNamesByIds(labelsAddedIds);

        addedNamesFuture.handleOnEventLoop(
            this,
            [this, labelsAddedIds](
                ResultOrError<QHash<quint32,QString>, AnyResultMessageCode> outcome)
            {
                if (outcome.failed())
                {
                    qWarning() << "TrackLabelsController: failed to fetch label names of"
                                  " IDs" << labelsAddedIds
                               << "; error:" << errorCodeString(outcome.error());
                    return;
                }

                auto namesToAdd = outcome.result().values();
                _labelNames.append(namesToAdd);

                Q_EMIT labelsAdded(namesToAdd);
            }
        );

        removedNamesFuture.handleOnEventLoop(
            this,
            [this, labelsRemovedIds](
                ResultOrError<QHash<quint32,QString>, AnyResultMessageCode> outcome)
            {
                if (outcome.failed())
                {
                    qWarning() << "TrackLabelsController: failed to fetch label names of"
                                  " IDs" << labelsRemovedIds
                               << "; error:" << errorCodeString(outcome.error());
                    return;
                }

                auto namesToRemove = outcome.result().values();

                _labelNames.removeIf(
                    [namesToRemove](QString name) { return namesToRemove.contains(name); }
                );

                Q_EMIT labelsRemoved(namesToRemove);
            }
        );
    }

    void TrackLabelsController::receivedCompleteList(QList<QString> labelNames)
    {
        if (_labelNames.isEmpty())
        {
            _labelNames = labelNames;
            Q_EMIT labelsAdded(labelNames);
            return;
        }

        // TODO
    }
}

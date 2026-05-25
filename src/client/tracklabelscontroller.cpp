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

        auto labelsFuture = labelsController->getLabelsByTrack(hashId);

        labelsFuture.handleOnEventLoop(
            this,
            [this](ResultOrError<QList<LabelIdAndName>, AnyResultMessageCode> outcome)
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
        QList<QString> result;
        result.reserve(_labels.size());

        for (auto const& label : _labels)
        {
            result << label.name();
        }

        return result;
    }

    SimpleFuture<AnyResultMessageCode> TrackLabelsController::addLabel(QString labelName)
    {
        return _labelsController->applyLabelToTrack(_hashId, labelName);
    }

    SimpleFuture<AnyResultMessageCode> TrackLabelsController::removeLabel(
        QString labelName)
    {
        return _labelsController->removeLabelFromTrack(_hashId, labelName);
    }

    void TrackLabelsController::onTrackLabelsChanged(LocalHashId hashId)
    {
        auto labelsFuture = _labelsController->getLabelsByTrack(hashId);

        labelsFuture.handleOnEventLoop(
            this,
            [this](ResultOrError<QList<LabelIdAndName>, AnyResultMessageCode> outcome)
            {
                if (outcome.failed())
                {
                    qWarning()
                        << "TrackLabelsController: failed to update labels of track"
                        << _hashId << "; error:" << errorCodeString(outcome.error());
                    return;
                }

                receivedCompleteList(outcome.result());
            }
        );
    }

    void TrackLabelsController::receivedCompleteList(QList<LabelIdAndName> labels)
    {
        QList<QString> namesAdded;

        QHash<quint32, LabelIdAndName> asHash;
        asHash.reserve(labels.size());

        for (auto const& label : labels)
        {
            asHash.insert(label.id(), label);

            if (!_labels.contains(label.id()))
                namesAdded << label.name();
        }

        QList<QString> namesRemoved;

        for (auto const& label : _labels)
        {
            if (!asHash.contains(label.id()))
                namesRemoved << label.name();
        }

        _labels = asHash;

        if (!namesAdded.isEmpty())
            Q_EMIT labelsAdded(namesAdded);

        if (!namesRemoved.isEmpty())
            Q_EMIT labelsRemoved(namesRemoved);
    }
}

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

#include "labelscontrollerimpl.h"

#include "serverconnection.h"

#include <QtAssert>

namespace PMP::Client
{
    LabelsControllerImpl::LabelsControllerImpl(ServerConnection* connection)
     : LabelsController(connection),
       _connection{connection}
    {
        //
    }

    SimpleFuture<AnyResultMessageCode> LabelsControllerImpl::applyLabelToTrack(
        LocalHashId hashId, QString label)
    {
        return _connection->applyLabelToTrack(hashId, label);
    }

    SimpleFuture<AnyResultMessageCode> LabelsControllerImpl::removeLabelFromTrack(
        LocalHashId hashId, QString label)
    {
        return _connection->removeLabelFromTrack(hashId, label);
    }

    Future<QList<QString>, AnyResultMessageCode>
        LabelsControllerImpl::getLabelNamesByTrack(LocalHashId hashId)
    {
        auto idsFuture = _connection->getLabelsOfTrack(hashId);

        auto namesFuture =
            idsFuture
                .thenOnEventLoopIndirect<QHash<quint32,QString>, AnyResultMessageCode>(
                    this,
                    [this](ResultOrError<QList<quint32>, AnyResultMessageCode> outcome)
                        -> Future<QHash<quint32, QString>, AnyResultMessageCode>
                    {
                        if (outcome.failed())
                            return FutureError(outcome.error());

                        auto labelIds = outcome.result();
                        return getLabelNamesFromIdsInternal(labelIds);
                    }
                )
                .thenOnEventLoop<QList<QString>, AnyResultMessageCode>(
                    this,
                    [](ResultOrError<QHash<quint32,QString>,AnyResultMessageCode> outcome)
                        -> ResultOrError<QList<QString>, AnyResultMessageCode>
                    {
                       if (outcome.failed())
                           return outcome.error();

                       return outcome.result().values();
                    }
                );

        return namesFuture;
    }

    Future<QHash<quint32, QString>, AnyResultMessageCode>
        LabelsControllerImpl::getLabelNamesFromIdsInternal(QList<quint32> labelIds)
    {
        auto fetchFuture = fetchMissingLabelNames(labelIds);

        auto resultFuture =
            fetchFuture.thenOnEventLoop<QHash<quint32,QString>, AnyResultMessageCode>(
                this,
                [this, labelIds](
                    ResultOrError<SuccessType, AnyResultMessageCode> outcomeOfFetch)
                    -> ResultOrError<QHash<quint32,QString>, AnyResultMessageCode>
                {
                    if (outcomeOfFetch.failed())
                        return outcomeOfFetch.error();

                    return getLabelIdsToNamesMappingAssumingFetched(labelIds);
                }
            );

        return resultFuture;
    }

    Future<SuccessType, AnyResultMessageCode>
        LabelsControllerImpl::fetchMissingLabelNames(QList<quint32> labelIds)
    {
        QList<quint32> idsToFetch;

        for (auto labelId : labelIds)
        {
            if (!_labelIdToName.contains(labelId))
                idsToFetch << labelId;
        }

        if (idsToFetch.isEmpty())
            return FutureResult(success);

        auto fetchFuture = _connection->getLabelNames(idsToFetch);

        auto storeFuture =
            fetchFuture.thenOnEventLoop<SuccessType, AnyResultMessageCode>(
                this,
                [this](ResultOrError<QHash<quint32,QString>,AnyResultMessageCode> outcome)
                    -> ResultOrError<SuccessType, AnyResultMessageCode>
                {
                    if (outcome.failed())
                        return outcome.error();

                    const auto idsToNames = outcome.result();

                    for (auto it = idsToNames.begin(); it != idsToNames.end(); ++it)
                    {
                        auto labelId = it.key();
                        auto labelName = it.value();

                        _labelIdToName.insert(labelId, labelName);
                        _labelNameToId.insert(labelName, labelId);
                    }

                    return success;
                }
            );

        return storeFuture;
    }

    QHash<quint32, QString>
        LabelsControllerImpl::getLabelIdsToNamesMappingAssumingFetched(
                                                                QList<quint32> labelIds)
    {
        QHash<quint32, QString> result;
        result.reserve(labelIds.size());

        for (auto labelId : labelIds)
        {
            auto it = _labelIdToName.constFind(labelId);

            Q_ASSERT_X(it != _labelIdToName.constEnd(),
                       "LabelsControllerImpl::getLabelIdsToNamesMappingAssumingFetched",
                       "name of label is not known");

            result.insert(labelId, it.value());
        }

        return result;
    }
}

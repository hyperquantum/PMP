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

#include "common/containerutil.h"

#include "serverconnection.h"

//#include <QtAssert> -- requires Qt 6.5
#include <QtGlobal> // instead of <QtAssert>

namespace PMP::Client
{
    LabelsControllerImpl::LabelsControllerImpl(ServerConnection* connection)
     : LabelsController(connection),
       _connection{connection}
    {
        connect(connection, &ServerConnection::trackLabelsChanged,
                this, &LabelsControllerImpl::onTrackLabelsChanged);
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
        auto idsFuture = getLabelsByTrackInternal(hashId);

        auto namesFuture =
            idsFuture
                .thenOnEventLoopIndirect<QList<QString>,AnyResultMessageCode>(
                    this,
                    [this](ResultOrError<QSet<quint32>, AnyResultMessageCode> outcome)
                        -> Future<QList<QString>, AnyResultMessageCode>
                    {
                        if (outcome.failed())
                            return FutureError(outcome.error());

                        auto labelIds = outcome.result();
                        return convertLabelIdsToLabelNamesInternal(labelIds);
                    }
                );

        return namesFuture;
    }

    Future<QHash<quint32, QString>, AnyResultMessageCode>
        LabelsControllerImpl::getLabelNamesByIds(QList<quint32> labelIds)
    {
        return getLabelIdsToNamesMappingInternal(labelIds);
    }

    Future<QList<QString>, AnyResultMessageCode>
        LabelsControllerImpl::getActiveLabelNames()
    {
        auto idsFuture = getActiveLabelsInternal();

        auto namesFuture =
            idsFuture
                .thenOnEventLoopIndirect<QList<QString>,AnyResultMessageCode>(
                    this,
                    [this](ResultOrError<QList<quint32>, AnyResultMessageCode> outcome)
                        -> Future<QList<QString>, AnyResultMessageCode>
                    {
                        if (outcome.failed())
                            return FutureError(outcome.error());

                        auto labelIds = outcome.result();
                        return convertLabelIdsToLabelNamesInternal(labelIds);
                    }
                );

        return namesFuture;
    }

    void LabelsControllerImpl::onTrackLabelsChanged(LocalHashId hashId,
                                                    QList<quint32> labelsAddedIds,
                                                    QList<quint32> labelsRemovedIds)
    {
        auto& currentLabelIds = _hashToLabelIds[hashId].labelIds;

        auto labelsReallyAdded =
            ContainerUtil::elementsOfListNotInSet(labelsAddedIds, currentLabelIds);

        if (labelsAddedIds.size() != labelsReallyAdded.size())
        {
            qDebug() << "LabelsControllerImpl: difference between added and really added;"
                        " added:" << labelsAddedIds
                     << "; really added:" << labelsReallyAdded;
        }

        auto labelsReallyRemoved =
            ContainerUtil::elementsOfListAlsoInSet(labelsRemovedIds, currentLabelIds);

        if (labelsRemovedIds.size() != labelsReallyRemoved.size())
        {
            qDebug() << "LabelsControllerImpl: difference between removed and really"
                        " removed; removed:" << labelsRemovedIds
                     << "; really removed:" << labelsReallyRemoved;
        }

        ContainerUtil::removeFromSet(labelsReallyRemoved, currentLabelIds);
        ContainerUtil::addToSet(labelsReallyAdded, currentLabelIds);

        Q_EMIT trackLabelsChanged(hashId, labelsReallyAdded, labelsReallyRemoved);
    }

    Future<QSet<quint32>, AnyResultMessageCode>
        LabelsControllerImpl::getLabelsByTrackInternal(LocalHashId hashId)
    {
        auto& hashData = _hashToLabelIds[hashId];

        if (hashData.fetched)
        {
            return FutureResult(hashData.labelIds);
        }

        if (hashData.futureForFetching.hasValue())
            return hashData.futureForFetching.value();

        auto future =
            _connection->getLabelsOfTrack(hashId)
                .thenOnEventLoop<QSet<quint32>, AnyResultMessageCode>(
                    this,
                    [this, hashId](
                            ResultOrError<QList<quint32>, AnyResultMessageCode> outcome)
                    -> ResultOrError<QSet<quint32>, AnyResultMessageCode>
                    {
                        if (outcome.failed())
                            return outcome.error();

                        auto labelIdsAsSet = ContainerUtil::toSet(outcome.result());

                        auto& hashData = _hashToLabelIds[hashId];
                        hashData.labelIds = labelIdsAsSet;
                        hashData.futureForFetching = null;
                        hashData.fetched = true;

                        return labelIdsAsSet;
                    }
                );

        hashData.futureForFetching = future;

        return future;
    }

    Future<QList<quint32>, AnyResultMessageCode>
        LabelsControllerImpl::getActiveLabelsInternal()
    {
        // TODO: caching
        return _connection->getActiveLabels();
    }

    template<typename TContainer>
    Future<QHash<quint32, QString>, AnyResultMessageCode>
        LabelsControllerImpl::getLabelIdsToNamesMappingInternal(TContainer labelIds)
    {
        Future<SuccessType, AnyResultMessageCode> fetchFuture =
            fetchMissingLabelNames(labelIds);

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

    template<typename TContainer>
    Future<QList<QString>, AnyResultMessageCode>
        LabelsControllerImpl::convertLabelIdsToLabelNamesInternal(TContainer labelIds)
    {
        Future<SuccessType, AnyResultMessageCode> fetchFuture =
            fetchMissingLabelNames(labelIds);

        auto resultFuture =
            fetchFuture.thenOnEventLoop<QList<QString>, AnyResultMessageCode>(
                this,
                [this, labelIds](
                    ResultOrError<SuccessType, AnyResultMessageCode> outcomeOfFetch)
                    -> ResultOrError<QList<QString>, AnyResultMessageCode>
                {
                    if (outcomeOfFetch.failed())
                        return outcomeOfFetch.error();

                    return convertLabelIdsToLabelNamesAssumingFetched(labelIds);
                }
            );

        return resultFuture;
    }

    template<typename TContainer>
    Future<SuccessType, AnyResultMessageCode>
        LabelsControllerImpl::fetchMissingLabelNames(TContainer labelIds)
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

    template<typename TContainer>
    QHash<quint32, QString>
        LabelsControllerImpl::getLabelIdsToNamesMappingAssumingFetched(
                                                                    TContainer labelIds)
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

    template<typename TContainer>
    QList<QString>
        LabelsControllerImpl::convertLabelIdsToLabelNamesAssumingFetched(
                                                                    TContainer labelIds)
    {
        QList<QString> result;
        result.reserve(labelIds.size());

        for (auto labelId : labelIds)
        {
            auto it = _labelIdToName.constFind(labelId);

            Q_ASSERT_X(it != _labelIdToName.constEnd(),
                       "LabelsControllerImpl::convertLabelIdsToLabelNamesAssumingFetched",
                       "name of label is not known");

            result.append(it.value());
        }

        return result;
    }
}

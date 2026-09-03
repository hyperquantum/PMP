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

#include "labelscontrollerimpl.h"

#include "serverconnection.h"
#include "trackidslistfetcher.h"

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

    TriBool LabelsControllerImpl::tryCheckIfTrackHasLabel(LocalHashId hashId,
                                                          quint32 labelId)
    {
        auto trackIt = _hashData.constFind(hashId);
        if (trackIt != _hashData.constEnd())
        {
            auto& trackData = trackIt.value();

            if (trackData.fetched)
                return trackData.labelIds.contains(labelId);

            if (trackData.labelIds.contains(labelId))
                return true;

            if (trackData.labelsRemovedIds.contains(labelId))
                return false;
        }

        return TriBool::unknown;
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

    Future<QList<LabelIdAndName>, AnyResultMessageCode>
        LabelsControllerImpl::getLabelsByTrack(LocalHashId hashId)
    {
        auto idsFuture = getLabelsByTrackInternal(hashId);

        auto idAndNamesFuture =
            idsFuture
                .thenOnEventLoopIndirect<QList<LabelIdAndName>,AnyResultMessageCode>(
                    this,
                    [this](ResultOrError<QSet<quint32>, AnyResultMessageCode> outcome)
                        -> Future<QList<LabelIdAndName>, AnyResultMessageCode>
                    {
                        if (outcome.failed())
                            return FutureError(outcome.error());

                        auto labelIds = outcome.result();
                        return convertLabelIdsToLabelIdAndNamesInternal(labelIds);
                    }
                );

        return idAndNamesFuture;
    }

    void LabelsControllerImpl::ensureFetchingOfTracksForLabel(quint32 labelId)
    {
        ensureFetchingOfTracksForLabelInternal(labelId);
    }

    Nullable<QString> LabelsControllerImpl::tryGetLabelNameById(quint32 labelId)
    {
        auto it = _labelData.constFind(labelId);
        if (it == _labelData.constEnd())
            return null;

        return it.value().name;
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

    Future<QList<LabelIdAndName>, AnyResultMessageCode>
        LabelsControllerImpl::getActiveLabels()
    {
        auto idsFuture = getActiveLabelsInternal();

        auto idAndNamesFuture =
            idsFuture
                .thenOnEventLoopIndirect<QList<LabelIdAndName>,AnyResultMessageCode>(
                    this,
                    [this](ResultOrError<QList<quint32>, AnyResultMessageCode> outcome)
                        -> Future<QList<LabelIdAndName>, AnyResultMessageCode>
                    {
                        if (outcome.failed())
                            return FutureError(outcome.error());

                        auto labelIds = outcome.result();
                        return convertLabelIdsToLabelIdAndNamesInternal(labelIds);
                    }
                );

        return idAndNamesFuture;
    }

    void LabelsControllerImpl::onTrackLabelsChanged(LocalHashId hashId,
                                                    QList<quint32> labelsAddedIds,
                                                    QList<quint32> labelsRemovedIds)
    {
        auto& hashData = _hashData[hashId];
        auto& currentLabelIds = hashData.labelIds;

        bool haveRealChanges = false;

        for (auto labelId : labelsAddedIds)
        {
            haveRealChanges |= hashData.labelsRemovedIds.remove(labelId);

            auto oldSize = currentLabelIds.size();
            currentLabelIds.insert(labelId);
            haveRealChanges |= currentLabelIds.size() > oldSize;

            _labelData[labelId].hashes.insert(hashId);
        }

        for (auto labelId : labelsRemovedIds)
        {
            hashData.labelsRemovedIds.insert(labelId); // really necessary ???

            haveRealChanges |= currentLabelIds.remove(labelId);

            _labelData[labelId].hashes.remove(hashId);
        }

        if (haveRealChanges)
            Q_EMIT trackLabelsChanged(hashId);
    }

    void LabelsControllerImpl::ensureFetchingOfTracksForLabelInternal(quint32 labelId)
    {
        auto& labelData = _labelData[labelId];
        if (labelData.hashesFetched || labelData.fetchRequestSent)
            return;

        auto* fetcher = new TrackIdsListFetcher();

        connect(
            fetcher, &TrackIdsListFetcher::receivedIds,
            this,
            [this, labelId](QList<quint64> trackIds)
            {
                bool conversionOk = true;
                auto localTrackIds =
                    tryConvertServerIdsToLocalIds(trackIds, conversionOk);

                if (!conversionOk)
                    _labelData[labelId].fetchResultWillBeIncomplete = true;

                receivedTracksForLabelChunk(labelId, localTrackIds);
            }
        );

        connect(
            fetcher, &TrackIdsListFetcher::completed,
            this,
            [this, labelId]()
            {
                auto& labelData = _labelData[labelId];
                bool incompleteFetch = labelData.fetchResultWillBeIncomplete;
                labelData.hashesFetched = !incompleteFetch;
                labelData.fetchRequestSent = false;

                /* try fetching again if the result was incomplete */
                if (incompleteFetch)
                    ensureFetchingOfTracksForLabelInternal(labelId);
            }
        );

        connect(
            fetcher, &TrackIdsListFetcher::errorOccurred,
            this,
            [this, labelId]()
            {
                qWarning() << "LabelsControllerImpl: failed to fetch tracks for label"
                           << labelId;

                auto& labelData = _labelData[labelId];
                labelData.hashesFetched = false;
                labelData.fetchRequestSent = false;
                labelData.fetchResultWillBeIncomplete = false;
            }
        );

        _connection->fetchTracksHavingLabel(labelId, fetcher);
        labelData.fetchRequestSent = true;
        labelData.fetchResultWillBeIncomplete = false;
    }

    QList<LocalHashId> LabelsControllerImpl::tryConvertServerIdsToLocalIds(
        QList<quint64> trackIds, bool &ok)
    {
        QList<LocalHashId> result;
        result.reserve(trackIds.size());

        auto* hashIdRepository = _connection->hashIdRepository();

        for (auto serverTrackId : trackIds)
        {

        }


        // TODO
    }

    void LabelsControllerImpl::receivedTracksForLabelChunk(quint32 labelId,
                                                           QList<LocalHashId> hashes)
    {
        auto& labelData = _labelData[labelId];

        for (auto& hashId : hashes)
        {
            auto& hashData = _hashData[hashId];

            if (hashData.labelIds.contains(labelId)
                || hashData.labelsRemovedIds.contains(labelId))
            {
                continue;
            }

            auto oldSize = hashData.labelIds.size();
            hashData.labelIds.insert(labelId);
            bool haveChangeForTrack = hashData.labelIds.size() > oldSize;

            labelData.hashes.insert(hashId);

            if (haveChangeForTrack)
                Q_EMIT trackLabelsChanged(hashId);
        }
    }

    Future<QSet<quint32>, AnyResultMessageCode>
        LabelsControllerImpl::getLabelsByTrackInternal(LocalHashId hashId)
    {
        auto& hashData = _hashData[hashId];

        if (hashData.fetched)
            return FutureResult(hashData.labelIds);

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

                        auto& hashData = _hashData[hashId];
                        auto oldSize = hashData.labelIds.size();

                        for (auto labelId : outcome.result())
                        {
                            if (hashData.labelIds.contains(labelId)
                                || hashData.labelsRemovedIds.contains(labelId))
                            {
                                continue;
                            }

                            hashData.labelIds.insert(labelId);
                            _labelData[labelId].hashes.insert(hashId);
                        }

                        hashData.labelsRemovedIds.clear();
                        hashData.futureForFetching = null;
                        hashData.fetched = true;

                        bool haveRealChanges = hashData.labelIds.size() > oldSize;

                        if (haveRealChanges)
                            Q_EMIT trackLabelsChanged(hashId);

                        return hashData.labelIds;
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
    Future<QList<LabelIdAndName>, AnyResultMessageCode>
        LabelsControllerImpl::convertLabelIdsToLabelIdAndNamesInternal(
                                                                    TContainer labelIds)
    {
        Future<SuccessType, AnyResultMessageCode> fetchFuture =
            fetchMissingLabelNames(labelIds);

        auto resultFuture =
            fetchFuture.thenOnEventLoop<QList<LabelIdAndName>, AnyResultMessageCode>(
                this,
                [this, labelIds](
                    ResultOrError<SuccessType, AnyResultMessageCode> outcomeOfFetch)
                    -> ResultOrError<QList<LabelIdAndName>, AnyResultMessageCode>
                {
                    if (outcomeOfFetch.failed())
                        return outcomeOfFetch.error();

                    return convertLabelIdsToLabelIdAndNamesAssumingFetched(labelIds);
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
            if (_labelData[labelId].name.isEmpty())
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

                        _labelData[labelId].name = labelName;
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
            auto name = _labelData[labelId].name;

            Q_ASSERT_X(!name.isEmpty(),
                       "LabelsControllerImpl::getLabelIdsToNamesMappingAssumingFetched",
                       "name of label is not known");

            result.insert(labelId, name);
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
            auto name = _labelData[labelId].name;

            Q_ASSERT_X(!name.isEmpty(),
                       "LabelsControllerImpl::convertLabelIdsToLabelNamesAssumingFetched",
                       "name of label is not known");

            result.append(name);
        }

        return result;
    }

    template<typename TContainer>
    QList<LabelIdAndName>
        LabelsControllerImpl::convertLabelIdsToLabelIdAndNamesAssumingFetched(
                                                                    TContainer labelIds)
    {
        QList<LabelIdAndName> result;
        result.reserve(labelIds.size());

        for (auto labelId : labelIds)
        {
            auto name = _labelData[labelId].name;

            Q_ASSERT_X(
                !name.isEmpty(),
                "LabelsControllerImpl::convertLabelIdsToLabelIdAndTextAssumingFetched",
                "name of label is not known"
            );

            result.append({ labelId, name });
        }

        return result;
    }
}

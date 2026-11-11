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
        LocalTrackId trackId, QString label)
    {
        return _connection->applyLabelToTrack(trackId, label);
    }

    SimpleFuture<AnyResultMessageCode> LabelsControllerImpl::removeLabelFromTrack(
        LocalTrackId trackId, QString label)
    {
        return _connection->removeLabelFromTrack(trackId, label);
    }

    TriBool LabelsControllerImpl::tryCheckIfTrackHasLabel(LocalTrackId trackId,
                                                          quint32 labelId)
    {
        auto trackIt = _trackData.constFind(trackId);
        if (trackIt != _trackData.constEnd())
        {
            auto& trackData = trackIt.value();

            if (trackData.fetched)
                return trackData.labelIds.contains(labelId);

            if (trackData.labelIds.contains(labelId))
                return true;

            if (trackData.labelsRemovedIds.contains(labelId))
                return false;
        }

        auto labelIt = _labelData.constFind(labelId);
        if (labelIt != _labelData.constEnd())
        {
            auto& labelData = labelIt.value();

            if (labelData.tracksFetched)
                return labelData.tracks.contains(trackId);
        }

        return TriBool::unknown;
    }

    Future<QList<QString>, AnyResultMessageCode>
        LabelsControllerImpl::getLabelNamesByTrack(LocalTrackId trackId)
    {
        auto idsFuture = getLabelsByTrackInternal(trackId);

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
        LabelsControllerImpl::getLabelsByTrack(LocalTrackId trackId)
    {
        auto idsFuture = getLabelsByTrackInternal(trackId);

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

    void LabelsControllerImpl::onTrackLabelsChanged(LocalTrackId trackId,
                                                    QList<quint32> labelsAddedIds,
                                                    QList<quint32> labelsRemovedIds)
    {
        auto& trackData = _trackData[trackId];
        auto& currentLabelIds = trackData.labelIds;

        bool haveRealChanges = false;

        for (auto labelId : labelsAddedIds)
        {
            haveRealChanges |= trackData.labelsRemovedIds.remove(labelId);

            auto oldSize = currentLabelIds.size();
            currentLabelIds.insert(labelId);
            haveRealChanges |= currentLabelIds.size() > oldSize;

            _labelData[labelId].tracks.insert(trackId);
        }

        for (auto labelId : labelsRemovedIds)
        {
            trackData.labelsRemovedIds.insert(labelId); // really necessary ???

            haveRealChanges |= currentLabelIds.remove(labelId);

            _labelData[labelId].tracks.remove(trackId);
        }

        if (haveRealChanges)
            Q_EMIT trackLabelsChanged(trackId);
    }

    void LabelsControllerImpl::ensureFetchingOfTracksForLabelInternal(quint32 labelId)
    {
        auto& labelData = _labelData[labelId];
        if (labelData.tracksFetched || labelData.fetchRequestSent)
            return;

        auto* fetcher = new TrackIdsListFetcher();

        connect(
            fetcher, &TrackIdsListFetcher::receivedIds,
            this,
            [this, labelId](QList<LocalTrackId> trackIds)
            {
                receivedTracksForLabelChunk(labelId, trackIds);
            }
        );

        connect(
            fetcher, &TrackIdsListFetcher::completed,
            this,
            [this, labelId]()
            {
                qDebug() << "LabelsControllerImpl: received tracks list for label"
                         << labelId;

                auto& labelData = _labelData[labelId];
                bool incompleteFetch = labelData.fetchResultWillBeIncomplete;
                labelData.tracksFetched = !incompleteFetch;
                labelData.fetchRequestSent = false;

                if (incompleteFetch)
                {
                    /* try fetching again if the result was incomplete */
                    ensureFetchingOfTracksForLabelInternal(labelId);
                }
                else
                {
                    Q_EMIT labelTracksChanged(labelId);
                }
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
                labelData.tracksFetched = false;
                labelData.fetchRequestSent = false;
                labelData.fetchResultWillBeIncomplete = false;
            }
        );

        _connection->fetchTracksHavingLabel(labelId, fetcher);
        labelData.fetchRequestSent = true;
        labelData.fetchResultWillBeIncomplete = false;
    }

    void LabelsControllerImpl::receivedTracksForLabelChunk(quint32 labelId,
                                                           QList<LocalTrackId> tracks)
    {
        auto& labelData = _labelData[labelId];

        for (auto& trackId : tracks)
        {
            auto& trackData = _trackData[trackId];

            if (trackData.labelIds.contains(labelId)
                || trackData.labelsRemovedIds.contains(labelId))
            {
                continue;
            }

            trackData.labelIds.insert(labelId);
            labelData.tracks.insert(trackId);
        }
    }

    Future<QSet<quint32>, AnyResultMessageCode>
        LabelsControllerImpl::getLabelsByTrackInternal(LocalTrackId trackId)
    {
        auto& trackData = _trackData[trackId];

        if (trackData.fetched)
            return FutureResult(trackData.labelIds);

        if (trackData.futureForFetching.hasValue())
            return trackData.futureForFetching.value();

        auto future =
            _connection->getLabelsOfTrack(trackId)
                .thenOnEventLoop<QSet<quint32>, AnyResultMessageCode>(
                    this,
                    [this, trackId](
                            ResultOrError<QList<quint32>, AnyResultMessageCode> outcome)
                        -> ResultOrError<QSet<quint32>, AnyResultMessageCode>
                    {
                        if (outcome.failed())
                            return outcome.error();

                        auto& trackData = _trackData[trackId];
                        auto oldSize = trackData.labelIds.size();

                        for (auto labelId : outcome.result())
                        {
                            if (trackData.labelIds.contains(labelId)
                                || trackData.labelsRemovedIds.contains(labelId))
                            {
                                continue;
                            }

                            trackData.labelIds.insert(labelId);
                            _labelData[labelId].tracks.insert(trackId);
                        }

                        trackData.labelsRemovedIds.clear();
                        trackData.futureForFetching = null;
                        trackData.fetched = true;

                        bool haveRealChanges = trackData.labelIds.size() > oldSize;

                        if (haveRealChanges)
                            Q_EMIT trackLabelsChanged(trackId);

                        return trackData.labelIds;
                    }
                );

        trackData.futureForFetching = future;

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

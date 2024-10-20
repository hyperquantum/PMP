/*
    Copyright (C) 2023-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "trackinfoproviderimpl.h"

#include "resolver.h"

namespace PMP::Server
{
    TrackInfoProviderImpl::TrackInfoProviderImpl(Resolver* resolver)
        : _resolver(resolver)
    {
        //
    }

    Future<CollectionTrackInfo, FailureType> TrackInfoProviderImpl::getTrackInfoAsync(
                                                                              uint hashId)
    {
        {
            auto trackInfo = _resolver->getHashTrackInfo(hashId);

            if (!trackInfo.titleAndArtistUnknown())
                return FutureResult(trackInfo);
        }

        qDebug() << "TrackInfoProviderImpl: will try to locate the file for hash ID"
                 << hashId;

        auto future =
            _resolver->findPathForHashAsync(hashId)
                .thenOnAnyThreadIndirect<SuccessType, FailureType>(
                    [this, hashId](FailureOr<QString> outcome) -> Future<SuccessType, FailureType>
                    {
                        if (outcome.failed())
                            return FutureError<FailureType>(failure);

                        qDebug() << "TrackInfoProviderImpl: have file for hash ID"
                                 << hashId
                                 << "and will now wait until Resolver has processed it";

                        return _resolver->waitUntilAnyFileAnalyzed(hashId);
                    }
                )
                .thenOnAnyThread<CollectionTrackInfo, FailureType>(
                    [this, hashId](SuccessOrFailure) -> FailureOr<CollectionTrackInfo>
                    {
                        qDebug() << "TrackInfoProviderImpl: will now attempt to return"
                                    " track info for hash ID" << hashId;

                        return _resolver->getHashTrackInfo(hashId);
                    }
                );

        return future;
    }
}

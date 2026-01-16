/*
    Copyright (C) 2016-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_TRACKJUDGE_H
#define PMP_TRACKJUDGE_H

#include "common/trackcriteria.h"
#include "common/tribool.h"

#include <QList>

namespace PMP::Client
{
    class CollectionTrackInfo;
    class QueueHashesMonitor;
    class UserDataFetcher;
}

namespace PMP
{
    class TrackJudge
    {
    public:
        TrackJudge(Client::UserDataFetcher& userDataFetcher,
                   Client::QueueHashesMonitor& queueHashesMonitor)
         : _criteriumTree(ConstantTrackCriterium::allTracksMatch()),
           _userDataFetcher(userDataFetcher),
           _queueHashesMonitor(queueHashesMonitor),
           _userId(0),
           _haveUserId(false),
           _criteriumTreeMatchesAllTracks(true)
        {
            //
        }

        void setUserId(quint32 userId);
        bool isUserIdSetTo(quint32 userId) const
        {
            return _userId == userId && _haveUserId;
        }

        bool setCriterium(const TrackCriterium& criterium);

        bool criteriumUsesUserData() const;
        bool criteriumResultsInAllTracks() const;

        TriBool trackSatisfiesCriteria(Client::CollectionTrackInfo const& track) const;

    private:
        class EvaluationContext;

        std::unique_ptr<TrackCriterium> _criteriumTree;
        Client::UserDataFetcher& _userDataFetcher;
        Client::QueueHashesMonitor& _queueHashesMonitor;
        quint32 _userId;
        bool _haveUserId;
        bool _criteriumTreeMatchesAllTracks;
    };
}
#endif

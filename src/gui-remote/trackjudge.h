/*
    Copyright (C) 2016-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/tribool.h"

#include <QDateTime>

#include <functional>

namespace PMP::Client
{
    class QueueHashesMonitor;
    class UserDataFetcher;
}

namespace PMP
{
    class CollectionTrackInfo;

    enum class TrackCriterium
    {
        AllTracks = 0,
        NoTracks,
        NeverHeard,
        LastHeardNotInLast1000Days,
        LastHeardNotInLast365Days,
        LastHeardNotInLast180Days,
        LastHeardNotInLast90Days,
        LastHeardNotInLast30Days,
        LastHeardNotInLast10Days,
        WithoutScore,
        ScoreMaximum30,
        ScoreAtLeast85,
        ScoreAtLeast90,
        ScoreAtLeast95,
        LengthMaximumOneMinute,
        LengthAtLeastFiveMinutes,
        NotInTheQueue,
        InTheQueue,
    };

    class TrackJudge
    {
    public:
        TrackJudge(Client::UserDataFetcher& userDataFetcher,
                   Client::QueueHashesMonitor& queueHashesMonitor)
         : _criterium1(TrackCriterium::AllTracks),
           _criterium2(TrackCriterium::AllTracks),
           _userId(0),
           _haveUserId(false),
           _userDataFetcher(userDataFetcher),
           _queueHashesMonitor(queueHashesMonitor)
        {
            //
        }

        void setUserId(quint32 userId);
        bool isUserIdSetTo(quint32 userId) const
        {
            return _userId == userId && _haveUserId;
        }

        void setCriterium1(TrackCriterium criterium) { _criterium1 = criterium; }
        void setCriterium2(TrackCriterium criterium) { _criterium2 = criterium; }

        bool criteriumUsesUserData() const;
        bool criteriumResultsInAllTracks() const;

        TriBool trackSatisfiesCriteria(CollectionTrackInfo const& track) const;

    private:
        static bool usesUserData(TrackCriterium criterium);

        TriBool trackSatisfiesCriterium(CollectionTrackInfo const& track,
                                        TrackCriterium criterium) const;

        TriBool trackSatisfiesScoreCriterium(CollectionTrackInfo const& track,
                              std::function<TriBool(int)> scorePermillageEvaluator) const;

        TriBool trackSatisfiesLastHeardDateCriterium(CollectionTrackInfo const& track,
                                   std::function<TriBool(QDateTime)> dateEvaluator) const;

        TriBool trackSatisfiesNotHeardInTheLastXDaysCriterium(
                                                         CollectionTrackInfo const& track,
                                                         int days) const;

        TrackCriterium _criterium1;
        TrackCriterium _criterium2;
        quint32 _userId;
        bool _haveUserId;
        Client::UserDataFetcher& _userDataFetcher;
        Client::QueueHashesMonitor& _queueHashesMonitor;
    };
}
#endif

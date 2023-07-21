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
    class CollectionTrackInfo;
    class QueueHashesMonitor;
    class UserDataFetcher;
}

namespace PMP
{
    enum class TrackCriterium
    {
        AllTracks = 0,
        NoTracks,
        NeverHeard,
        NotHeardInLast5Years,
        NotHeardInLast3Years,
        NotHeardInLast2Years,
        NotHeardInLastYear,
        NotHeardInLast180Days,
        NotHeardInLast90Days,
        NotHeardInLast30Days,
        NotHeardInLast10Days,
        HeardAtLeastOnce,
        WithoutScore,
        WithScore,
        ScoreLessThan30,
        ScoreLessThan50,
        ScoreAtLeast80,
        ScoreAtLeast85,
        ScoreAtLeast90,
        ScoreAtLeast95,
        LengthLessThanOneMinute,
        LengthAtLeastFiveMinutes,
        NotInTheQueue,
        InTheQueue,
        WithoutTitle,
        WithoutArtist,
        WithoutAlbum,
        NoLongerAvailable,
    };

    class TrackJudge
    {
    public:
        TrackJudge(Client::UserDataFetcher& userDataFetcher,
                   Client::QueueHashesMonitor& queueHashesMonitor)
         : _criterium1(TrackCriterium::AllTracks),
           _criterium2(TrackCriterium::AllTracks),
           _criterium3(TrackCriterium::AllTracks),
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

        bool setCriteria(TrackCriterium criterium1, TrackCriterium criterium2,
                         TrackCriterium criterium3)
        {
            if (criterium1 == _criterium1 &&
                criterium2 == _criterium2 &&
                criterium3 == _criterium3)
            {
                return false;
            }

            _criterium1 = criterium1;
            _criterium2 = criterium2;
            _criterium3 = criterium3;
            return true;
        }

        bool criteriumUsesUserData() const;
        bool criteriumResultsInAllTracks() const;

        TriBool trackSatisfiesCriteria(Client::CollectionTrackInfo const& track) const;

    private:
        static bool usesUserData(TrackCriterium criterium);
        static bool isTextFieldEmpty(QString contents);

        TriBool trackSatisfiesCriterium(Client::CollectionTrackInfo const& track,
                                        TrackCriterium criterium) const;

        TriBool trackSatisfiesScoreCriterium(Client::CollectionTrackInfo const& track,
                              std::function<TriBool(int)> scorePermillageEvaluator) const;

        TriBool trackSatisfiesLastHeardDateCriterium(
                                   Client::CollectionTrackInfo const& track,
                                   std::function<TriBool(QDateTime)> dateEvaluator) const;

        TriBool trackSatisfiesNotHeardInTheLastXDaysCriterium(
                                                 Client::CollectionTrackInfo const& track,
                                                 int days) const;

        TriBool trackSatisfiesNotHeardInTheLastXYearsCriterium(
                                                 Client::CollectionTrackInfo const& track,
                                                 int years) const;

        TrackCriterium _criterium1;
        TrackCriterium _criterium2;
        TrackCriterium _criterium3;
        quint32 _userId;
        bool _haveUserId;
        Client::UserDataFetcher& _userDataFetcher;
        Client::QueueHashesMonitor& _queueHashesMonitor;
    };
}
#endif

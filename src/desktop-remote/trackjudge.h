/*
    Copyright (C) 2016-2025, Kevin André <hyperquantum@gmail.com>

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

#include "trackcriteria.h"

#include <QDateTime>
#include <QList>
#include <QMetaType>

#include <functional>

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
         : _userId(0),
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

        bool setCriteria(QList<PredefinedTrackCriterium> criteria);

        bool criteriumUsesUserData() const;
        bool criteriumResultsInAllTracks() const;

        TriBool trackSatisfiesCriteria(Client::CollectionTrackInfo const& track) const;

    private:
        class EvaluationContext;

        static QList<PredefinedTrackCriterium> simplifyCriteria(QList<PredefinedTrackCriterium> criteria);
        static bool usesUserData(PredefinedTrackCriterium criterium);
        static bool isTextFieldEmpty(QString contents);

        TriBool trackSatisfiesCriterium(Client::CollectionTrackInfo const& track,
                                        PredefinedTrackCriterium criterium) const;

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

        TriBool trackLengthLessThanXMinutes(Client::CollectionTrackInfo const& track,
                                            int minutes) const;

        TriBool trackLengthAtLeastXMinutes(Client::CollectionTrackInfo const& track,
                                           int minutes) const;

        QList<PredefinedTrackCriterium> _criteria;
        quint32 _userId;
        bool _haveUserId;
        Client::UserDataFetcher& _userDataFetcher;
        Client::QueueHashesMonitor& _queueHashesMonitor;
    };
}

Q_DECLARE_METATYPE(PMP::PredefinedTrackCriterium)

#endif

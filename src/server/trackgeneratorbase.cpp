/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "trackgeneratorbase.h"

#include "common/util.h"

#include "randomtrackssource.h"
#include "resolver.h"
#include "trackrepetitionchecker.h"

#include <QtDebug>

#include <algorithm>

namespace PMP {

    void TrackGeneratorBase::setCriteria(const DynamicModeCriteria& criteria)
    {
        if (criteria == _criteria)
            return;

        qDebug() << "criteria changing";

        _criteria = criteria;
        criteriaChanged();
    }

    void TrackGeneratorBase::setDesiredUpcomingCount(int trackCount)
    {
        if (trackCount == _desiredUpcomingTrackCount)
            return;

        qDebug() << "target size for upcoming tracks list set to" << trackCount;

        _desiredUpcomingTrackCount = trackCount;
        desiredUpcomingCountChanged();
    }

    TrackGeneratorBase::TrackGeneratorBase(QObject* parent, RandomTracksSource* source,
                                           Resolver* resolver, History* history,
                                           TrackRepetitionChecker* repetitionChecker)
     : QObject(parent),
       _source(source),
       _resolver(resolver),
       _history(history),
       _repetitionChecker(repetitionChecker),
       _randomEngine(Util::getRandomSeed()),
       _desiredUpcomingTrackCount(0)
    {
        //
    }

    quint16 TrackGeneratorBase::getRandomPermillage()
    {
        std::uniform_int_distribution<int> range(0, 1000);
        return quint16(range(_randomEngine));
    }

    QSharedPointer<TrackGeneratorBase::Candidate> TrackGeneratorBase::createCandidate(
                                                                    const FileHash& hash)
    {
        if (hash.isNull()) {
            qWarning() << "the null hash turned up as a potential candidate";
            return nullptr;
        }

        if (!_resolver->haveFileFor(hash)) {
            qDebug() << "cannot use hash" << hash
                     << "as a candidate because we don't have a file for it";
            return nullptr;
        }

        uint id = _resolver->getID(hash);
        if (id <= 0) {
            qDebug() << "cannot use hash" << hash
                     << "as a candidate because it hasn't been registered";
            return nullptr;
        }

        const AudioData& audioData = _resolver->findAudioData(hash);

        return QSharedPointer<Candidate>::create(id, hash, audioData,
                                                 getRandomPermillage(),
                                                 getRandomPermillage());
    }

    QVector<QSharedPointer<TrackGeneratorBase::Candidate>>
        TrackGeneratorBase::takeFromSourceAndApplyFilter(
                                            int trackCount,
                                            int maxAttempts,
                                            bool allOrNothing,
                                            std::function<bool (const Candidate&)> filter)
    {
        QVector<QSharedPointer<Candidate>> tracks;
        tracks.reserve(trackCount);

        int tries = maxAttempts;

        while (tries > 0 && tracks.size() < trackCount) {
            tries--;

            auto hash = source().takeTrack();
            auto candidate = createCandidate(hash);

            if (!candidate || !filter(*candidate)) {
                source().putBackUsedTrack(hash); // "used", so it won't come back soon
                continue;
            }

            tracks.append(candidate);
        }

        if (tracks.size() == trackCount) {
            return tracks;
        }

        if (allOrNothing) {
            qDebug() << "ran out of attempts; got" << tracks.size()
                     << "out of" << trackCount << "; giving them back to the source";

            // ran out of attempts, put everything back for the next attempt
            for (auto track : tracks) {
                source().putBackUnusedTrack(track->hash());
            }

            return {};
        }
        else {
            qDebug() << "ran out of attempts; got" << tracks.size()
                     << "out of" << trackCount;

            return tracks;
        }
    }

    QVector<QSharedPointer<TrackGeneratorBase::Candidate>>
        TrackGeneratorBase::takeFromSourceAndApplyBasicFilter(int trackCount,
                                                              int maxAttempts,
                                                              bool allOrNothing)
    {
        return takeFromSourceAndApplyFilter(trackCount, maxAttempts, allOrNothing,
                                            [this](Candidate const& c) {
                                                return satisfiesBasicFilter(c);
                                            });
    }

    void TrackGeneratorBase::applyBasicFilterToQueue(
                                                 QQueue<QSharedPointer<Candidate>>& queue,
                                                 int reserveSpaceForAtLeastXElements)
    {
        if (queue.isEmpty())
            return;

        QQueue<QSharedPointer<Candidate>> filtered;
        filtered.reserve(std::max(queue.size(), reserveSpaceForAtLeastXElements));

        for (auto track : queue)
        {
            if (satisfiesBasicFilter(*track))
                filtered.append(track);
        }

        queue = filtered;
    }

    bool TrackGeneratorBase::satisfiesNonRepetition(Candidate const& candidate,
                                                    qint64 extraMarginMilliseconds)
    {
        return !_repetitionChecker->isRepetitionWhenQueued(candidate.id(),
                                                           candidate.hash(),
                                                           extraMarginMilliseconds);
    }

    QVector<QSharedPointer<TrackGeneratorBase::Candidate>>
        TrackGeneratorBase::applyFilter(QVector<QSharedPointer<Candidate>> tracks,
                                        std::function<bool (const Candidate&)> filter)
    {
        QVector<QSharedPointer<Candidate>> result;
        result.reserve(tracks.size());

        for (auto track : tracks)
        {
            if (filter(*track))
                result.append(track);
            else
                source().putBackUsedTrack(track->hash());
        }

        return result;
    }

    QVector<QSharedPointer<TrackGeneratorBase::Candidate>>
        TrackGeneratorBase::applySelectionFilter(
                                                QVector<QSharedPointer<Candidate>> tracks,
                                                int keepCount,
              std::function<int (const Candidate&, const Candidate&)> candidateComparison)
    {
        if (keepCount <= 0)
            return {};

        if (keepCount >= tracks.size())
            return tracks;

        QVector<int> sorted;
        sorted.reserve(tracks.size());
        for (int i = 0; i < tracks.size(); ++i) {
            sorted.append(i);
        }

        std::sort(
            sorted.begin(),
            sorted.end(),
            [&tracks, candidateComparison](int i1, int i2) {
                return candidateComparison(*tracks[i1], *tracks[i2]) < 0;
            }
        );

        QVector<bool> included(tracks.size());

        for (int i = 0; i < tracks.size(); ++i) {
            bool keep = i >= tracks.size() - keepCount;
            included[sorted[i]] = keep;
        }

        QVector<QSharedPointer<Candidate>> result;
        result.reserve(keepCount);

        for (int i = 0; i < tracks.size(); ++i)
        {
            if (included[i])
            {
                result.append(tracks[i]);
            }
            else
            {
                source().putBackUsedTrack(tracks[i]->hash());
            }
        }

        return result;
    }

}
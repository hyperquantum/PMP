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

#ifndef PMP_TRACKGENERATORBASE_H
#define PMP_TRACKGENERATORBASE_H

#include "common/audiodata.h"
#include "common/filehash.h"

#include "dynamicmodecriteria.h"

#include <QObject>
#include <QQueue>
#include <QSharedPointer>

#include <functional>
#include <random>

namespace PMP {

    class History;
    class PlayerQueue;
    class QueueEntry;
    class RandomTracksSource;
    class Resolver;
    class TrackRepetitionChecker;

    class TrackGeneratorBase : public QObject {
        Q_OBJECT
    public:
        virtual QVector<FileHash> getTracks(int count) = 0;

        void setCriteria(DynamicModeCriteria const& criteria);

    public Q_SLOTS:
        void setDesiredUpcomingCount(int trackCount);

    protected:
        TrackGeneratorBase(QObject* parent, RandomTracksSource* source,
                           Resolver* resolver, History* history,
                           TrackRepetitionChecker* repetitionChecker);

        class Candidate {
        public:
            Candidate(RandomTracksSource* source, uint id, FileHash const& hash,
                      AudioData const& audioData,
                      quint16 randomPermillageNumber1, quint16 randomPermillageNumber2);

            ~Candidate();

            Candidate(Candidate const&) = delete;
            Candidate& operator=(Candidate const&) = delete;

            void setUnused() { _unused = true; }

            uint id() const { return _id; }
            const FileHash& hash() const { return _hash; }

            qint64 lengthMilliseconds() const
            {
                return _audioData.trackLengthMilliseconds();
            }

            bool lengthIsLessThanXSeconds(int seconds) const
            {
                if (lengthMilliseconds() < 0)
                    return false; // unknown length doesn't count

                return lengthMilliseconds() < qint64(1000) * seconds;
            }

            quint16 randomPermillageNumber() const { return _randomPermillageNumber1; }

            quint16 randomPermillageNumber2() const { return _randomPermillageNumber2; }

        private:
            RandomTracksSource* _source;
            uint _id;
            FileHash _hash;
            AudioData _audioData;
            quint16 _randomPermillageNumber1;
            quint16 _randomPermillageNumber2;
            bool _unused;
        };

        quint16 getRandomPermillage();
        QSharedPointer<Candidate> createCandidate();

        DynamicModeCriteria const& criteria() const { return _criteria; }
        virtual void criteriaChanged() = 0;

        int desiredUpcomingCount() const { return _desiredUpcomingTrackCount; }
        virtual void desiredUpcomingCountChanged() = 0;

        History& history() const { return *_history; }

        QVector<QSharedPointer<Candidate>> takeFromSourceAndApplyFilter(
                                           int trackCount,
                                           int maxAttempts,
                                           bool allOrNothing,
                                           std::function<bool (const Candidate&)> filter);
        QVector<QSharedPointer<Candidate>> takeFromSourceAndApplyBasicFilter(
                                                                       int trackCount,
                                                                       int maxAttempts,
                                                                       bool allOrNothing);

        void applyBasicFilterToQueue(QQueue<QSharedPointer<Candidate>>& queue,
                                     int reserveSpaceForAtLeastXElements = 0);

        virtual bool satisfiesBasicFilter(Candidate const& candidate) = 0;
        bool satisfiesNonRepetition(Candidate const& candidate,
                                    qint64 extraMarginMilliseconds = 0);

        QVector<QSharedPointer<Candidate>> applyFilter(
                                           QVector<QSharedPointer<Candidate>> tracks,
                                           std::function<bool (const Candidate&)> filter);
        QVector<QSharedPointer<Candidate>> applySelectionFilter(
                                QVector<QSharedPointer<Candidate>> tracks, int keepCount,
             std::function<int (Candidate const&, Candidate const&)> candidateComparison);

    private:
        RandomTracksSource* _source;
        Resolver* _resolver;
        History* _history;
        TrackRepetitionChecker* _repetitionChecker;
        std::mt19937 _randomEngine;
        DynamicModeCriteria _criteria;
        int _desiredUpcomingTrackCount;
    };
}
#endif

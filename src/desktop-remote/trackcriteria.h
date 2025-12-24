/*
    Copyright (C) 2023-2025, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_TRACKCRITERIA_H
#define PMP_TRACKCRITERIA_H

#include <QList>

#include <memory>
#include <utility>

namespace PMP
{
    enum class PredefinedTrackCriterium
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
        LengthAtLeastOneMinute,
        LengthLessThanTwoMinutes,
        LengthAtLeastTwoMinutes,
        LengthLessThanThreeMinutes,
        LengthAtLeastThreeMinutes,
        LengthLessThanFourMinutes,
        LengthAtLeastFourMinutes,
        LengthLessThanFiveMinutes,
        LengthAtLeastFiveMinutes,
        NotInTheQueue,
        InTheQueue,
        WithoutTitle,
        WithoutArtist,
        WithoutAlbum,
        NoLongerAvailable,
    };

    class TrackCriterium;

    std::unique_ptr<TrackCriterium> convertToTrackCriterium(PredefinedTrackCriterium);

    class TrackCriteriumVisitor;

    class TrackCriterium
    {
    public:
        TrackCriterium() {}
        virtual ~TrackCriterium() {}

        virtual void accept(TrackCriteriumVisitor& visitor) const = 0;

        virtual bool usesUserData() const = 0;
    };

    class ConstantTrackCriterium;
    class TrackLengthPresenceCriterium;
    class TrackLengthComparisonCriterium;
    class TrackScorePresenceCriterium;
    class TrackScoreComparisonCriterium;
    class TrackLastHeardPresenceCriterium;
    class TrackLastHeardRecentlyCriterium;
    class TrackQueuePresenceCriterium;
    class TrackAvailabilityCriterium;
    class TrackMetaDataPresenceCriterium;
    class CompositeTrackCriterium;

    class TrackCriteriumVisitor
    {
    public:
        virtual ~TrackCriteriumVisitor() = default;

        virtual void visit(const ConstantTrackCriterium&) = 0;
        virtual void visit(const TrackLengthPresenceCriterium&) = 0;
        virtual void visit(const TrackLengthComparisonCriterium&) = 0;
        virtual void visit(const TrackScorePresenceCriterium&) = 0;
        virtual void visit(const TrackScoreComparisonCriterium&) = 0;
        virtual void visit(const TrackLastHeardPresenceCriterium&) = 0;
        virtual void visit(const TrackLastHeardRecentlyCriterium&) = 0;
        virtual void visit(const TrackQueuePresenceCriterium&) = 0;
        virtual void visit(const TrackAvailabilityCriterium&) = 0;
        virtual void visit(const TrackMetaDataPresenceCriterium&) = 0;
        virtual void visit(const CompositeTrackCriterium&) = 0;
    };

    enum class ComparisonOperator
    {
        Equal,
        NotEqual,
        LessThan,
        LessThanOrEqual,
        GreaterThan,
        GreaterThanOrEqual,
    };

    class ConstantTrackCriterium final : public TrackCriterium
    {
    public:
        static std::unique_ptr<ConstantTrackCriterium> allTracksMatch()
        {
            return std::unique_ptr<ConstantTrackCriterium>(
                new ConstantTrackCriterium(true)
            );
        }

        static std::unique_ptr<ConstantTrackCriterium> noTracksMatch()
        {
            return std::unique_ptr<ConstantTrackCriterium>(
                new ConstantTrackCriterium(false)
            );
        }

        bool value() const { return _value; }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override { return false; }

    private:
        explicit ConstantTrackCriterium(bool value) : _value(value) {}

        bool _value;
    };

    class TrackLengthPresenceCriterium final : public TrackCriterium
    {
    public:
        static std::unique_ptr<TrackLengthPresenceCriterium> lengthMustBePresent()
        {
            return std::unique_ptr<TrackLengthPresenceCriterium>(
                new TrackLengthPresenceCriterium(true)
            );
        }

        static std::unique_ptr<TrackLengthPresenceCriterium> lengthMustBeAbsent()
        {
            return std::unique_ptr<TrackLengthPresenceCriterium>(
                new TrackLengthPresenceCriterium(false)
            );
        }

        bool presence() const { return _present; }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override { return true; }

    private:
        explicit TrackLengthPresenceCriterium(bool scorePresent)
            : _present(scorePresent)
        {
            //
        }

        bool _present;
    };

    class TrackLengthComparisonCriterium : public TrackCriterium
    {
    public:
        TrackLengthComparisonCriterium();
        TrackLengthComparisonCriterium(ComparisonOperator comparisonOperator,
                                       int minutes);

        void setLengthMinutes(int minutes) { _minutes = minutes; }
        int lengthMinutes() const { return _minutes; }

        void setComparisonOperator(ComparisonOperator comparisonOperator)
        {
            _operator = comparisonOperator;
        }

        ComparisonOperator comparisonOperator() const { return _operator; }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override { return false; }

    private:
        ComparisonOperator _operator;
        int _minutes;
    };

    class TrackScorePresenceCriterium final : public TrackCriterium
    {
    public:
        static std::unique_ptr<TrackScorePresenceCriterium> scoreMustBePresent()
        {
            return std::unique_ptr<TrackScorePresenceCriterium>(
                new TrackScorePresenceCriterium(true)
            );
        }

        static std::unique_ptr<TrackScorePresenceCriterium> scoreMustBeAbsent()
        {
            return std::unique_ptr<TrackScorePresenceCriterium>(
                new TrackScorePresenceCriterium(false)
            );
        }

        bool presence() const { return _present; }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override { return true; }

    private:
        explicit TrackScorePresenceCriterium(bool scorePresent) : _present(scorePresent){}

        bool _present;
    };

    class TrackScoreComparisonCriterium : public TrackCriterium
    {
    public:
        TrackScoreComparisonCriterium();
        TrackScoreComparisonCriterium(ComparisonOperator comparisonOperator, int score);

        void setScore(int score) { _score = score; }
        int score() const { return _score; }

        void setComparisonOperator(ComparisonOperator comparisonOperator)
        {
            _operator = comparisonOperator;
        }

        ComparisonOperator comparisonOperator() const { return _operator; }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override { return true; }

    private:
        ComparisonOperator _operator;
        int _score;
    };

    class TrackLastHeardPresenceCriterium final : public TrackCriterium
    {
    public:
        static std::unique_ptr<TrackLastHeardPresenceCriterium> lastHeardMustBePresent()
        {
            return std::unique_ptr<TrackLastHeardPresenceCriterium>(
                new TrackLastHeardPresenceCriterium(true)
            );
        }

        static std::unique_ptr<TrackLastHeardPresenceCriterium> lastHeardMustBeAbsent()
        {
            return std::unique_ptr<TrackLastHeardPresenceCriterium>(
                new TrackLastHeardPresenceCriterium(false)
                );
        }

        bool presence() const { return _present; }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override { return true; }

    private:
        explicit TrackLastHeardPresenceCriterium(bool isPresent) : _present(isPresent) {}

        bool _present;
    };

    struct CompositeDuration
    {
        int years { 0 };
        int days { 0 };
    };

    class TrackLastHeardRecentlyCriterium : public TrackCriterium
    {
    public:
        TrackLastHeardRecentlyCriterium();
        TrackLastHeardRecentlyCriterium(CompositeDuration duration, bool isInverted);

        void setDuration(CompositeDuration duration) { _duration = duration; }
        CompositeDuration duration() const { return _duration; }

        void setInverted(bool isInverted) { _inverted = isInverted; }
        bool isInverted() const { return _inverted; }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override { return true; }

    private:
        CompositeDuration _duration;
        bool _inverted;
    };

    class TrackQueuePresenceCriterium final : public TrackCriterium
    {
    public:
        static std::unique_ptr<TrackQueuePresenceCriterium> mustBePresentInQueue()
        {
            return std::unique_ptr<TrackQueuePresenceCriterium>(
                new TrackQueuePresenceCriterium(true)
            );
        }

        static std::unique_ptr<TrackQueuePresenceCriterium> mustBeAbsentInQueue()
        {
            return std::unique_ptr<TrackQueuePresenceCriterium>(
                new TrackQueuePresenceCriterium(false)
            );
        }

        bool presence() const { return _present; }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override { return false; }

    private:
        explicit TrackQueuePresenceCriterium(bool present) : _present(present) {}

        bool _present;
    };

    class TrackAvailabilityCriterium final : public TrackCriterium
    {
    public:
        static std::unique_ptr<TrackAvailabilityCriterium> mustBeAvailable()
        {
            return std::unique_ptr<TrackAvailabilityCriterium>(
                new TrackAvailabilityCriterium(true)
            );
        }

        static std::unique_ptr<TrackAvailabilityCriterium> mustBeUnavailable()
        {
            return std::unique_ptr<TrackAvailabilityCriterium>(
                new TrackAvailabilityCriterium(false)
            );
        }

        bool availability() const { return _available; }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override { return false; }

    private:
        explicit TrackAvailabilityCriterium(bool available) : _available(available) {}

        bool _available;
    };

    enum class TrackMetaDataKind { Title, Artist, Album };

    class TrackMetaDataPresenceCriterium final : public TrackCriterium
    {
    public:
        static std::unique_ptr<TrackMetaDataPresenceCriterium> mustBePresent(
            TrackMetaDataKind metaDataKind)
        {
            return std::unique_ptr<TrackMetaDataPresenceCriterium>(
                new TrackMetaDataPresenceCriterium(metaDataKind, true)
            );
        }

        static std::unique_ptr<TrackMetaDataPresenceCriterium> mustBeAbsent(
            TrackMetaDataKind metaDataKind)
        {
            return std::unique_ptr<TrackMetaDataPresenceCriterium>(
                new TrackMetaDataPresenceCriterium(metaDataKind, false)
            );
        }

        TrackMetaDataKind metaDataKind() const { return _metaDataKind; }
        bool presence() const { return _present; }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override { return false; }

    private:
        explicit TrackMetaDataPresenceCriterium(TrackMetaDataKind metaDataKind,
                                                bool present)
         : _metaDataKind(metaDataKind), _present(present)
        {
            //
        }

        TrackMetaDataKind _metaDataKind;
        bool _present;
    };

    class CompositeTrackCriterium : public TrackCriterium
    {
    public:
        CompositeTrackCriterium();

        template<typename T, typename... Args>
        void add(Args&&... args)
        {
            _criteria.append(std::make_unique<T>(std::forward<Args>(args)...));
        }

        void accept(TrackCriteriumVisitor& visitor) const override
        {
            visitor.visit(*this);
        };

        bool usesUserData() const override;

    private:
        QList<std::unique_ptr<TrackCriterium>> _criteria;
    };
}
#endif

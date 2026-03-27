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

#include "trackfilterwidgets.h"

#include "common/nullable.h"
#include "common/unicodechars.h"
#include "common/util.h"

#include "clickablelabel.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <functional>

namespace PMP
{
    FilterLabelWidget::FilterLabelWidget(QWidget *parent)
     : QWidget(parent)
    {
        _label = new ClickableLabel();
        _label->setClickable(false);

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(_label);
        layout->addStretch();

        connect(
            _label, &ClickableLabel::clicked,
            this, &FilterLabelWidget::editingRequested
        );
    }

    void FilterLabelWidget::setCriterium(std::unique_ptr<TrackCriterium> criterium)
    {
        _criterium = std::move(criterium);

        if (!_criterium)
        {
            _label->clear();
            _label->setClickable(false);
            return;
        }

        bool isEditable = FilterEditorFactory::isEditable(*_criterium);
        _label->setClickable(isEditable);

        CriteriumCaptionGenerator visitor;
        _criterium->accept(visitor);
        auto caption = visitor.caption();

        _label->setText(caption);
    }

    std::unique_ptr<TrackCriterium> FilterLabelWidget::createCriterium() const
    {
        return _criterium->clone();
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const ConstantTrackCriterium& criterium)
    {
        if (criterium.value())
            _caption = tr("match any track");
        else
            _caption = tr("match no tracks");
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const TrackLengthPresenceCriterium& criterium)
    {
        if (criterium.presence())
            _caption = tr("length is known");
        else
            _caption = tr("length is unknown");
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const TrackLengthComparisonCriterium& criterium)
    {
        auto opStr = toString(criterium.comparisonOperator());

        auto hours = criterium.hours();
        auto minutes = criterium.minutes();
        auto seconds = criterium.seconds();

        Util::normalizeDuration(hours, minutes, seconds);

        if (hours > 0 && minutes == 0 && seconds == 0)
        {
            _caption = tr("length %1 %2 hour(s)").arg(opStr).arg(hours);
        }
        else if (hours == 0 && minutes > 0 && seconds == 0)
        {
            _caption = tr("length %1 %2 minute(s)").arg(opStr).arg(minutes);
        }
        else if (hours == 0 && minutes == 0 && seconds > 0)
        {
            _caption = tr("length %1 %2 second(s)").arg(opStr).arg(seconds);
        }
        else if (hours == 0)
        {
            _caption =
                tr("length %1 %2:%3")
                    .arg(opStr)
                    .arg(minutes, 2, 10, QChar('0'))
                    .arg(seconds, 2, 10, QChar('0'));
        }
        else
        {
            _caption =
                tr("length %1 %2:%3:%4")
                    .arg(opStr)
                    .arg(hours, 2, 10, QChar('0'))
                    .arg(minutes, 2, 10, QChar('0'))
                    .arg(seconds, 2, 10, QChar('0'));
        }
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const TrackScorePresenceCriterium& criterium)
    {
        if (criterium.presence())
            _caption = tr("has score");
        else
            _caption = tr("no score");
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const TrackScoreComparisonCriterium& criterium)
    {
        auto caption =
            tr("score %1 %2")
                .arg(toString(criterium.comparisonOperator()))
                .arg(criterium.score());

        _caption = caption;
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const TrackLastHeardPresenceCriterium& criterium)
    {
        if (criterium.presence())
            _caption = tr("heard at least once");
        else
            _caption = tr("never heard");
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const TrackLastHeardRecentlyCriterium& criterium)
    {
        auto isInverted = criterium.isInverted();
        auto duration = criterium.duration();

        Util::normalizeLongDuration(duration.years, duration.days, duration.hours);

        if (duration.isZero())
        {
            if (isInverted)
            {
                _caption = tr("not heard in the last 0 seconds");
            }
            else
            {
                _caption = tr("heard in the last 0 seconds");
            }
        }
        else if (duration.years > 0 && duration.days == 0 && duration.hours == 0)
        {
            if (isInverted)
            {
                _caption = tr("not heard in the last %1 year(s)").arg(duration.years);
            }
            else
            {
                _caption = tr("heard in the last %1 year(s)").arg(duration.years);
            }
        }
        else if (duration.days > 0 && duration.years == 0 && duration.hours == 0)
        {
            if (isInverted)
            {
                _caption = tr("not heard in the last %1 day(s)").arg(duration.days);
            }
            else
            {
                _caption = tr("heard in the last %1 day(s)").arg(duration.days);
            }
        }
        else if (duration.hours > 0 && duration.years == 0 && duration.days == 0)
        {
            if (isInverted)
            {
                _caption = tr("not heard in the last %1 hour(s)").arg(duration.hours);
            }
            else
            {
                _caption = tr("heard in the last %1 hour(s)").arg(duration.hours);
            }
        }
        else if (duration.days > 0 && duration.hours > 0 && duration.years == 0)
        {
            if (isInverted)
            {
                _caption = tr("not heard in the last %1 day(s) %2 hour(s)")
                               .arg(duration.days)
                               .arg(duration.hours);
            }
            else
            {
                _caption = tr("heard in the last %1 day(s) %2 hour(s)")
                               .arg(duration.days)
                               .arg(duration.hours);
            }
        }
        else if (duration.years > 0 && duration.days > 0 && duration.hours == 0)
        {
            if (isInverted)
            {
                _caption = tr("not heard in the last %1 year(s) %2 day(s)")
                               .arg(duration.years)
                               .arg(duration.days);
            }
            else
            {
                _caption = tr("heard in the last %1 year(s) %2 day(s)")
                               .arg(duration.years)
                               .arg(duration.days);
            }
        }
        else // catch-all case
        {
            if (isInverted)
            {
                _caption =
                    tr("not heard in the last %1 year(s) %2 day(s) %3 hour(s)")
                        .arg(duration.years)
                        .arg(duration.days)
                        .arg(duration.hours);
            }
            else
            {
                _caption =
                    tr("heard in the last %1 year(s) %2 day(s) %3 hour(s)")
                        .arg(duration.years)
                        .arg(duration.days)
                        .arg(duration.hours);
            }
        }
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const TrackQueuePresenceCriterium& criterium)
    {
        if (criterium.presence())
            _caption = tr("in the queue");
        else
            _caption = tr("not in the queue");
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const TrackAvailabilityCriterium& criterium)
    {
        if (criterium.availability())
            _caption = tr("available");
        else
            _caption = tr("no longer available");
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const TrackMetaDataPresenceCriterium& criterium)
    {
        switch (criterium.metaDataKind())
        {
        case TrackMetaDataKind::Title:
            if (criterium.presence())
                _caption = tr("with title");
            else
                _caption = tr("without title");
            break;
        case TrackMetaDataKind::Artist:
            if (criterium.presence())
                _caption = tr("with artist");
            else
                _caption = tr("without artist");
            break;
        case TrackMetaDataKind::Album:
            if (criterium.presence())
                _caption = tr("with album");
            else
                _caption = tr("without album");
            break;
        }
    }

    void FilterLabelWidget::CriteriumCaptionGenerator::visit(
        const CompositeTrackCriterium& criterium)
    {
        // This function should not be called, because the composite criterium cannot be
        // put inside a FilterLineWidget yet
        Q_UNREACHABLE();
    }

    QString FilterLabelWidget::CriteriumCaptionGenerator::toString(
        ComparisonOperator comparisonOperator)
    {
        QString operatorString;
        switch (comparisonOperator)
        {
        case ComparisonOperator::Equal:
            operatorString = "=";
            break;
        case ComparisonOperator::NotEqual:
            operatorString = UnicodeChars::notEqual;
            break;
        case ComparisonOperator::LessThan:
            operatorString = "<";
            break;
        case ComparisonOperator::LessThanOrEqual:
            operatorString = UnicodeChars::lessThanOrEqual;
            break;
        case ComparisonOperator::GreaterThan:
            operatorString = ">";
            break;
        case ComparisonOperator::GreaterThanOrEqual:
            operatorString = UnicodeChars::greaterThanOrEqual;
            break;
        }

        return operatorString;
    }

    // =============================================================== //

    namespace
    {
        void fillComboBoxWithComparisonOperators(QComboBox* comboBox)
        {
            comboBox->addItem("=", QVariant::fromValue(ComparisonOperator::Equal));

            comboBox->addItem(UnicodeChars::notEqual,
                              QVariant::fromValue(ComparisonOperator::NotEqual));

            comboBox->addItem("<", QVariant::fromValue(ComparisonOperator::LessThan));

            comboBox->addItem(UnicodeChars::lessThanOrEqual,
                              QVariant::fromValue(ComparisonOperator::LessThanOrEqual));

            comboBox->addItem(">", QVariant::fromValue(ComparisonOperator::GreaterThan));

            comboBox->addItem(UnicodeChars::greaterThanOrEqual,
                             QVariant::fromValue(ComparisonOperator::GreaterThanOrEqual));
        }

        void selectValue(QComboBox* comboBox, ComparisonOperator op)
        {
            int index = -1;
            switch (op)
            {
            case ComparisonOperator::Equal:
                index = 0;
                break;
            case ComparisonOperator::NotEqual:
                index = 1;
                break;
            case ComparisonOperator::LessThan:
                index = 2;
                break;
            case ComparisonOperator::LessThanOrEqual:
                index = 3;
                break;
            case ComparisonOperator::GreaterThan:
                index = 4;
                break;
            case ComparisonOperator::GreaterThanOrEqual:
                index = 5;
                break;
            }

            comboBox->setCurrentIndex(index);
        }

        Nullable<ComparisonOperator> getSelectedComparisonOperator(QComboBox* comboBox)
        {
            if (comboBox->currentIndex() < 0)
                return null;

            auto comparisonOperator =
                comboBox->currentData().value<ComparisonOperator>();

            return comparisonOperator;
        }

        inline QString filtersMenuTr(const char* text)
        {
            return QCoreApplication::translate("TrackFilterMenu", text);
        }

        void displayFiltersPopupMenu(QWidget* parent, QPoint globalPopupPosition,
                        std::function<void (std::unique_ptr<TrackCriterium>)> setFilter,
                                     Nullable<std::function<void ()>> emptyAction)
        {
            QMenu menu(parent);

            // Category: Score
            QMenu* scoreMenu = menu.addMenu(filtersMenuTr("Score"));

            scoreMenu->addAction(
                filtersMenuTr("Less than 30"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::scoreLessThanXPercent(30)); }
            );

            scoreMenu->addAction(
                filtersMenuTr("Less than 50"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::scoreLessThanXPercent(50)); }
            );

            scoreMenu->addAction(
                filtersMenuTr("At least 80"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::scoreAtLeastXPercent(80)); }
            );

            scoreMenu->addAction(
                filtersMenuTr("At least 85"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::scoreAtLeastXPercent(85)); }
            );

            scoreMenu->addAction(
                filtersMenuTr("At least 90"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::scoreAtLeastXPercent(90)); }
            );

            scoreMenu->addSeparator();

            scoreMenu->addAction(
                filtersMenuTr("Has score"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::scoreMustBePresent()); }
            );

            scoreMenu->addAction(
                filtersMenuTr("No score"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::scoreMustBeAbsent()); }
            );

            // Category: Length
            QMenu* lengthMenu = menu.addMenu(filtersMenuTr("Length"));

            lengthMenu->addAction(
                filtersMenuTr("Less than 3 minutes"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::lengthLessThanXMinutes(3)); }
            );

            lengthMenu->addAction(
                filtersMenuTr("At least 3 minutes"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::lengthAtLeastXMinutes(3)); }
            );

            lengthMenu->addAction(
                filtersMenuTr("Less than 4 minutes"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::lengthLessThanXMinutes(4)); }
            );

            lengthMenu->addAction(
                filtersMenuTr("At least 4 minutes"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::lengthAtLeastXMinutes(4)); }
            );

            lengthMenu->addAction(
                filtersMenuTr("Less than 5 minutes"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::lengthLessThanXMinutes(5)); }
            );

            lengthMenu->addAction(
                filtersMenuTr("At least 5 minutes"),
                [setFilter]() { setFilter(
                                    TrackCriteriumFactory::lengthAtLeastXMinutes(5)); }
            );

            // Category: last heard
            QMenu* lastHeardMenu = menu.addMenu(filtersMenuTr("Last heard"));

            lastHeardMenu->addAction(
                filtersMenuTr("More than 2 years ago"),
                [setFilter]() { setFilter(
                               TrackCriteriumFactory::notRecentlyHeard({ .years = 2 })); }
            );

            lastHeardMenu->addAction(
                filtersMenuTr("More than a year ago"),
                [setFilter]() { setFilter(
                               TrackCriteriumFactory::notRecentlyHeard({ .years = 1 })); }
            );

            lastHeardMenu->addAction(
                filtersMenuTr("More than 90 days ago"),
                [setFilter]() { setFilter(
                               TrackCriteriumFactory::notRecentlyHeard({ .days = 90 })); }
            );

            lastHeardMenu->addAction(
                filtersMenuTr("More than 7 days ago"),
                [setFilter]() { setFilter(
                               TrackCriteriumFactory::notRecentlyHeard({ .days = 7 })); }
            );

            lastHeardMenu->addAction(
                filtersMenuTr("More than 8 hours ago"),
                [setFilter]() { setFilter(
                               TrackCriteriumFactory::notRecentlyHeard({ .hours = 8 })); }
            );

            lastHeardMenu->addAction(
                filtersMenuTr("More than an hour ago"),
                [setFilter]() { setFilter(
                               TrackCriteriumFactory::notRecentlyHeard({ .hours = 1 })); }
            );

            lastHeardMenu->addSeparator();

            lastHeardMenu->addAction(
                filtersMenuTr("Never"),
                [setFilter]() { setFilter(TrackCriteriumFactory::neverHeard()); }
            );

            lastHeardMenu->addAction(
                filtersMenuTr("At least once"),
                [setFilter]() { setFilter(TrackCriteriumFactory::heardAtLeastOnce()); }
            );

            // Category: Metadata
            QMenu* metadataMenu = menu.addMenu(filtersMenuTr("Metadata"));

            metadataMenu->addAction(
                filtersMenuTr("With title"),
                [setFilter]() { setFilter(TrackCriteriumFactory::withTitle()); }
            );

            metadataMenu->addAction(
                filtersMenuTr("With artist"),
                [setFilter]() { setFilter(TrackCriteriumFactory::withArtist()); }
            );

            metadataMenu->addAction(
                filtersMenuTr("With album"),
                [setFilter]() { setFilter(TrackCriteriumFactory::withAlbum()); }
            );

            metadataMenu->addSeparator();

            metadataMenu->addAction(
                filtersMenuTr("Without title"),
                [setFilter]() { setFilter(TrackCriteriumFactory::withoutTitle()); }
            );

            metadataMenu->addAction(
                filtersMenuTr("Without artist"),
                [setFilter]() { setFilter(TrackCriteriumFactory::withoutArtist()); }
            );

            metadataMenu->addAction(
                filtersMenuTr("Without album"),
                [setFilter]() { setFilter(TrackCriteriumFactory::withoutAlbum()); }
            );

            // Category: Status
            QMenu* statusMenu = menu.addMenu(filtersMenuTr("Status"));

            statusMenu->addAction(
                filtersMenuTr("In queue"),
                [setFilter]() { setFilter(TrackCriteriumFactory::inTheQueue()); }
            );

            statusMenu->addAction(
                filtersMenuTr("Not in queue"),
                [setFilter]() { setFilter(TrackCriteriumFactory::notInTheQueue()); }
            );

            statusMenu->addSeparator();

            statusMenu->addAction(
                filtersMenuTr("Available"),
                [setFilter]() { setFilter(TrackCriteriumFactory::available()); }
            );

            statusMenu->addAction(
                filtersMenuTr("Unavailable"),
                [setFilter]() { setFilter(TrackCriteriumFactory::unavailable()); }
            );

            // The empty entry
            if (emptyAction.hasValue())
            {
                menu.addSeparator();
                menu.addAction(
                    filtersMenuTr("(empty)"),
                    [emptyAction]() { emptyAction.value()(); }
                );
            }

            menu.exec(globalPopupPosition);
        }
    }

    // =============================================================== //

    ScoreComparisonEditorWidget::ScoreComparisonEditorWidget(QWidget* parent)
     : FilterEditorWidget(parent)
    {
        auto* scoreLabel = new QLabel(tr("score"));
        _operatorComboBox = new QComboBox();
        _scoreSpinBox = new QSpinBox();

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(scoreLabel);
        layout->addWidget(_operatorComboBox);
        layout->addWidget(_scoreSpinBox);
        layout->addStretch();

        fillComboBoxWithComparisonOperators(_operatorComboBox);

        connect(
            _operatorComboBox, &QComboBox::currentIndexChanged,
            this, &ScoreComparisonEditorWidget::criteriumChanged
        );

        connect(
            _scoreSpinBox, &QSpinBox::valueChanged,
            this, &ScoreComparisonEditorWidget::criteriumChanged
        );
    }

    void ScoreComparisonEditorWidget::setOperator(ComparisonOperator comparisonOperator)
    {
        selectValue(_operatorComboBox, comparisonOperator);
    }

    void ScoreComparisonEditorWidget::setScore(int score)
    {
        _scoreSpinBox->setValue(score);
    }

    std::unique_ptr<TrackCriterium> ScoreComparisonEditorWidget::createCriterium() const
    {
        auto comparisonOperator = getSelectedComparisonOperator(_operatorComboBox);

        if (comparisonOperator == null)
            return ConstantTrackCriterium::noTracksMatch();

        auto score = _scoreSpinBox->value();

        return std::make_unique<TrackScoreComparisonCriterium>(comparisonOperator.value(),
                                                               score);
    }

    // =============================================================== //

    LastHeardEditorWidget::LastHeardEditorWidget(QWidget* parent)
     : FilterEditorWidget(parent)
    {
        _inversionComboBox = new QComboBox();
        _yearsSpinBox = new QSpinBox();
        auto* yearsLabel = new QLabel(tr("years"));
        _daysSpinBox = new QSpinBox();
        auto* daysLabel = new QLabel(tr("days"));
        _hoursSpinBox = new QSpinBox();
        auto* hoursLabel = new QLabel(tr("hours"));

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(_inversionComboBox);
        layout->addWidget(_yearsSpinBox);
        layout->addWidget(yearsLabel);
        layout->addWidget(_daysSpinBox);
        layout->addWidget(daysLabel);
        layout->addWidget(_hoursSpinBox);
        layout->addWidget(hoursLabel);
        layout->addStretch();

        _inversionComboBox->addItem(tr("heard within"));
        _inversionComboBox->addItem(tr("not heard within"));

        _daysSpinBox->setMaximum(999);

        connect(
            _inversionComboBox, &QComboBox::currentIndexChanged,
            this, &ScoreComparisonEditorWidget::criteriumChanged
        );

        connect(
            _yearsSpinBox, &QSpinBox::valueChanged,
            this, [this]() { if (!_suspendChangeSignal) Q_EMIT criteriumChanged(); }
        );

        connect(
            _daysSpinBox, &QSpinBox::valueChanged,
            this, [this]() { if (!_suspendChangeSignal) Q_EMIT criteriumChanged(); }
        );

        connect(
            _hoursSpinBox, &QSpinBox::valueChanged,
            this, [this]() { if (!_suspendChangeSignal) Q_EMIT criteriumChanged(); }
        );
    }

    void LastHeardEditorWidget::setInverted(bool isInverted)
    {
        _inversionComboBox->setCurrentIndex(isInverted ? 1 : 0);
    }

    void LastHeardEditorWidget::setPeriod(int years, int days, int hours)
    {
        Util::normalizeLongDuration(years, days, hours);

        Q_ASSERT_X(years >= 0 && years < 100,
                   "LastHeardEditorWidget::setPeriod",
                   "years out of range");

        Q_ASSERT_X(days >= 0 && days < 1000,
                   "LastHeardEditorWidget::setPeriod",
                   "days out of range");

        Q_ASSERT_X(hours >= 0 && hours < 100,
                   "LastHeardEditorWidget::setPeriod",
                   "hours out of range");

        _suspendChangeSignal++;

        _yearsSpinBox->setValue(years);
        _daysSpinBox->setValue(days);
        _hoursSpinBox->setValue(hours);

        _suspendChangeSignal--;

        Q_EMIT criteriumChanged();
    }

    std::unique_ptr<TrackCriterium> LastHeardEditorWidget::createCriterium() const
    {
        auto inversionIndex = _inversionComboBox->currentIndex();

        if (inversionIndex < 0)
            return ConstantTrackCriterium::noTracksMatch();

        bool isInverted = inversionIndex == 1;

        auto duration =
            CompositeDuration
            {
                .years = _yearsSpinBox->value(),
                .days = _daysSpinBox->value(),
                .hours = _hoursSpinBox->value()
            };

        return std::make_unique<TrackLastHeardRecentlyCriterium>(duration, isInverted);
    }

    // =============================================================== //

    LengthComparisonEditorWidget::LengthComparisonEditorWidget(QWidget* parent)
     : FilterEditorWidget(parent)
    {
        auto* lengthLabel = new QLabel(tr("length"));
        _operatorComboBox = new QComboBox();
        _hoursSpinBox = new QSpinBox();
        auto* hoursLabel = new QLabel(tr("h"));
        _minutesSpinBox = new QSpinBox();
        auto* minutesLabel = new QLabel(tr("min."));
        _secondsSpinBox = new QSpinBox();
        auto* secondsLabel = new QLabel(tr("s"));

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(lengthLabel);
        layout->addWidget(_operatorComboBox);
        layout->addWidget(_hoursSpinBox);
        layout->addWidget(hoursLabel);
        layout->addWidget(_minutesSpinBox);
        layout->addWidget(minutesLabel);
        layout->addWidget(_secondsSpinBox);
        layout->addWidget(secondsLabel);
        layout->addStretch();

        fillComboBoxWithComparisonOperators(_operatorComboBox);

        connect(
            _operatorComboBox, &QComboBox::currentIndexChanged,
            this, &ScoreComparisonEditorWidget::criteriumChanged
            );

        connect(
            _hoursSpinBox, &QSpinBox::valueChanged,
            this, [this]() { if (!_suspendChangeSignal) Q_EMIT criteriumChanged(); }
        );

        connect(
            _minutesSpinBox, &QSpinBox::valueChanged,
            this, [this]() { if (!_suspendChangeSignal) Q_EMIT criteriumChanged(); }
        );

        connect(
            _secondsSpinBox, &QSpinBox::valueChanged,
            this, [this]() { if (!_suspendChangeSignal) Q_EMIT criteriumChanged(); }
        );
    }

    void LengthComparisonEditorWidget::setOperator(ComparisonOperator comparisonOperator)
    {
        selectValue(_operatorComboBox, comparisonOperator);
    }

    void LengthComparisonEditorWidget::setLength(int hours, int minutes, int seconds)
    {
        Util::normalizeDuration(hours, minutes, seconds);

        Q_ASSERT_X(hours >= 0 && hours < 100,
                   "LengthComparisonEditorWidget::setLength",
                   "hours out of range");

        Q_ASSERT_X(minutes >= 0 && minutes < 100,
                   "LengthComparisonEditorWidget::setLength",
                   "minutes out of range");

        Q_ASSERT_X(seconds >= 0 && seconds < 100,
                   "LengthComparisonEditorWidget::setLength",
                   "seconds out of range");

        _suspendChangeSignal++;

        _hoursSpinBox->setValue(hours);
        _minutesSpinBox->setValue(minutes);
        _secondsSpinBox->setValue(seconds);

        _suspendChangeSignal--;

        Q_EMIT criteriumChanged();
    }

    std::unique_ptr<TrackCriterium> LengthComparisonEditorWidget::createCriterium() const
    {
        auto comparisonOperator = getSelectedComparisonOperator(_operatorComboBox);

        if (comparisonOperator == null)
            return ConstantTrackCriterium::noTracksMatch();

        int hours = _hoursSpinBox->value();
        int minutes = _minutesSpinBox->value();
        int seconds = _secondsSpinBox->value();

        return std::make_unique<TrackLengthComparisonCriterium>(
            comparisonOperator.value(), hours, minutes, seconds);
    }

    // =============================================================== //

    bool FilterEditorFactory::isEditable(const TrackCriterium& criterium)
    {
        IsEditableVisitor visitor;
        criterium.accept(visitor);
        return visitor.isCriteriumEditable();
    }

    FilterEditorWidget* FilterEditorFactory::createFromCriterium(QWidget* parent,
                                                          const TrackCriterium& criterium)
    {
        EditorWidgetCreationVisitor visitor(parent);
        criterium.accept(visitor);
        return visitor.editorWidget();
    }

    void FilterEditorFactory::IsEditableVisitor::visit(const ConstantTrackCriterium&)
    {
        _isEditable = false;
    }

    void FilterEditorFactory::IsEditableVisitor::visit(
        const TrackLengthPresenceCriterium&)
    {
        _isEditable = false;
    }

    void FilterEditorFactory::IsEditableVisitor::visit(
        const TrackLengthComparisonCriterium&)
    {
        _isEditable = true;
    }

    void FilterEditorFactory::IsEditableVisitor::visit(const TrackScorePresenceCriterium&)
    {
        _isEditable = false;
    }

    void FilterEditorFactory::IsEditableVisitor::visit(
        const TrackScoreComparisonCriterium&)
    {
        _isEditable = true;
    }

    void FilterEditorFactory::IsEditableVisitor::visit(
        const TrackLastHeardPresenceCriterium&)
    {
        _isEditable = false;
    }

    void FilterEditorFactory::IsEditableVisitor::visit(
        const TrackLastHeardRecentlyCriterium&)
    {
        _isEditable = true;
    }

    void FilterEditorFactory::IsEditableVisitor::visit(const TrackQueuePresenceCriterium&)
    {
        _isEditable = false;
    }

    void FilterEditorFactory::IsEditableVisitor::visit(const TrackAvailabilityCriterium&)
    {
        _isEditable = false;
    }

    void FilterEditorFactory::IsEditableVisitor::visit(
        const TrackMetaDataPresenceCriterium&)
    {
        _isEditable = false;
    }

    void FilterEditorFactory::IsEditableVisitor::visit(const CompositeTrackCriterium&)
    {
        _isEditable = false;
    }

    FilterEditorFactory::EditorWidgetCreationVisitor::EditorWidgetCreationVisitor(
        QWidget* parent)
     : _parent(parent),
       _editorWidget(nullptr)
    {
        //
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const ConstantTrackCriterium&)
    {
        _editorWidget = nullptr;
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const TrackLengthPresenceCriterium&)
    {
        _editorWidget = nullptr;
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const TrackLengthComparisonCriterium& lengthCriterium)
    {
        auto* editor = new LengthComparisonEditorWidget(_parent);
        editor->setOperator(lengthCriterium.comparisonOperator());
        editor->setLength(lengthCriterium.hours(),
                          lengthCriterium.minutes(),
                          lengthCriterium.seconds());

        _editorWidget = editor;
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const TrackScorePresenceCriterium&)
    {
        _editorWidget = nullptr;
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const TrackScoreComparisonCriterium& scoreComparisonCriterium)
    {
        auto* editor = new ScoreComparisonEditorWidget(_parent);
        editor->setOperator(scoreComparisonCriterium.comparisonOperator());
        editor->setScore(scoreComparisonCriterium.score());

        _editorWidget = editor;
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const TrackLastHeardPresenceCriterium&)
    {
        _editorWidget = nullptr;
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const TrackLastHeardRecentlyCriterium& criterium)
    {
        auto period = criterium.duration();

        auto* editor = new LastHeardEditorWidget(_parent);
        editor->setPeriod(period.years, period.days, period.hours);
        editor->setInverted(criterium.isInverted());

        _editorWidget = editor;
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const TrackQueuePresenceCriterium&)
    {
        _editorWidget = nullptr;
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const TrackAvailabilityCriterium&)
    {
        _editorWidget = nullptr;
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const TrackMetaDataPresenceCriterium&)
    {
        _editorWidget = nullptr;
    }

    void FilterEditorFactory::EditorWidgetCreationVisitor::visit(
        const CompositeTrackCriterium&)
    {
        _editorWidget = nullptr;
    }

    // =============================================================== //

    FilterLineWidget::FilterLineWidget()
    {
        init(); // default initialization to empty filter
    }

    FilterLineWidget::FilterLineWidget(std::unique_ptr<TrackCriterium> criterium)
    {
        init(); // default initialization to empty filter

        switchToLabel(_emptyFilterLabel, std::move(criterium));
    }

    FilterLineWidget::FilterLineWidget(FilterEditorWidget* editor)
    {
        init(); // default initialization to empty filter

        switchToEditor(_emptyFilterLabel, editor);
    }

    void FilterLineWidget::init()
    {
        _emptyFilterLabel = new ClickableLabel();
        _labelWidget = nullptr;
        _editorWidget = nullptr;
        _editButton = new QPushButton();
        _doneButton = new QPushButton();
        _deleteButton = new QPushButton();
        _resetButton = new QPushButton();
        _isEmpty = true;

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(_emptyFilterLabel, 1);
        layout->addWidget(_editButton, 0);
        layout->addWidget(_doneButton, 0);
        layout->addWidget(_deleteButton, 0);
        layout->addWidget(_resetButton, 0);

        _emptyFilterLabel->setText(tr("(empty)"));

        _editButton->setIcon(
            QIcon::fromTheme("document-edit",
                             style()->standardIcon(QStyle::SP_FileDialogDetailedView)));
        _editButton->setToolTip(tr("Edit filter"));

        _doneButton->setIcon(
            QIcon::fromTheme("dialog-ok-apply",
                             style()->standardIcon(QStyle::SP_DialogApplyButton)));
        _doneButton->setToolTip(tr("Done editing"));

        _deleteVisible = true;
        _deleteButton->setIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton));
        _deleteButton->setToolTip(tr("Remove filter"));

        _resetVisible = true;
        _resetButton->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));
        _resetButton->setToolTip(tr("Clear filter"));

        _editButton->setVisible(false);
        _doneButton->setVisible(false);
        _resetButton->setEnabled(false);

        connect(
            _emptyFilterLabel, &ClickableLabel::clicked,
            this, &FilterLineWidget::onEmptyLabelClicked
        );

        connect(
            _editButton, &QPushButton::clicked,
            this, &FilterLineWidget::onEditClicked
        );

        connect(
            _doneButton, &QPushButton::clicked,
            this, [this]() { switchEditorToLabel(); }
        );

        connect(
            _deleteButton, &QPushButton::clicked,
            this, [this]() { Q_EMIT deleteClicked(); }
        );

        connect(
            _resetButton, &QPushButton::clicked,
            this, &FilterLineWidget::onResetClicked
        );
    }

    std::unique_ptr<TrackCriterium> FilterLineWidget::createCriterium() const
    {
        if (_labelWidget)
            return _labelWidget->createCriterium();

        if (_editorWidget)
            return _editorWidget->createCriterium();

        Q_ASSERT_X(_isEmpty,
                   "FilterLineWidget::createCriterium",
                   "should be empty at his point");

        return ConstantTrackCriterium::noTracksMatch();
    }

    void FilterLineWidget::setDeleteButtonVisible(bool visible)
    {
        if (_deleteVisible == visible)
            return;

        _deleteVisible = visible;
        _deleteButton->setVisible(_deleteVisible);
    }

    void FilterLineWidget::setResetButtonVisible(bool visible)
    {
        if (_resetVisible == visible)
            return;

        _resetVisible = visible;
        _resetButton->setVisible(_resetVisible);
    }

    void FilterLineWidget::onEmptyLabelClicked(QPoint position)
    {
        QPoint globalPosition = _emptyFilterLabel->mapToGlobal(position);

        displayFiltersPopupMenu(
            this,
            globalPosition,
            [this](auto criterium)
            {
                switchToLabel(_emptyFilterLabel, std::move(criterium));

                Q_EMIT criteriumChanged();
            },
            null /* do not display 'empty' */
        );
    }

    void FilterLineWidget::onEditClicked()
    {
        Q_ASSERT_X(_labelWidget != nullptr,
                   "FilterLineWidget::onEditClicked",
                   "label widget must be present");

        switchLabelToEditor();
    }

    void FilterLineWidget::onResetClicked()
    {
        clearCriterium();
    }

    void FilterLineWidget::clearCriterium()
    {
        if (_isEmpty)
            return;

        if (_labelWidget)
        {
            switchToEmpty(_labelWidget);

            _labelWidget->deleteLater();
            _labelWidget = nullptr;
        }
        else if (_editorWidget)
        {
            switchToEmpty(_editorWidget);

            _editorWidget->deleteLater();
            _editorWidget = nullptr;
        }
        else
        {
            Q_UNREACHABLE();
        }

        Q_EMIT criteriumChanged();
    }

    void FilterLineWidget::switchToEmpty(QWidget* widgetToReplace)
    {
        layout()->replaceWidget(widgetToReplace, _emptyFilterLabel);
        widgetToReplace->setVisible(false);

        _isEmpty = true;

        _emptyFilterLabel->setVisible(true);
        _editButton->setVisible(false);
        _doneButton->setVisible(false);
        _resetButton->setEnabled(false);
    }

    void FilterLineWidget::switchEditorToLabel()
    {
        Q_ASSERT_X(_editorWidget != nullptr,
                   "FilterLineWidget::switchEditorToLabel",
                   "editor not present!");

        auto criterium = _editorWidget->createCriterium();

        switchToLabel(_editorWidget, std::move(criterium));

        _editorWidget->deleteLater();
        _editorWidget = nullptr;
    }

    void FilterLineWidget::switchToLabel(QWidget* widgetToReplace,
                                         std::unique_ptr<TrackCriterium> criterium)
    {
        Q_ASSERT_X(_labelWidget == nullptr,
                   "FilterLineWidget::switchToLabel",
                   "label widget already present!");

        bool isEditable = FilterEditorFactory::isEditable(*criterium);

        _labelWidget = new FilterLabelWidget(nullptr);
        _labelWidget->setCriterium(std::move(criterium));

        connect(
            _labelWidget, &FilterLabelWidget::editingRequested,
            this, [this] { switchLabelToEditor(); }
        );

        layout()->replaceWidget(widgetToReplace, _labelWidget);
        widgetToReplace->setVisible(false);

        _isEmpty = false;

        _editButton->setVisible(isEditable);
        _doneButton->setVisible(false);
        _resetButton->setEnabled(true);
    }

    void FilterLineWidget::switchLabelToEditor()
    {
        Q_ASSERT_X(_labelWidget != nullptr,
                   "FilterLineWidget::switchLabelToEditor",
                   "label widget not present!");

        auto criterium = _labelWidget->createCriterium();

        switchToEditor(_labelWidget, *criterium);

        _labelWidget->deleteLater();
        _labelWidget = nullptr;
    }

    void FilterLineWidget::switchToEditor(QWidget* widgetToReplace,
                                          const TrackCriterium& criterium)
    {
        switchToEditor(widgetToReplace,
                       FilterEditorFactory::createFromCriterium(nullptr, criterium));
    }

    void FilterLineWidget::switchToEditor(QWidget *widgetToReplace,
                                          FilterEditorWidget* editor)
    {
        Q_ASSERT_X(_editorWidget == nullptr,
                   "FilterLineWidget::switchToEditor",
                   "editor already present!");

        _editorWidget = editor;

        Q_ASSERT_X(_editorWidget != nullptr,
                   "FilterLineWidget::switchToEditor",
                   "failed to obtain editor for criterium");

        connect(
            _editorWidget, &FilterEditorWidget::criteriumChanged,
            this, &FilterLineWidget::criteriumChanged
        );

        layout()->replaceWidget(widgetToReplace, _editorWidget);
        widgetToReplace->setVisible(false);

        _isEmpty = false;

        _editButton->setVisible(false);
        _doneButton->setVisible(true);
        _resetButton->setEnabled(true);
    }

    // =============================================================== //

    FiltersListWidget::FiltersListWidget()
    {
        _verticalLayout = new QVBoxLayout(this);
        _verticalLayout->setContentsMargins(0, 0, 0, 0);

        auto* buttonsLayout = new QHBoxLayout();
        _verticalLayout->addLayout(buttonsLayout);

        _addMenuButton = new QPushButton(tr("Add…"));
        _addMenuButton->setToolTip(tr("Add filter"));

        buttonsLayout->addWidget(_addMenuButton);
        buttonsLayout->addStretch();

        connect(
            _addMenuButton, &QPushButton::clicked,
            this, &FiltersListWidget::showAddMenu
        );
    }

    std::unique_ptr<TrackCriterium> FiltersListWidget::createCriterium() const
    {
        auto compositeCriterium = std::make_unique<CompositeTrackCriterium>();

        for (auto const* filterLine : _filters)
        {
            if (filterLine->isEmpty())
                continue; // skip empty filter

            compositeCriterium->add(filterLine->createCriterium());
        }

        return compositeCriterium;
    }

    void FiltersListWidget::showAddMenu()
    {
        // Show menu below the button
        QPoint pos = _addMenuButton->mapToGlobal(QPoint(0, _addMenuButton->height()));

        displayFiltersPopupMenu(
            this,
            pos,
            [this](auto criterium) { addFilterLine(std::move(criterium)); },
            { [this]() { addFilterLine(new FilterLineWidget()); } } /* add empty filter */
        );
    }

    void FiltersListWidget::addFilterLine(std::unique_ptr<TrackCriterium> criterium)
    {
        addFilterLine(new FilterLineWidget(std::move(criterium)));
    }

    void FiltersListWidget::addFilterLine(FilterLineWidget* filterLine)
    {
        auto index = _filters.size();
        _verticalLayout->insertWidget(index, filterLine);

        _filters.append(filterLine);

        connect(
            filterLine, &FilterLineWidget::criteriumChanged,
            this,
            [this]()
            {
                Q_EMIT criteriumChanged();
            }
        );

        connect(
            filterLine, &FilterLineWidget::deleteClicked,
            this,
            [this, filterLine]()
            {
                auto index = _filters.indexOf(filterLine);
                Q_ASSERT_X(
                    index >= 0,
                    "FiltersListWidget::addFilterLine",
                    "filter to be deleted not found"
                );

                _filters.removeAt(index);
                filterLine->deleteLater();

                Q_EMIT criteriumChanged();
            }
        );

        Q_EMIT criteriumChanged();
    }
}

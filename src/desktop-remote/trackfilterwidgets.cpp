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
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace PMP
{
    FilterLabelWidget::FilterLabelWidget(QWidget *parent)
     : QWidget(parent)
    {
        _label = new ClickableLabel();

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
            return;
        }

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
            _caption = tr("with score");
        else
            _caption = tr("without score");
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
        else if (duration.years > 0 && duration.days == 0)
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
        else if (duration.days > 0 && duration.years == 0)
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
        else // catch-all case
        {
            if (isInverted)
            {
                _caption =
                    tr("not heard in the last %1 year(s) %2 day(s)")
                        .arg(duration.years)
                        .arg(duration.days);
            }
            else
            {
                _caption =
                    tr("heard in the last %1 year(s) %2 day(s)")
                        .arg(duration.years)
                        .arg(duration.days);
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

    FilterPickerWidget::FilterPickerWidget(PredefinedTrackCriterium criteriumForEmpty,
                                           QString captionForEmpty)
     : _predefinedCriterium(criteriumForEmpty)
    {
        _comboBox = new QComboBox();

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(_comboBox);

        fillTrackCriteriaComboBox(_comboBox, criteriumForEmpty, captionForEmpty);

        connect(
            _comboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this]()
            {
                auto predefinedCriterium =
                    _comboBox->currentData().value<PredefinedTrackCriterium>();

                if (_predefinedCriterium == predefinedCriterium)
                    return;

                _predefinedCriterium = predefinedCriterium;
                Q_EMIT criteriumChanged();
            }
        );
    }

    void FilterPickerWidget::clearCriterium()
    {
        _comboBox->setCurrentIndex(0);
    }

    std::unique_ptr<TrackCriterium> FilterPickerWidget::createCriterium() const
    {
        return convertToTrackCriterium(_predefinedCriterium);
    }

    void FilterPickerWidget::fillTrackCriteriaComboBox(QComboBox* comboBox,
                                            PredefinedTrackCriterium criteriumForEmpty,
                                                       QString captionForEmpty)
    {
        auto addItem =
            [comboBox](QString text, PredefinedTrackCriterium mode)
            {
                text.replace(">=", UnicodeChars::greaterThanOrEqual)
                    .replace("<=", UnicodeChars::lessThanOrEqual);

                comboBox->addItem(text, QVariant::fromValue(mode));
            };

        addItem(captionForEmpty, criteriumForEmpty);

        addItem(tr("never heard"), PredefinedTrackCriterium::NeverHeard);
        addItem(tr("not heard in the last 5 years"),
                PredefinedTrackCriterium::NotHeardInLast5Years);
        addItem(tr("not heard in the last 3 years"),
                PredefinedTrackCriterium::NotHeardInLast3Years);
        addItem(tr("not heard in the last 2 years"),
                PredefinedTrackCriterium::NotHeardInLast2Years);
        addItem(tr("not heard in the last year"),
                PredefinedTrackCriterium::NotHeardInLastYear);
        addItem(tr("not heard in the last 180 days"),
                PredefinedTrackCriterium::NotHeardInLast180Days);
        addItem(tr("not heard in the last 90 days"),
                PredefinedTrackCriterium::NotHeardInLast90Days);
        addItem(tr("not heard in the last 30 days"),
                PredefinedTrackCriterium::NotHeardInLast30Days);
        addItem(tr("not heard in the last 10 days"),
                PredefinedTrackCriterium::NotHeardInLast10Days);
        addItem(tr("heard at least once"), PredefinedTrackCriterium::HeardAtLeastOnce);

        addItem(tr("without score"), PredefinedTrackCriterium::WithoutScore);
        addItem(tr("with score"), PredefinedTrackCriterium::WithScore);
        addItem(tr("score < 30"), PredefinedTrackCriterium::ScoreLessThan30);
        addItem(tr("score < 50"), PredefinedTrackCriterium::ScoreLessThan50);
        addItem(tr("score >= 80"), PredefinedTrackCriterium::ScoreAtLeast80);
        addItem(tr("score >= 85"), PredefinedTrackCriterium::ScoreAtLeast85);
        addItem(tr("score >= 90"), PredefinedTrackCriterium::ScoreAtLeast90);
        addItem(tr("score >= 95"), PredefinedTrackCriterium::ScoreAtLeast95);

        addItem(tr("length < 1 min."), PredefinedTrackCriterium::LengthLessThanOneMinute);
        addItem(tr("length >= 1 min."), PredefinedTrackCriterium::LengthAtLeastOneMinute);
        addItem(tr("length < 2 min."), PredefinedTrackCriterium::LengthLessThanTwoMinutes);
        addItem(tr("length >= 2 min."), PredefinedTrackCriterium::LengthAtLeastTwoMinutes);
        addItem(tr("length < 3 min."), PredefinedTrackCriterium::LengthLessThanThreeMinutes);
        addItem(tr("length >= 3 min."), PredefinedTrackCriterium::LengthAtLeastThreeMinutes);
        addItem(tr("length < 4 min."), PredefinedTrackCriterium::LengthLessThanFourMinutes);
        addItem(tr("length >= 4 min."), PredefinedTrackCriterium::LengthAtLeastFourMinutes);
        addItem(tr("length < 5 min."), PredefinedTrackCriterium::LengthLessThanFiveMinutes);
        addItem(tr("length >= 5 min."), PredefinedTrackCriterium::LengthAtLeastFiveMinutes);

        addItem(tr("not in the queue"), PredefinedTrackCriterium::NotInTheQueue);
        addItem(tr("in the queue"), PredefinedTrackCriterium::InTheQueue);

        addItem(tr("without title"), PredefinedTrackCriterium::WithoutTitle);
        addItem(tr("without artist"), PredefinedTrackCriterium::WithoutArtist);
        addItem(tr("without album"), PredefinedTrackCriterium::WithoutAlbum);

        addItem(tr("no longer available"), PredefinedTrackCriterium::NoLongerAvailable);

        comboBox->setCurrentIndex(0);
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

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(_inversionComboBox);
        layout->addWidget(_yearsSpinBox);
        layout->addWidget(yearsLabel);
        layout->addWidget(_daysSpinBox);
        layout->addWidget(daysLabel);
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
    }

    void LastHeardEditorWidget::setInverted(bool isInverted)
    {
        _inversionComboBox->setCurrentIndex(isInverted ? 1 : 0);
    }

    void LastHeardEditorWidget::setPeriod(int years, int days)
    {
        Util::normalizeLongDuration(years, days);

        Q_ASSERT_X(years >= 0 && years < 100,
                   "LastHeardEditorWidget::setPeriod",
                   "years out of range");

        Q_ASSERT_X(days >= 0 && days < 1000,
                   "LastHeardEditorWidget::setPeriod",
                   "days out of range");

        _suspendChangeSignal++;

        _yearsSpinBox->setValue(years);
        _daysSpinBox->setValue(days);

        _suspendChangeSignal--;

        Q_EMIT criteriumChanged();
    }

    std::unique_ptr<TrackCriterium> LastHeardEditorWidget::createCriterium() const
    {
        auto inversionIndex = _inversionComboBox->currentIndex();

        if (inversionIndex < 0)
            return ConstantTrackCriterium::noTracksMatch();

        bool isInverted = inversionIndex == 1;

        int years = _yearsSpinBox->value();
        int days = _daysSpinBox->value();

        return std::make_unique<TrackLastHeardRecentlyCriterium>(
            CompositeDuration { .years = years, .days = days }, isInverted);
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

    FilterEditorWidget* FilterEditorFactory::createEditor(QWidget* parent,
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
        editor->setPeriod(period.years, period.days);
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
     : _labelWidget(nullptr),
        _editorWidget(nullptr)
    {
        _filterPicker = new FilterPickerWidget(PredefinedTrackCriterium::AllTracks,
                                               tr("(empty)"));
        _editButton = new QPushButton();
        _okButton = new QPushButton();
        _deleteButton = new QPushButton();
        _resetButton = new QPushButton();

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(_filterPicker, 1);
        layout->addWidget(_editButton, 0);
        layout->addWidget(_okButton, 0);
        layout->addWidget(_deleteButton, 0);
        layout->addWidget(_resetButton, 0);

        _editButton->setText(tr("Edit"));
        _editButton->setEnabled(false);

        _okButton->setText(tr("OK"));
        _okButton->setVisible(false);

        _deleteButton->setIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton));
        _deleteButton->setToolTip(tr("Remove filter"));

        _resetButton->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));
        _resetButton->setToolTip(tr("Clear filter"));

        connect(
            _filterPicker, &FilterPickerWidget::criteriumChanged,
            this, &FilterLineWidget::onPickerCriteriumChanged
        );

        connect(
            _editButton, &QPushButton::clicked,
            this, &FilterLineWidget::onEditClicked
        );

        connect(
            _okButton, &QPushButton::clicked,
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

        return _filterPicker->createCriterium();
    }


    void FilterLineWidget::onPickerCriteriumChanged()
    {
        if (_labelWidget || _editorWidget)
            return;

        auto criterium = _filterPicker->createCriterium();
        bool isEditable = FilterEditorFactory::isEditable(*criterium);
        _editButton->setEnabled(isEditable);

        Q_EMIT criteriumChanged();
    }

    void FilterLineWidget::onEditClicked()
    {
        if (_labelWidget)
        {
            switchLabelToEditor();
        }
        else
        {
            switchPickerToEditor();
        }
    }

    void FilterLineWidget::onResetClicked()
    {
        if (_labelWidget)
        {
            layout()->replaceWidget(_labelWidget, _filterPicker);
            _labelWidget->setVisible(false);
            _labelWidget->deleteLater();
            _labelWidget = nullptr;
        }

        if (_editorWidget)
        {
            layout()->replaceWidget(_editorWidget, _filterPicker);
            _editorWidget->setVisible(false);
            _editorWidget->deleteLater();
            _editorWidget = nullptr;
        }

        _filterPicker->clearCriterium();
        _filterPicker->setVisible(true);
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

        _labelWidget = new FilterLabelWidget(nullptr);
        _labelWidget->setCriterium(std::move(criterium));

        connect(
            _labelWidget, &FilterLabelWidget::editingRequested,
            this, [this] { switchLabelToEditor(); }
        );

        layout()->replaceWidget(widgetToReplace, _labelWidget);
        widgetToReplace->setVisible(false);

        _editButton->setEnabled(true);
        _editButton->setVisible(true);
        _okButton->setVisible(false);
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

    void FilterLineWidget::switchPickerToEditor()
    {
        Q_ASSERT_X(_labelWidget == nullptr,
                   "FilterLineWidget::switchPickerToEditor",
                   "label widget present!");

        auto criterium = _filterPicker->createCriterium();

        switchToEditor(_filterPicker, *criterium);
    }

    void FilterLineWidget::switchToEditor(QWidget* widgetToReplace,
                                          const TrackCriterium& criterium)
    {
        Q_ASSERT_X(_editorWidget == nullptr,
                   "FilterLineWidget::switchToEditor",
                   "editor already present!");

        _editorWidget = FilterEditorFactory::createEditor(nullptr, criterium);

        Q_ASSERT_X(_editorWidget != nullptr,
                   "FilterLineWidget::switchToEditor",
                   "failed to obtain editor for criterium");

        connect(
            _editorWidget, &FilterEditorWidget::criteriumChanged,
            this, &FilterLineWidget::criteriumChanged
        );

        layout()->replaceWidget(widgetToReplace, _editorWidget);
        widgetToReplace->setVisible(false);

        _editButton->setEnabled(false);
        _editButton->setVisible(false);
        _okButton->setVisible(true);
    }

    // =============================================================== //

    FiltersListWidget::FiltersListWidget()
    {
        _verticalLayout = new QVBoxLayout(this);
        _verticalLayout->setContentsMargins(0, 0, 0, 0);

        addFilterLine();

        auto* buttonsLayout = new QHBoxLayout();
        _verticalLayout->addLayout(buttonsLayout);

        _addButton = new QPushButton(tr("Add"));
        _addButton->setToolTip(tr("Add filter"));

        buttonsLayout->addWidget(_addButton);
        buttonsLayout->addStretch();

        connect(
            _addButton, &QPushButton::clicked,
            this,
            [this]()
            {
                addFilterLine();

                // emit is not necessary because the new filter is "none"
                //Q_EMIT criteriaChanged();
            }
        );
    }

    std::unique_ptr<TrackCriterium> FiltersListWidget::createCriterium() const
    {
        auto compositeCriterium = std::make_unique<CompositeTrackCriterium>();

        for (auto const* filterLine : _filters)
        {
            compositeCriterium->add(filterLine->createCriterium());
        }

        return compositeCriterium;
    }

    void FiltersListWidget::addFilterLine()
    {
        auto* filter = new FilterLineWidget();

        auto index = _filters.size();
        _verticalLayout->insertWidget(index, filter);

        _filters.append(filter);

        connect(
            filter, &FilterLineWidget::criteriumChanged,
            this,
            [this]()
            {
                Q_EMIT criteriumChanged();
            }
        );

        connect(
            filter, &FilterLineWidget::deleteClicked,
            this,
            [this, filter]()
            {
                auto index = _filters.indexOf(filter);
                Q_ASSERT_X(
                    index >= 0,
                    "FiltersListWidget::addFilterLine",
                    "filter to be deleted not found"
                    );

                _filters.removeAt(index);
                filter->deleteLater();

                Q_EMIT criteriumChanged();
            }
        );
    }
}

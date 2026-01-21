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

#include "common/unicodechars.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace PMP
{
    FilterPickerWidget::FilterPickerWidget(PredefinedTrackCriterium criteriumForEmpty,
                                           QString captionForEmpty)
     : _predefinedCriterium(criteriumForEmpty),
        _criterium(convertToTrackCriterium(criteriumForEmpty))
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
                _criterium = convertToTrackCriterium(predefinedCriterium);
                Q_EMIT criteriumChanged();
            }
            );
    }

    void FilterPickerWidget::clearCriterium()
    {
        _comboBox->setCurrentIndex(0);
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

    FilterLineWidget::FilterLineWidget()
    {
        _filterPicker = new FilterPickerWidget(PredefinedTrackCriterium::AllTracks,
                                               tr("(empty)"));
        _criterium = _filterPicker->criterium().clone();
        _deleteButton = new QPushButton();
        _resetButton = new QPushButton();

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(_filterPicker, 1);
        layout->addWidget(_deleteButton, 0);
        layout->addWidget(_resetButton, 0);

        _deleteButton->setIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton));
        _deleteButton->setToolTip(tr("Remove filter"));

        _resetButton->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));
        _resetButton->setToolTip(tr("Clear filter"));

        connect(
            _filterPicker, &FilterPickerWidget::criteriumChanged,
            [this]()
            {
                _criterium = _filterPicker->criterium().clone();
                Q_EMIT criteriumChanged();
            }
        );

        connect(
            _deleteButton, &QPushButton::clicked,
            this, [this]() { Q_EMIT deleteClicked(); }
        );

        connect(
            _resetButton, &QPushButton::clicked,
            this, [this]() { _filterPicker->clearCriterium(); }
        );
    }

    // =============================================================== //

    FiltersListWidget::FiltersListWidget()
     : _criterium(ConstantTrackCriterium::allTracksMatch())
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

    void FiltersListWidget::addFilterLine()
    {
        auto* filter = new FilterLineWidget();

        auto index = _filters.size();
        _verticalLayout->insertWidget(index, filter);

        _filters.append(filter);
        rebuildCriterium();

        connect(
            filter, &FilterLineWidget::criteriumChanged,
            this,
            [this]()
            {
                rebuildCriterium();
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

                rebuildCriterium();
                Q_EMIT criteriumChanged();
            }
        );
    }

    void FiltersListWidget::rebuildCriterium()
    {
        auto compositeCriterium = std::make_unique<CompositeTrackCriterium>();

        for (auto const* filterLine : _filters)
        {
            compositeCriterium->add(filterLine->criterium().clone());
        }

        _criterium = std::move(compositeCriterium);
    }
}

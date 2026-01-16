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

#include "collectionwidget.h"
#include "ui_collectionwidget.h"

#include "common/unicodechars.h"

#include "client/collectionwatcher.h"
#include "client/queuecontroller.h"
#include "client/serverinterface.h"

#include "collectiontablemodel.h"
#include "colors.h"
#include "colorswitcher.h"
#include "trackinfodialog.h"
#include "waitingspinnerwidget.h"

#include <QMenu>
#include <QtDebug>
#include <QSettings>

using namespace PMP::Client;

namespace PMP
{
    CollectionWidget::CollectionWidget(QWidget* parent, ServerInterface* serverInterface,
                                       QueueHashesMonitor* queueHashesMonitor,
                                       UserForStatisticsDisplay* userForStatisticsDisplay,
                                       SearchData* searchData)
     : QWidget(parent),
       _ui(new Ui::CollectionWidget),
       _colorSwitcher(new ColorSwitcher()),
       _serverInterface(serverInterface),
       _userStatisticsDisplay(userForStatisticsDisplay),
       _collectionSourceModel(new SortedCollectionTableModel(this,
                                                             serverInterface,
                                                             queueHashesMonitor,
                                                             userForStatisticsDisplay)),
       _collectionDisplayModel(new FilteredCollectionTableModel(this,
                                                                _collectionSourceModel,
                                                                serverInterface,
                                                                searchData,
                                                                queueHashesMonitor,
                                                               userForStatisticsDisplay)),
       _collectionContextMenu(nullptr)
    {
        _ui->setupUi(this);

        initTrackFilterWidgets();
        initTrackHighlightingWidgets();

        _ui->collectionTableView->setModel(_collectionDisplayModel);
        _ui->collectionTableView->setDragEnabled(true);
        _ui->collectionTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _ui->collectionTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

        connect(
            _ui->searchLineEdit, &QLineEdit::textChanged,
            _collectionDisplayModel, &FilteredCollectionTableModel::setSearchText
        );
        connect(
            _ui->collectionTableView, &QTableView::customContextMenuRequested,
            this, &CollectionWidget::collectionContextMenuRequested
        );

        connect(
            _collectionDisplayModel, &FilteredCollectionTableModel::rowsInserted,
            this, &CollectionWidget::rowCountChanged
        );
        connect(
            _collectionDisplayModel, &FilteredCollectionTableModel::rowsRemoved,
            this, &CollectionWidget::rowCountChanged
        );
        rowCountChanged();

        auto* collectionWatcher = &_serverInterface->collectionWatcher();
        connect(
            collectionWatcher, &CollectionWatcher::downloadingInProgressChanged,
            this, [this]() { updateSpinnerVisibility(); }
        );
        updateSpinnerVisibility();

        {
            QSettings settings(QCoreApplication::organizationName(),
                               QCoreApplication::applicationName());

            settings.beginGroup("collectionview");

            _ui->collectionTableView->horizontalHeader()->restoreState(
                settings.value("columnsstate").toByteArray()
            );

            int sortColumn = settings.value("sortcolumn").toInt();
            if (sortColumn < 0 || sortColumn > 3) { sortColumn = 0; }

            bool sortDescending = settings.value("sortdescending").toBool();
            auto sortOrder = sortDescending ? Qt::DescendingOrder : Qt::AscendingOrder;

            _ui->collectionTableView->sortByColumn(sortColumn, sortOrder);
            _ui->collectionTableView->setSortingEnabled(true);
        }

        _ui->collectionTableView->horizontalHeader()->setSortIndicatorShown(true);
    }

    CollectionWidget::~CollectionWidget()
    {
        QSettings settings(QCoreApplication::organizationName(),
                           QCoreApplication::applicationName());

        settings.beginGroup("collectionview");
        settings.setValue(
            "columnsstate", _ui->collectionTableView->horizontalHeader()->saveState()
        );
        settings.setValue("sortcolumn", _collectionSourceModel->sortColumn());
        settings.setValue(
            "sortdescending", _collectionSourceModel->sortOrder() == Qt::DescendingOrder
        );

        delete _ui;
    }

    void CollectionWidget::changeEvent(QEvent* event)
    {
        if (event->type() == QEvent::PaletteChange)
        {
            qDebug() << "CollectionWidget: detected palette change event";
            updateColors(/* force: */ false);
        }

        QWidget::changeEvent(event);
    }

    void CollectionWidget::onFiltersChanged()
    {
        _collectionDisplayModel->setTrackFilters(_filtersListWidget->criterium());
    }

    void CollectionWidget::onHighlightCriteriumChanged()
    {
        bool nothingToHighlight =
            _highlightingCriteriumPicker->criterium().equals(
                *ConstantTrackCriterium::noTracksMatch()
            );

        _colorSwitcher->setVisible(!nothingToHighlight);

        _collectionSourceModel->setHighlightCriterium(
            _highlightingCriteriumPicker->criterium()
        );
    }

    void CollectionWidget::highlightColorIndexChanged()
    {
        _collectionSourceModel->setHighlightColorIndex(_colorSwitcher->colorIndex());
    }

    void CollectionWidget::collectionContextMenuRequested(const QPoint& position)
    {
        qDebug() << "CollectionWidget: contextmenu requested";

        auto index = _ui->collectionTableView->indexAt(position);
        if (!index.isValid()) return;

        auto trackPointer = _collectionDisplayModel->trackAt(index);
        if (!trackPointer) return;

        auto track = *trackPointer;
        auto hashId = track.hashId();

        if (_collectionContextMenu)
            delete _collectionContextMenu;
        _collectionContextMenu = new QMenu(this);

        auto enqueueFrontAction =
            _collectionContextMenu->addAction(tr("Add to front of queue"));
        connect(
            enqueueFrontAction, &QAction::triggered,
            this,
            [this, hashId]()
            {
                qDebug() << "collection context menu: enqueue (front) triggered";
                _serverInterface->queueController().insertQueueEntryAtFront(hashId);
            }
        );

        auto enqueueEndAction =
                _collectionContextMenu->addAction(tr("Add to end of queue"));
        connect(
            enqueueEndAction, &QAction::triggered,
            this,
            [this, hashId]()
            {
                qDebug() << "collection context menu: enqueue (end) triggered";
                _serverInterface->queueController().insertQueueEntryAtEnd(hashId);
            }
        );

        _collectionContextMenu->addSeparator();

        auto trackInfoAction = _collectionContextMenu->addAction(tr("Track info"));
        connect(
            trackInfoAction, &QAction::triggered,
            this,
            [this, track]()
            {
                qDebug() << "collection context menu: track info triggered";
                auto dialog = new TrackInfoDialog(this, _serverInterface,
                                                  _userStatisticsDisplay, track);
                connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
                dialog->show();
            }
        );

        auto popupPosition = _ui->collectionTableView->viewport()->mapToGlobal(position);
        _collectionContextMenu->popup(popupPosition);
    }

    void CollectionWidget::rowCountChanged()
    {
        auto rowCount = _collectionDisplayModel->rowCount();

        _ui->trackCountLabel->setText(tr("%n track(s) shown", "", rowCount));
    }

    void CollectionWidget::updateSpinnerVisibility()
    {
        bool downloading = _serverInterface->collectionWatcher().downloadingInProgress();

        if (downloading)
        {
            if (!_spinner)
                _spinner = new WaitingSpinnerWidget(this, true, false);

            _spinner->start();
        }
        else if (_spinner)
        {
            _spinner->stop();
            _spinner->deleteLater();
            _spinner = nullptr;
        }
    }

    void CollectionWidget::initTrackFilterWidgets()
    {
        _filtersListWidget = new FiltersListWidget();

        auto layoutItem =
            this->layout()->replaceWidget(_ui->filterPlaceholder, _filtersListWidget);

        delete layoutItem;
        /* we cannot delete the placeholder because of retranslateUi() so we hide it */
        _ui->filterPlaceholder->setVisible(false);

        connect(
            _filtersListWidget, &FiltersListWidget::criteriumChanged,
            this, [this]() { onFiltersChanged(); }
        );

        onFiltersChanged();
    }

    void CollectionWidget::initTrackHighlightingWidgets()
    {
        _highlightingCriteriumPicker =
            new FilterPickerWidget(PredefinedTrackCriterium::NoTracks, tr("(none)"));

        {
            auto layoutItem =
                this->layout()->replaceWidget(_ui->highlightTracksComboBox,
                                              _highlightingCriteriumPicker);

            delete layoutItem;
            /* we cannot delete the placeholder because of retranslateUi() so we hide it*/
            _ui->highlightTracksComboBox->setVisible(false);
        }

        connect(
            _highlightingCriteriumPicker, &FilterPickerWidget::criteriumChanged,
            this, [this]() { onHighlightCriteriumChanged(); }
        );

        connect(
            _colorSwitcher, &ColorSwitcher::colorIndexChanged,
            this, &CollectionWidget::highlightColorIndexChanged
        );

        {
            auto layoutItem =
                this->layout()->replaceWidget(_ui->highlightColorButton, _colorSwitcher);

            delete layoutItem;
            /* we cannot delete the placeholder because of retranslateUi() so we hide it*/
            _ui->highlightColorButton->setVisible(false);
        }

        auto* resetButton = _ui->highlightTracksResetButton;
        resetButton->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));
        resetButton->setToolTip(tr("Reset highlighting"));

        connect(
            resetButton, &QPushButton::clicked,
            this, [this]() { _highlightingCriteriumPicker->clearCriterium(); }
        );

        updateColors(/* force: */ true);
        onHighlightCriteriumChanged();
    }

    void CollectionWidget::updateColors(bool force)
    {
        auto& colors = Colors::instance();

        bool darkMode = colors.isDarkMode();
        if (!force && darkMode == _usingColorsForDarkMode)
            return;

        _colorSwitcher->setColors(colors.itemBackgroundHighlightColors);
        _usingColorsForDarkMode = darkMode;
    }

    // =============================================================== //

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

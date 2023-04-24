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
                                       QueueHashesMonitor* queueHashesMonitor)
     : QWidget(parent),
       _ui(new Ui::CollectionWidget),
       _colorSwitcher(nullptr),
       _serverInterface(serverInterface),
       _collectionViewContext(new CollectionViewContext(this, serverInterface)),
       _collectionSourceModel(new SortedCollectionTableModel(this,
                                                             serverInterface,
                                                             queueHashesMonitor,
                                                             _collectionViewContext)),
       _collectionDisplayModel(new FilteredCollectionTableModel(this,
                                                                _collectionSourceModel,
                                                                serverInterface,
                                                                queueHashesMonitor,
                                                                _collectionViewContext)),
       _collectionContextMenu(nullptr)
    {
        _ui->setupUi(this);

        initTrackFilterComboBoxes();
        initTrackHighlightingComboBox();
        initTrackHighlightingColorSwitcher();

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

    void CollectionWidget::filterTracksIndexChanged()
    {
        auto filter1 = getTrackCriteriumFromComboBox(_ui->filterTracksComboBox);
        auto filter2 = getTrackCriteriumFromComboBox(_ui->filterTracks2ComboBox);

        bool shouldDisplayFilter2 =
                filter1 != TrackCriterium::AllTracks
                    || filter2 != TrackCriterium::AllTracks;

        _ui->filterTracks2Label->setVisible(shouldDisplayFilter2);
        _ui->filterTracks2ComboBox->setVisible(shouldDisplayFilter2);

        _collectionDisplayModel->setTrackFilters(filter1, filter2);
    }

    void CollectionWidget::highlightTracksIndexChanged(int index)
    {
        Q_UNUSED(index)

        auto highlightMode = getCurrentHighlightMode();

        _colorSwitcher->setVisible(highlightMode != TrackCriterium::NoTracks);

        _collectionSourceModel->setHighlightCriterium(highlightMode);
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
            [this, track]() {
                qDebug() << "collection context menu: track info triggered";
                auto dialog = new TrackInfoDialog(this, _serverInterface, track);
                connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
                dialog->open();
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

    void CollectionWidget::initTrackFilterComboBoxes()
    {
        auto comboBoxInit =
            [this](QComboBox* comboBox)
            {
                fillTrackCriteriaComboBox(comboBox, TrackCriterium::AllTracks);

                connect(
                    comboBox, qOverload<int>(&QComboBox::currentIndexChanged),
                    this, &CollectionWidget::filterTracksIndexChanged
                );
            };

        comboBoxInit(_ui->filterTracksComboBox);
        comboBoxInit(_ui->filterTracks2ComboBox);

        filterTracksIndexChanged();
    }

    void CollectionWidget::initTrackHighlightingComboBox()
    {
        auto combo = _ui->highlightTracksComboBox;

        fillTrackCriteriaComboBox(combo, TrackCriterium::NoTracks);

        connect(
            combo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &CollectionWidget::highlightTracksIndexChanged
        );
    }

    void CollectionWidget::fillTrackCriteriaComboBox(QComboBox* comboBox,
                                                     TrackCriterium criteriumForNone)
    {
        auto addItem =
            [comboBox](QString text, TrackCriterium mode)
            {
                text.replace(">=", UnicodeChars::greaterThanOrEqual)
                    .replace("<=", UnicodeChars::lessThanOrEqual);

                comboBox->addItem(text, QVariant::fromValue(mode));
            };

        addItem(tr("none"), criteriumForNone);

        addItem(tr("never heard"), TrackCriterium::NeverHeard);
        addItem(tr("not heard in the last 1000 days"),
                TrackCriterium::LastHeardNotInLast1000Days);
        addItem(tr("not heard in the last 365 days"),
                TrackCriterium::LastHeardNotInLast365Days);
        addItem(tr("not heard in the last 180 days"),
                TrackCriterium::LastHeardNotInLast180Days);
        addItem(tr("not heard in the last 90 days"),
                TrackCriterium::LastHeardNotInLast90Days);
        addItem(tr("not heard in the last 30 days"),
                TrackCriterium::LastHeardNotInLast30Days);
        addItem(tr("not heard in the last 10 days"),
                TrackCriterium::LastHeardNotInLast10Days);

        addItem(tr("without score"), TrackCriterium::WithoutScore);
        addItem(tr("score <= 30"), TrackCriterium::ScoreMaximum30);
        addItem(tr("score >= 85"), TrackCriterium::ScoreAtLeast85);
        addItem(tr("score >= 90"), TrackCriterium::ScoreAtLeast90);
        addItem(tr("score >= 95"), TrackCriterium::ScoreAtLeast95);

        addItem(tr("length <= 1 min."), TrackCriterium::LengthMaximumOneMinute);
        addItem(tr("length >= 5 min."), TrackCriterium::LengthAtLeastFiveMinutes);

        addItem(tr("not in the queue"), TrackCriterium::NotInTheQueue);
        addItem(tr("in the queue"), TrackCriterium::InTheQueue);

        comboBox->setCurrentIndex(0);
    }

    void CollectionWidget::initTrackHighlightingColorSwitcher()
    {
        auto& colors = Colors::instance();

        _colorSwitcher = new ColorSwitcher();
        _colorSwitcher->setColors(colors.itemBackgroundHighlightColors);
        _colorSwitcher->setVisible(getCurrentHighlightMode() != TrackCriterium::NoTracks);

        connect(
            _colorSwitcher, &ColorSwitcher::colorIndexChanged,
            this, &CollectionWidget::highlightColorIndexChanged
        );

        auto layoutItem =
            this->layout()->replaceWidget(_ui->highlightColorButton, _colorSwitcher);

        delete layoutItem;
        delete _ui->highlightColorButton;
        _ui->highlightColorButton = nullptr;
    }

    TrackCriterium CollectionWidget::getCurrentHighlightMode() const
    {
        return getTrackCriteriumFromComboBox(_ui->highlightTracksComboBox);
    }

    TrackCriterium CollectionWidget::getTrackCriteriumFromComboBox(
                                                                QComboBox* comboBox) const
    {
        return comboBox->currentData().value<TrackCriterium>();
    }
}

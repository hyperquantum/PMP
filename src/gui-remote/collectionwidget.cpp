/*
    Copyright (C) 2016-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "client/clientserverinterface.h"
#include "client/collectionwatcher.h"
#include "client/queuecontroller.h"

#include "collectiontablemodel.h"
#include "colors.h"
#include "colorswitcher.h"
#include "trackinfodialog.h"
#include "waitingspinnerwidget.h"

#include <QMenu>
#include <QtDebug>
#include <QSettings>

namespace PMP
{
    CollectionWidget::CollectionWidget(QWidget* parent,
                                       ClientServerInterface* clientServerInterface)
     : QWidget(parent),
       _ui(new Ui::CollectionWidget),
       _colorSwitcher(nullptr),
       _clientServerInterface(clientServerInterface),
       _collectionViewContext(new CollectionViewContext(this, clientServerInterface)),
       _collectionSourceModel(new SortedCollectionTableModel(this,
                                                             clientServerInterface,
                                                             _collectionViewContext)),
       _collectionDisplayModel(new FilteredCollectionTableModel(this,
                                                                _collectionSourceModel,
                                                                clientServerInterface,
                                                                _collectionViewContext)),
       _collectionContextMenu(nullptr)
    {
        _ui->setupUi(this);

        initTrackFilterComboBox();
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

        auto* collectionWatcher = &_clientServerInterface->collectionWatcher();
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

    void CollectionWidget::filterTracksIndexChanged(int index)
    {
        Q_UNUSED(index)

        auto filter = getCurrentTrackFilter();

        _collectionDisplayModel->setTrackFilter(filter);
    }

    void CollectionWidget::highlightTracksIndexChanged(int index)
    {
        Q_UNUSED(index)

        auto mode = getCurrentHighlightMode();

        _colorSwitcher->setVisible(mode != TrackCriterium::None);

        _collectionSourceModel->setHighlightCriterium(mode);
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
        FileHash hash = track.hash();

        if (_collectionContextMenu)
            delete _collectionContextMenu;
        _collectionContextMenu = new QMenu(this);

        auto enqueueFrontAction =
            _collectionContextMenu->addAction(tr("Add to front of queue"));
        connect(
            enqueueFrontAction, &QAction::triggered,
            this,
            [this, hash]() {
                qDebug() << "collection context menu: enqueue (front) triggered";
                _clientServerInterface->queueController().insertQueueEntryAtFront(hash);
            }
        );

        auto enqueueEndAction =
                _collectionContextMenu->addAction(tr("Add to end of queue"));
        connect(
            enqueueEndAction, &QAction::triggered,
            this,
            [this, hash]() {
                qDebug() << "collection context menu: enqueue (end) triggered";
                _clientServerInterface->queueController().insertQueueEntryAtEnd(hash);
            }
        );

        _collectionContextMenu->addSeparator();

        auto trackInfoAction = _collectionContextMenu->addAction(tr("Track info"));
        connect(
            trackInfoAction, &QAction::triggered,
            this,
            [this, track]() {
                qDebug() << "collection context menu: track info triggered";
                auto dialog = new TrackInfoDialog(this, _clientServerInterface, track);
                connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
                dialog->open();
            }
        );

        auto popupPosition = _ui->collectionTableView->viewport()->mapToGlobal(position);
        _collectionContextMenu->popup(popupPosition);
    }

    void CollectionWidget::updateSpinnerVisibility()
    {
        bool downloading =
                _clientServerInterface->collectionWatcher().downloadingInProgress();

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

    void CollectionWidget::initTrackFilterComboBox()
    {
        auto combo = _ui->filterTracksComboBox;

        fillTrackCriteriaComboBox(combo);

        connect(
            combo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &CollectionWidget::filterTracksIndexChanged
        );
    }

    void CollectionWidget::initTrackHighlightingComboBox()
    {
        auto combo = _ui->highlightTracksComboBox;

        fillTrackCriteriaComboBox(combo);

        connect(
            combo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &CollectionWidget::highlightTracksIndexChanged
        );
    }

    void CollectionWidget::fillTrackCriteriaComboBox(QComboBox* comboBox)
    {
        auto addItem =
            [comboBox](QString text, TrackCriterium mode)
            {
                text.replace(">=", UnicodeChars::greaterThanOrEqual)
                    .replace("<=", UnicodeChars::lessThanOrEqual);

                comboBox->addItem(text, QVariant::fromValue(mode));
            };

        addItem(tr("none"), TrackCriterium::None);

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

        comboBox->setCurrentIndex(0);
    }

    void CollectionWidget::initTrackHighlightingColorSwitcher()
    {
        auto& colors = Colors::instance();

        _colorSwitcher = new ColorSwitcher();
        _colorSwitcher->setColors(colors.itemBackgroundHighlightColors);
        _colorSwitcher->setVisible(getCurrentHighlightMode() != TrackCriterium::None);

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

    TrackCriterium CollectionWidget::getCurrentTrackFilter() const
    {
        return getTrackCriteriumFromComboBox(_ui->filterTracksComboBox);
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

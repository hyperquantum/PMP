/*
    Copyright (C) 2016-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/serverconnection.h"
#include "common/util.h"

#include "collectiontablemodel.h"
#include "trackinfodialog.h"

#include <QMenu>
#include <QtDebug>
#include <QSettings>

namespace PMP {

    CollectionWidget::CollectionWidget(QWidget* parent, ServerConnection* connection,
                                       ClientServerInterface* clientServerInterface)
     : QWidget(parent),
       _ui(new Ui::CollectionWidget),
       _connection(connection),
       _clientServerInterface(clientServerInterface),
       _collectionSourceModel(new SortedCollectionTableModel(this,clientServerInterface)),
       _collectionDisplayModel(
           new FilteredCollectionTableModel(_collectionSourceModel, this)),
       _collectionContextMenu(nullptr)
    {
        _ui->setupUi(this);

        _ui->highlightTracksComboBox->addItem(tr("none"),
                                           QVariant::fromValue(TrackHighlightMode::None));
        _ui->highlightTracksComboBox->addItem(tr("never heard"),
                                     QVariant::fromValue(TrackHighlightMode::NeverHeard));
        _ui->highlightTracksComboBox->addItem(
                                tr("without score"),
                                QVariant::fromValue(TrackHighlightMode::WithoutScore));
        _ui->highlightTracksComboBox->addItem(
                                tr("score >= 85").replace(">=", Util::GreaterThanOrEqual),
                                QVariant::fromValue(TrackHighlightMode::ScoreAtLeast85));
        _ui->highlightTracksComboBox->addItem(
                                tr("score >= 90").replace(">=", Util::GreaterThanOrEqual),
                                QVariant::fromValue(TrackHighlightMode::ScoreAtLeast90));
        _ui->highlightTracksComboBox->addItem(
                                tr("score >= 95").replace(">=", Util::GreaterThanOrEqual),
                                QVariant::fromValue(TrackHighlightMode::ScoreAtLeast95));
        _ui->highlightTracksComboBox->addItem(
                        tr("length <= 1 min.").replace("<=", Util::LessThanOrEqual),
                        QVariant::fromValue(TrackHighlightMode::LengthMaximumOneMinute));
        _ui->highlightTracksComboBox->addItem(
                       tr("length >= 5 min.").replace(">=", Util::GreaterThanOrEqual),
                       QVariant::fromValue(TrackHighlightMode::LengthAtLeastFiveMinutes));
        _ui->highlightTracksComboBox->setCurrentIndex(0);

        _ui->collectionTableView->setModel(_collectionDisplayModel);
        _ui->collectionTableView->setDragEnabled(true);
        _ui->collectionTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _ui->collectionTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

        connect(
            _ui->searchLineEdit, &QLineEdit::textChanged,
            _collectionDisplayModel, &FilteredCollectionTableModel::setSearchText
        );
        connect(
            _ui->highlightTracksComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &CollectionWidget::highlightTracksIndexChanged
        );
        connect(
            _ui->collectionTableView, &QTableView::customContextMenuRequested,
            this, &CollectionWidget::collectionContextMenuRequested
        );

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

    CollectionWidget::~CollectionWidget() {
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

    void CollectionWidget::highlightTracksIndexChanged(int index) {
        Q_UNUSED(index)

        auto mode =
                _ui->highlightTracksComboBox->currentData().value<TrackHighlightMode>();

        _collectionSourceModel->setHighlightMode(mode);
    }

    void CollectionWidget::collectionContextMenuRequested(const QPoint& position) {
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
            [this, hash]() {
                qDebug() << "collection context menu: enqueue (front) triggered";
                _connection->insertQueueEntryAtFront(hash);
            }
        );

        auto enqueueEndAction =
                _collectionContextMenu->addAction(tr("Add to end of queue"));
        connect(
            enqueueEndAction, &QAction::triggered,
            [this, hash]() {
                qDebug() << "collection context menu: enqueue (end) triggered";
                _connection->insertQueueEntryAtEnd(hash);
            }
        );

        _collectionContextMenu->addSeparator();

        auto trackInfoAction = _collectionContextMenu->addAction(tr("Track info"));
        connect(
            trackInfoAction, &QAction::triggered,
            this,
            [this, track]() {
                qDebug() << "collection context menu: track info triggered";
                auto dialog = new TrackInfoDialog(this, track, _clientServerInterface);
                connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
                dialog->open();
            }
        );

        auto popupPosition = _ui->collectionTableView->viewport()->mapToGlobal(position);
        _collectionContextMenu->popup(popupPosition);
    }

}

/*
    Copyright (C) 2016, Kevin Andre <hyperquantum@gmail.com>

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

#include "collectiontablemodel.h"

#include <QMenu>
#include <QtDebug>
#include <QSettings>

namespace PMP {

    CollectionWidget::CollectionWidget(QWidget* parent)
     : QWidget(parent), _ui(new Ui::CollectionWidget), _connection(nullptr),
       _collectionSourceModel(new CollectionTableModel(this)),
       _collectionDisplayModel(
           new SortedCollectionTableModel(_collectionSourceModel, this))
    {
        _ui->setupUi(this);

        _ui->collectionTableView->setModel(_collectionDisplayModel);

        connect(
            _ui->searchLineEdit, &QLineEdit::textChanged,
            _collectionDisplayModel, &SortedCollectionTableModel::setFilterFixedString
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
            if (sortColumn < 0 || sortColumn > 1) { sortColumn = 0; }

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
        settings.setValue("sortcolumn", _collectionDisplayModel->sortColumn());
        settings.setValue(
            "sortdescending", _collectionDisplayModel->sortOrder() == Qt::DescendingOrder
        );

        delete _ui;
    }

    void CollectionWidget::setConnection(ServerConnection* connection) {
        _connection = connection;
        _collectionSourceModel->setConnection(connection);
    }

    void CollectionWidget::collectionContextMenuRequested(const QPoint& position) {
        qDebug() << "CollectionWidget: contextmenu requested";

        auto index = _ui->collectionTableView->indexAt(position);
        if (!index.isValid()) return;

        auto track = _collectionDisplayModel->trackAt(index);
        if (!track) return;

        QMenu* menu = new QMenu(this);

        QAction* enqueueFrontAction = menu->addAction("Add to front of queue");
        connect(
            enqueueFrontAction, &QAction::triggered,
            [this, track]() {
                qDebug() << "collection context menu: enqueue (front) triggered";
                _connection->insertQueueEntryAtFront(track->hash());
            }
        );

        QAction* enqueueEndAction = menu->addAction("Add to end of queue");
        connect(
            enqueueEndAction, &QAction::triggered,
            [this, track]() {
                qDebug() << "collection context menu: enqueue (end) triggered";
                _connection->insertQueueEntryAtEnd(track->hash());
            }
        );

        menu->popup(_ui->collectionTableView->viewport()->mapToGlobal(position));
    }

}

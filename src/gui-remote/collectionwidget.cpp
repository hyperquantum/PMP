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

namespace PMP {

    CollectionWidget::CollectionWidget(QWidget* parent)
     : QWidget(parent), _ui(new Ui::CollectionWidget), _connection(nullptr),
       _collectionModel(new CollectionTableModel(this))
    {
        _ui->setupUi(this);
        _ui->collectionTableView->setModel(_collectionModel);

        connect(
            _ui->collectionTableView, &QTableView::customContextMenuRequested,
            this, &CollectionWidget::collectionContextMenuRequested
        );
    }

    CollectionWidget::~CollectionWidget() {
        delete _ui;
    }

    void CollectionWidget::setConnection(ServerConnection* connection) {
        _connection = connection;
        _collectionModel->setConnection(connection);
    }

    void CollectionWidget::collectionContextMenuRequested(const QPoint& position) {
        qDebug() << "CollectionWidget: contextmenu requested";

        auto index = _ui->collectionTableView->indexAt(position);

        if (index.isValid()) {
            //int row = index.row();
            auto track = _collectionModel->trackAt(index);
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

}

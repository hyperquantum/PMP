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

namespace PMP {

    CollectionWidget::CollectionWidget(QWidget* parent)
     : QWidget(parent), _ui(new Ui::CollectionWidget), _connection(nullptr),
       _collectionModel(new CollectionTableModel(this))
    {
        _ui->setupUi(this);
        _ui->collectionTableView->setModel(_collectionModel);
    }

    CollectionWidget::~CollectionWidget() {
        delete _ui;
    }

    void CollectionWidget::setConnection(ServerConnection* connection) {
        _connection = connection;
        _collectionModel->setConnection(connection);
    }

}

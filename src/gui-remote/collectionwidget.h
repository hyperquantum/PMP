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

#ifndef PMP_COLLECTIONWIDGET_H
#define PMP_COLLECTIONWIDGET_H

#include <QWidget>

namespace Ui {
    class CollectionWidget;
}

namespace PMP {

    class CollectionTableModel;
    class ServerConnection;

    class CollectionWidget : public QWidget {
        Q_OBJECT

    public:
        explicit CollectionWidget(QWidget* parent = 0);
        ~CollectionWidget();

        void setConnection(ServerConnection* connection);

    private slots:
        void collectionContextMenuRequested(const QPoint& position);

    private:
        Ui::CollectionWidget* _ui;
        ServerConnection* _connection;
        CollectionTableModel* _collectionModel;
    };
}
#endif

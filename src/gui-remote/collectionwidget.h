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

#ifndef PMP_COLLECTIONWIDGET_H
#define PMP_COLLECTIONWIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Ui {
    class CollectionWidget;
}

namespace PMP {

    class ClientServerInterface;
    class FilteredCollectionTableModel;
    class ServerConnection;
    class SortedCollectionTableModel;

    class CollectionWidget : public QWidget {
        Q_OBJECT

    public:
        CollectionWidget(QWidget* parent, ServerConnection* connection,
                         ClientServerInterface* clientServerInterface);
        ~CollectionWidget();

    private slots:
        void highlightTracksIndexChanged(int index);
        void collectionContextMenuRequested(const QPoint& position);

    private:
        Ui::CollectionWidget* _ui;
        ServerConnection* _connection;
        SortedCollectionTableModel* _collectionSourceModel;
        FilteredCollectionTableModel* _collectionDisplayModel;
        QMenu* _collectionContextMenu;
    };
}
#endif

/*
    Copyright (C) 2016-2021, Kevin Andre <hyperquantum@gmail.com>

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

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Ui
{
    class CollectionWidget;
}

namespace PMP
{
    class ClientServerInterface;
    class CollectionViewContext;
    class ColorSwitcher;
    class FilteredCollectionTableModel;
    class SortedCollectionTableModel;
    enum class TrackCriterium;

    class CollectionWidget : public QWidget {
        Q_OBJECT

    public:
        CollectionWidget(QWidget* parent, ClientServerInterface* clientServerInterface);
        ~CollectionWidget();

    private Q_SLOTS:
        void filterTracksIndexChanged(int index);
        void highlightTracksIndexChanged(int index);
        void highlightColorIndexChanged();
        void collectionContextMenuRequested(const QPoint& position);

    private:
        void initTrackFilterComboBox();
        void initTrackHighlightingComboBox();
        void fillTrackCriteriaComboBox(QComboBox* comboBox);
        void initTrackHighlightingColorSwitcher();

        TrackCriterium getCurrentTrackFilter() const;
        TrackCriterium getCurrentHighlightMode() const;
        TrackCriterium getTrackCriteriumFromComboBox(QComboBox* comboBox) const;

        Ui::CollectionWidget* _ui;
        ColorSwitcher* _colorSwitcher;
        ClientServerInterface* _clientServerInterface;
        CollectionViewContext* _collectionViewContext;
        SortedCollectionTableModel* _collectionSourceModel;
        FilteredCollectionTableModel* _collectionDisplayModel;
        QMenu* _collectionContextMenu;
    };
}
#endif

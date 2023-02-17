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

#ifndef PMP_COLLECTIONWIDGET_H
#define PMP_COLLECTIONWIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Ui
{
    class CollectionWidget;
}

namespace PMP::Client
{
    class QueueHashesMonitor;
    class ServerInterface;
}

namespace PMP
{
    class CollectionViewContext;
    class ColorSwitcher;
    class FilteredCollectionTableModel;
    class SortedCollectionTableModel;
    enum class TrackCriterium;
    class WaitingSpinnerWidget;

    class CollectionWidget : public QWidget
    {
        Q_OBJECT

    public:
        CollectionWidget(QWidget* parent, Client::ServerInterface* serverInterface,
                         Client::QueueHashesMonitor* queueHashesMonitor);
        ~CollectionWidget();

    private Q_SLOTS:
        void filterTracksIndexChanged(int index);
        void highlightTracksIndexChanged(int index);
        void highlightColorIndexChanged();
        void collectionContextMenuRequested(const QPoint& position);

    private:
        void updateSpinnerVisibility();
        void initTrackFilterComboBoxes();
        void initTrackHighlightingComboBox();
        void fillTrackCriteriaComboBox(QComboBox* comboBox,
                                       TrackCriterium criteriumForNone);
        void initTrackHighlightingColorSwitcher();

        TrackCriterium getCurrentHighlightMode() const;
        TrackCriterium getTrackCriteriumFromComboBox(QComboBox* comboBox) const;

        Ui::CollectionWidget* _ui;
        WaitingSpinnerWidget* _spinner { nullptr };
        ColorSwitcher* _colorSwitcher;
        Client::ServerInterface* _serverInterface;
        CollectionViewContext* _collectionViewContext;
        SortedCollectionTableModel* _collectionSourceModel;
        FilteredCollectionTableModel* _collectionDisplayModel;
        QMenu* _collectionContextMenu;
    };
}
#endif

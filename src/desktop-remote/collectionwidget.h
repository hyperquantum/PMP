/*
    Copyright (C) 2016-2025, Kevin André <hyperquantum@gmail.com>

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
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

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
    class ColorSwitcher;
    class FilteredCollectionTableModel;
    class FiltersListWidget;
    class SearchData;
    class SortedCollectionTableModel;
    enum class TrackCriterium;
    class UserForStatisticsDisplay;
    class WaitingSpinnerWidget;

    class CollectionWidget : public QWidget
    {
        Q_OBJECT

    public:
        CollectionWidget(QWidget* parent, Client::ServerInterface* serverInterface,
                         Client::QueueHashesMonitor* queueHashesMonitor,
                         UserForStatisticsDisplay* userForStatisticsDisplay,
                         SearchData* searchData);
        ~CollectionWidget();

    protected:
        void changeEvent(QEvent* event) override;

    private Q_SLOTS:
        void onFiltersChanged();
        void highlightTracksIndexChanged(int index);
        void highlightColorIndexChanged();
        void collectionContextMenuRequested(const QPoint& position);
        void rowCountChanged();

    private:
        void updateSpinnerVisibility();
        void initTrackFilterWidgets();
        void initTrackHighlightingWidgets();
        void updateColors(bool force);

        void fillTrackCriteriaComboBox(QComboBox* comboBox,
                                       TrackCriterium criteriumForNone);

        TrackCriterium getCurrentHighlightMode() const;
        TrackCriterium getTrackCriteriumFromComboBox(QComboBox* comboBox) const;

        Ui::CollectionWidget* _ui;
        WaitingSpinnerWidget* _spinner { nullptr };
        FiltersListWidget* _filtersListWidget { nullptr };
        ColorSwitcher* _colorSwitcher;
        Client::ServerInterface* _serverInterface;
        UserForStatisticsDisplay* _userStatisticsDisplay;
        SortedCollectionTableModel* _collectionSourceModel;
        FilteredCollectionTableModel* _collectionDisplayModel;
        QMenu* _collectionContextMenu;
        bool _usingColorsForDarkMode { false };
    };

    class FilterLineWidget : public QWidget
    {
        Q_OBJECT
    public:
        FilterLineWidget();

        TrackCriterium criterium() const { return _criterium; }

    Q_SIGNALS:
        void criteriumChanged();
        void deleteClicked();

    private:
        void fillTrackCriteriaComboBox(QComboBox* comboBox,
                                       TrackCriterium criteriumForNone);

        QComboBox* _comboBox;
        QPushButton* _deleteButton;
        QPushButton* _resetButton;
        TrackCriterium _criterium;
    };

    class FiltersListWidget : public QWidget
    {
        Q_OBJECT
    public:
        FiltersListWidget();

        QList<TrackCriterium> criteria() const;

    Q_SIGNALS:
        void criteriaChanged();

    private:
        void addFilterLine();

        QPushButton* _addButton;
        QVBoxLayout* _verticalLayout;
        QList<FilterLineWidget*> _filters;
    };
}
#endif

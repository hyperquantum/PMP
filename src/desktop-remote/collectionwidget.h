/*
    Copyright (C) 2016-2026, Kevin André <hyperquantum@gmail.com>

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

#include "common/trackcriteria.h"

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
    class FilterPickerWidget;
    class FiltersListWidget;
    class SearchData;
    class SortedCollectionTableModel;
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
        void onHighlightCriteriumChanged();
        void highlightColorIndexChanged();
        void collectionContextMenuRequested(const QPoint& position);
        void rowCountChanged();

    private:
        void updateSpinnerVisibility();
        void initTrackFilterWidgets();
        void initTrackHighlightingWidgets();
        void updateColors(bool force);

        Ui::CollectionWidget* _ui;
        WaitingSpinnerWidget* _spinner { nullptr };
        FiltersListWidget* _filtersListWidget { nullptr };
        FilterPickerWidget* _highlightingCriteriumPicker { nullptr };
        ColorSwitcher* _colorSwitcher;
        Client::ServerInterface* _serverInterface;
        UserForStatisticsDisplay* _userStatisticsDisplay;
        SortedCollectionTableModel* _collectionSourceModel;
        FilteredCollectionTableModel* _collectionDisplayModel;
        QMenu* _collectionContextMenu;
        bool _usingColorsForDarkMode { false };
    };

    class FilterPickerWidget : public QWidget
    {
        Q_OBJECT
    public:
        FilterPickerWidget(PredefinedTrackCriterium criteriumForEmpty, QString captionForEmpty);

        void clearCriterium();
        const TrackCriterium& criterium() const { return *_criterium; }

    Q_SIGNALS:
        void criteriumChanged();

    private:
        void fillTrackCriteriaComboBox(QComboBox* comboBox,
                                       PredefinedTrackCriterium criteriumForEmpty,
                                       QString captionForEmpty);

        QComboBox* _comboBox;
        PredefinedTrackCriterium _predefinedCriterium;
        std::unique_ptr<TrackCriterium> _criterium;
    };

    class FilterLineWidget : public QWidget
    {
        Q_OBJECT
    public:
        FilterLineWidget();

        const TrackCriterium& criterium() const { return *_criterium; }

    Q_SIGNALS:
        void criteriumChanged();
        void deleteClicked();

    private:
        FilterPickerWidget* _filterPicker;
        QPushButton* _deleteButton;
        QPushButton* _resetButton;
        std::unique_ptr<TrackCriterium> _criterium;
    };

    class FiltersListWidget : public QWidget
    {
        Q_OBJECT
    public:
        FiltersListWidget();

        const TrackCriterium& criterium() const { return *_criterium; }

    Q_SIGNALS:
        void criteriumChanged();

    private:
        void addFilterLine();
        void rebuildCriterium();

        QPushButton* _addButton;
        QVBoxLayout* _verticalLayout;
        QList<FilterLineWidget*> _filters;
        std::unique_ptr<TrackCriterium> _criterium;
    };
}
#endif

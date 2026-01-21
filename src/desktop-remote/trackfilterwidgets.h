/*
    Copyright (C) 2025-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_TRACKFILTERWIDGETS_H
#define PMP_TRACKFILTERWIDGETS_H

#include "common/trackcriteria.h"

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace PMP
{
    class FilterPickerWidget : public QWidget
    {
        Q_OBJECT
    public:
        FilterPickerWidget(PredefinedTrackCriterium criteriumForEmpty,
                           QString captionForEmpty);

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

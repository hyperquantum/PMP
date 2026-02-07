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
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QSpinBox)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace PMP
{
    class FilterLabelWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit FilterLabelWidget(QWidget* parent);

        void setCriterium(std::unique_ptr<TrackCriterium> criterium);

        std::unique_ptr<TrackCriterium> createCriterium() const;

    private:
        class CriteriumCaptionGenerator : public TrackCriteriumVisitor
        {
        public:
            QString caption() const { return _caption; }

            void visit(const ConstantTrackCriterium&) override;
            void visit(const TrackLengthPresenceCriterium&) override;
            void visit(const TrackLengthComparisonCriterium&) override;
            void visit(const TrackScorePresenceCriterium&) override;
            void visit(const TrackScoreComparisonCriterium&) override;
            void visit(const TrackLastHeardPresenceCriterium&) override;
            void visit(const TrackLastHeardRecentlyCriterium&) override;
            void visit(const TrackQueuePresenceCriterium&) override;
            void visit(const TrackAvailabilityCriterium&) override;
            void visit(const TrackMetaDataPresenceCriterium&) override;
            void visit(const CompositeTrackCriterium&) override;

        private:
            QString toString(ComparisonOperator comparisonOperator);

            QString _caption;
        };

        std::unique_ptr<TrackCriterium> _criterium;
        QLabel* _label;
    };

    class FilterPickerWidget : public QWidget
    {
        Q_OBJECT
    public:
        FilterPickerWidget(PredefinedTrackCriterium criteriumForEmpty,
                           QString captionForEmpty);

        void clearCriterium();
        std::unique_ptr<TrackCriterium> createCriterium() const;

    Q_SIGNALS:
        void criteriumChanged();

    private:
        void fillTrackCriteriaComboBox(QComboBox* comboBox,
                                       PredefinedTrackCriterium criteriumForEmpty,
                                       QString captionForEmpty);

        QComboBox* _comboBox;
        PredefinedTrackCriterium _predefinedCriterium;
    };

    class FilterEditorWidget : public QWidget
    {
        Q_OBJECT
    public:
        virtual ~FilterEditorWidget() = default;

        virtual std::unique_ptr<TrackCriterium> createCriterium() const = 0;

    Q_SIGNALS:
        void criteriumChanged();

    protected:
        explicit FilterEditorWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    };

    class ScoreComparisonEditorWidget : public FilterEditorWidget
    {
        Q_OBJECT
    public:
        explicit ScoreComparisonEditorWidget(QWidget* parent);
        void setOperator(ComparisonOperator comparisonOperator);
        void setScore(int score);

        std::unique_ptr<TrackCriterium> createCriterium() const override;

    private:
        QComboBox* _operatorComboBox;
        QSpinBox* _scoreSpinBox;
    };

    class LastHeardEditorWidget : public FilterEditorWidget
    {
        Q_OBJECT
    public:
        explicit LastHeardEditorWidget(QWidget* parent);

        void setInverted(bool isInverted);
        void setPeriod(int years, int days);

        std::unique_ptr<TrackCriterium> createCriterium() const override;

    private:
        QComboBox* _inversionComboBox;
        QSpinBox* _yearsSpinBox;
        QSpinBox* _daysSpinBox;
        quint8 _suspendChangeSignal { 0 };
    };

    class LengthComparisonEditorWidget : public FilterEditorWidget
    {
        Q_OBJECT
    public:
        explicit LengthComparisonEditorWidget(QWidget* parent);
        void setOperator(ComparisonOperator comparisonOperator);
        void setLength(int hours, int minutes, int seconds);

        std::unique_ptr<TrackCriterium> createCriterium() const override;

    private:
        QComboBox* _operatorComboBox;
        QSpinBox* _hoursSpinBox;
        QSpinBox* _minutesSpinBox;
        QSpinBox* _secondsSpinBox;
        quint8 _suspendChangeSignal { 0 };
    };

    class FilterEditorFactory
    {
    public:
        static bool isEditable(TrackCriterium const& criterium);
        static FilterEditorWidget* createEditor(QWidget* parent,
                                                TrackCriterium const& criterium);

    private:
        class IsEditableVisitor final : public TrackCriteriumVisitor
        {
        public:
            bool isCriteriumEditable() const { return _isEditable; }

            void visit(const ConstantTrackCriterium&) override;
            void visit(const TrackLengthPresenceCriterium&) override;
            void visit(const TrackLengthComparisonCriterium&) override;
            void visit(const TrackScorePresenceCriterium&) override;
            void visit(const TrackScoreComparisonCriterium&) override;
            void visit(const TrackLastHeardPresenceCriterium&) override;
            void visit(const TrackLastHeardRecentlyCriterium&) override;
            void visit(const TrackQueuePresenceCriterium&) override;
            void visit(const TrackAvailabilityCriterium&) override;
            void visit(const TrackMetaDataPresenceCriterium&) override;
            void visit(const CompositeTrackCriterium&) override;

        private:
            bool _isEditable { false };
        };

        class EditorWidgetCreationVisitor final : public TrackCriteriumVisitor
        {
        public:
            explicit EditorWidgetCreationVisitor(QWidget* parent);

            FilterEditorWidget* editorWidget() const { return _editorWidget; }

            void visit(const ConstantTrackCriterium&) override;
            void visit(const TrackLengthPresenceCriterium&) override;
            void visit(const TrackLengthComparisonCriterium&) override;
            void visit(const TrackScorePresenceCriterium&) override;
            void visit(const TrackScoreComparisonCriterium&) override;
            void visit(const TrackLastHeardPresenceCriterium&) override;
            void visit(const TrackLastHeardRecentlyCriterium&) override;
            void visit(const TrackQueuePresenceCriterium&) override;
            void visit(const TrackAvailabilityCriterium&) override;
            void visit(const TrackMetaDataPresenceCriterium&) override;
            void visit(const CompositeTrackCriterium&) override;

        private:
            QWidget* _parent;
            FilterEditorWidget* _editorWidget;
        };
    };

    class FilterLineWidget : public QWidget
    {
        Q_OBJECT
    public:
        FilterLineWidget();

        std::unique_ptr<TrackCriterium> createCriterium() const;

    Q_SIGNALS:
        void criteriumChanged();
        void deleteClicked();

    private Q_SLOTS:
        void onPickerCriteriumChanged();
        void onEditClicked();
        void onResetClicked();

    private:
        void switchEditorToLabel();
        void switchToLabel(QWidget* widgetToReplace,
                           std::unique_ptr<TrackCriterium> criterium);
        void switchLabelToEditor();
        void switchPickerToEditor();
        void switchToEditor(QWidget* widgetToReplace, TrackCriterium const& criterium);

        FilterLabelWidget* _labelWidget;
        FilterPickerWidget* _filterPicker;
        FilterEditorWidget* _editorWidget;
        QPushButton* _editButton;
        QPushButton* _okButton;
        QPushButton* _deleteButton;
        QPushButton* _resetButton;
    };

    class FiltersListWidget : public QWidget
    {
        Q_OBJECT
    public:
        FiltersListWidget();

        std::unique_ptr<TrackCriterium> createCriterium() const;

    Q_SIGNALS:
        void criteriumChanged();

    private:
        void addFilterLine();

        QPushButton* _addButton;
        QVBoxLayout* _verticalLayout;
        QList<FilterLineWidget*> _filters;
    };
}
#endif

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

#include <QPointer>
#include <QWidget>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QSpinBox)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace PMP::Client
{
    class ServerInterface;
    class TrackLabelsController;
}

namespace PMP
{
    class ClickableLabel;

    class FilterLabelWidget : public QWidget
    {
        Q_OBJECT
    public:
        FilterLabelWidget(QWidget* parent, Client::ServerInterface* serverInterface);

        void setCriterium(std::unique_ptr<TrackCriterium> criterium);

        std::unique_ptr<TrackCriterium> createCriterium() const;

    Q_SIGNALS:
        void editingRequested();

    private:
        class CriteriumCaptionGenerator : public TrackCriteriumVisitor
        {
        public:
            CriteriumCaptionGenerator(Client::ServerInterface* serverInterface);

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
            void visit(const TrackLabelPresenceCriterium&) override;
            void visit(const CompositeTrackCriterium&) override;

        private:
            QString toString(ComparisonOperator comparisonOperator);

            Client::ServerInterface* _serverInterface;
            QString _caption;
        };

        Client::ServerInterface* _serverInterface;
        std::unique_ptr<TrackCriterium> _criterium;
        ClickableLabel* _label;
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
        void setPeriod(int years, int days, int hours);

        std::unique_ptr<TrackCriterium> createCriterium() const override;

    private:
        QComboBox* _inversionComboBox;
        QSpinBox* _yearsSpinBox;
        QSpinBox* _daysSpinBox;
        QSpinBox* _hoursSpinBox;
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

    class LabelPresenceEditorWidget : public FilterEditorWidget
    {
        Q_OBJECT
    public:
        explicit LabelPresenceEditorWidget(QWidget* parent,
                                           Client::ServerInterface* serverInterface);
        void setInverted(bool isInverted);
        void setLabel(quint32 labelId);

        std::unique_ptr<TrackCriterium> createCriterium() const override;

    private:
        Client::ServerInterface* _serverInterface;
        QComboBox* _inversionComboBox;
        QComboBox* _labelComboBox;
        quint32 _labelIdToSelect { 0 };
        QHash<quint32, QString> _labelIdsToNames;
        quint8 _ignoreLabelComboBoxIndexChanges { 0 };
        bool _labelsLoaded { false };
    };

    class FilterEditorFactory
    {
    public:
        static bool isEditable(TrackCriterium const& criterium);
        static FilterEditorWidget* createFromCriterium(QWidget* parent,
                                                       TrackCriterium const& criterium,
                                                Client::ServerInterface* serverInterface);

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
            void visit(const TrackLabelPresenceCriterium&) override;
            void visit(const CompositeTrackCriterium&) override;

        private:
            bool _isEditable { false };
        };

        class EditorWidgetCreationVisitor final : public TrackCriteriumVisitor
        {
        public:
            explicit EditorWidgetCreationVisitor(QWidget* parent,
                                                Client::ServerInterface* serverInterface);

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
            void visit(const TrackLabelPresenceCriterium&) override;
            void visit(const CompositeTrackCriterium&) override;

        private:
            QWidget* _parent;
            Client::ServerInterface* _serverInterface;
            FilterEditorWidget* _editorWidget;
        };
    };

    class ModifiableFilter
    {
    public:
        virtual ~ModifiableFilter() = default;

        virtual void clearFilter() = 0;
        virtual void setFilterToCriterium(std::unique_ptr<TrackCriterium> criterium) = 0;
        virtual void setFilterToEditor(FilterEditorWidget* editor) = 0;
    };

    class FilterLineWidget : public QWidget, public ModifiableFilter
    {
        Q_OBJECT
    public:
        FilterLineWidget(Client::ServerInterface* serverInterface);

        void clearFilter() override;
        bool isEmpty() const { return _isEmpty; }

        void setFilterToCriterium(std::unique_ptr<TrackCriterium> criterium) override;
        void setFilterToEditor(FilterEditorWidget* editor) override;

        std::unique_ptr<TrackCriterium> createCriterium() const;

        void setDeleteButtonVisible(bool visible);
        void setResetButtonVisible(bool visible);

    Q_SIGNALS:
        void criteriumChanged();
        void deleteClicked();

    private Q_SLOTS:
        void onEmptyLabelClicked(QPoint position);
        void onEditClicked();
        void onResetClicked();

    private:
        class FilterSetter : public ModifiableFilter
        {
        public:
            explicit FilterSetter(FilterLineWidget* parent);

            void clearFilter() override;
            void setFilterToCriterium(std::unique_ptr<TrackCriterium> criterium) override;
            void setFilterToEditor(FilterEditorWidget* editor) override;

        private:
            QPointer<FilterLineWidget> _parent;
        };

        void init();
        void switchToEmpty(QWidget* widgetToReplace);
        void switchEditorToLabel();
        void switchToLabel(QWidget* widgetToReplace,
                           std::unique_ptr<TrackCriterium> criterium);
        void switchLabelToEditor();
        void switchToEditor(QWidget* widgetToReplace, TrackCriterium const& criterium);
        void switchToEditor(QWidget* widgetToReplace, FilterEditorWidget* editor);

        Client::ServerInterface* _serverInterface;
        ClickableLabel* _emptyFilterLabel;
        FilterLabelWidget* _labelWidget;
        FilterEditorWidget* _editorWidget;
        QPushButton* _editButton;
        QPushButton* _doneButton;
        QPushButton* _deleteButton;
        QPushButton* _resetButton;
        bool _isEmpty;
        bool _deleteVisible;
        bool _resetVisible;
    };

    class FiltersListWidget : public QWidget
    {
        Q_OBJECT
    public:
        FiltersListWidget(Client::ServerInterface* serverInterface);

        std::unique_ptr<TrackCriterium> createCriterium() const;

    Q_SIGNALS:
        void criteriumChanged();

    private Q_SLOTS:
        void showAddMenu();

    private:
        class FilterAdder : public ModifiableFilter
        {
        public:
            explicit FilterAdder(FiltersListWidget* parent);

            void clearFilter() override;
            void setFilterToCriterium(std::unique_ptr<TrackCriterium> criterium) override;
            void setFilterToEditor(FilterEditorWidget* editor) override;

        private:
            QPointer<FiltersListWidget> _parent;
        };

        void addEmptyFilterLine();
        void addFilterLineWithCriteriumAsLabel(std::unique_ptr<TrackCriterium> criterium);
        void addFilterLineWithEditorWidget(FilterEditorWidget* editor);
        void addFilterLineWithWidget(FilterLineWidget* filterLine);

        Client::ServerInterface* _serverInterface;
        QPushButton* _addMenuButton;
        QVBoxLayout* _verticalLayout;
        QList<FilterLineWidget*> _filters;
    };
}
#endif

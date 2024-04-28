/*
    Copyright (C) 2022-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "delayedstartdialog.h"
#include "ui_delayedstartdialog.h"

#include "common/util.h"

#include "client/abstractqueuemonitor.h"
#include "client/dynamicmodecontroller.h"
#include "client/playercontroller.h"
#include "client/queueentryinfostorage.h"
#include "client/serverinterface.h"

#include <QLocale>
#include <QMessageBox>
#include <QTimer>

using namespace PMP::Client;

namespace PMP
{
    PlayDurationCalculator::PlayDurationCalculator(QObject* parent,
                                                   ServerInterface* serverInterface)
     : QObject(parent),
       _serverInterface(serverInterface),
       _calculating(false),
       _mustRestartCalculation(false)
    {
        auto* dynamicModeController = &_serverInterface->dynamicModeController();
        connect(dynamicModeController, &DynamicModeController::dynamicModeEnabledChanged,
                this, &PlayDurationCalculator::onDynamicModeEnabledChanged);

        auto* queueMonitor = &serverInterface->queueMonitor();
        connect(queueMonitor, &AbstractQueueMonitor::queueResetted,
                this, &PlayDurationCalculator::triggerRecalculation);
        connect(
            queueMonitor, &AbstractQueueMonitor::entriesReceived,
            this,
            [this](int index)
            {
                if (_breakIndex.hasValue() && index > _breakIndex.value())
                    return;

                triggerRecalculation();
            }
        );
        connect(
            queueMonitor, &AbstractQueueMonitor::trackAdded,
            this,
            [this](int index)
            {
                if (_breakIndex.hasValue() && index > _breakIndex.value())
                    return;

                triggerRecalculation();
            }
        );
        connect(
            queueMonitor, &AbstractQueueMonitor::trackRemoved,
            this,
            [this](int index)
            {
                if (_breakIndex.hasValue() && index > _breakIndex.value())
                    return;

                triggerRecalculation();
            }
        );
        connect(
            queueMonitor, &AbstractQueueMonitor::trackMoved,
            this,
            [this](int fromIndex, int toIndex)
            {
                if (_breakIndex.isNull())
                    return;

                if (fromIndex < _breakIndex.value() && toIndex < _breakIndex.value())
                    return;

                if (fromIndex > _breakIndex.value() && toIndex > _breakIndex.value())
                    return;

                triggerRecalculation();
            }
        );

        auto* queueEntryInfoStorage = &serverInterface->queueEntryInfoStorage();
        connect(queueEntryInfoStorage, &QueueEntryInfoStorage::tracksChanged,
                this, &PlayDurationCalculator::triggerRecalculation);

        triggerRecalculation();
    }

    void PlayDurationCalculator::onDynamicModeEnabledChanged()
    {
        if (_breakIndex.hasValue())
            return; // dynamic mode status does not affect our calculation

        auto& dynamicModeController = _serverInterface->dynamicModeController();
        auto dynamicModeEnabled = dynamicModeController.dynamicModeEnabled();

        if (dynamicModeEnabled.isTrue())
        {
            _duration.setToNull();
            Q_EMIT resultChanged();
        }
        else if (dynamicModeEnabled.isFalse())
        {
            triggerRecalculation();
        }
        else
        {
            _duration = null;
            Q_EMIT resultChanged();
        }
    }

    void PlayDurationCalculator::triggerRecalculation()
    {
        if (_calculating)
        {
            _mustRestartCalculation = true;
            return;
        }

        //QtConcurrent::run(&calculate, this);
        QTimer::singleShot(0, this, [this]() { calculate(this); });
    }

    void PlayDurationCalculator::calculate(PlayDurationCalculator* calculator)
    {
        auto* serverInterface = calculator->_serverInterface;

        auto& dynamicModeController = serverInterface->dynamicModeController();

        auto& queueMonitor = serverInterface->queueMonitor();
        auto& queueEntryInfoStorage = serverInterface->queueEntryInfoStorage();

        Nullable<int> breakIndex;
        Nullable<qint64> duration;

        auto queueLength = queueMonitor.queueLength();
        if (queueLength >= 0)
        {
            qint64 durationSum = 0;
            for (int i = 0; i < queueLength; ++i)
            {
                auto queueEntryId = queueMonitor.queueEntry(i);
                auto* entryInfo = queueEntryInfoStorage.entryInfoByQueueId(queueEntryId);
                if (!entryInfo)
                {
                    durationSum = -1;
                    break;
                }

                auto entryType = entryInfo->type();
                auto entryDuration = entryInfo->lengthInMilliseconds();
                switch (entryType)
                {
                case QueueEntryType::BreakPoint:
                case QueueEntryType::Barrier:
                    breakIndex = i;
                    break;
                case QueueEntryType::Track:
                    if (entryDuration < 0)
                        durationSum = -1;
                    else
                        durationSum += entryDuration;
                    break;
                default:
                    durationSum = -1;
                    break;
                }

                if (breakIndex.hasValue() || durationSum < 0)
                    break;
            }

            if (durationSum >=0
                    && (breakIndex.hasValue()
                        || dynamicModeController.dynamicModeEnabled().isFalse()))
            {
                duration = durationSum;
            }
        }

        QTimer::singleShot(
            0, calculator,
            [calculator, breakIndex, duration]()
            {
                calculator->_calculating = false;
                calculator->_breakIndex = breakIndex;
                calculator->_duration = duration;

                if (calculator->_mustRestartCalculation)
                    calculator->triggerRecalculation();

                Q_EMIT calculator->resultChanged();
            }
        );
    }

    /* ====================================== */

    DelayedStartDialog::DelayedStartDialog(QWidget* parent,
                                           ServerInterface* serverInterface)
     : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
       _ui(new Ui::DelayedStartDialog),
       _serverInterface(serverInterface),
       _playDurationCalculator(new PlayDurationCalculator(this, serverInterface))
    {
        _ui->setupUi(this);

        QLocale locale;

        QDateTime now { QDateTime::currentDateTime() };
        QDateTime suggestion { now.addSecs(60 * 5) };
        QTime suggestionTime = suggestion.time();
        suggestionTime.setHMS(suggestionTime.hour(), suggestionTime.minute(), 0);
        suggestion.setTime(suggestionTime);

        _ui->dateTimeEdit->setDateTime(suggestion);
        _ui->dateTimeEdit->setMinimumDateTime(now);
        _ui->dateTimeEdit->setDisplayFormat(locale.dateTimeFormat(QLocale::LongFormat));

        connect(_ui->dateTimeEdit, &QDateTimeEdit::dateTimeChanged,
                this, [this]() { _ui->clockTimeRadioButton->setChecked(true); });
        connect(_ui->dateTimeEdit, &QDateTimeEdit::dateTimeChanged,
                this, &DelayedStartDialog::updateEstimatedEndTime);

        connect(_ui->hoursSpinBox, qOverload<int>(&QSpinBox::valueChanged),
                this, [this]() { _ui->delayRadioButton->setChecked(true); });
        connect(_ui->minutesSpinBox, qOverload<int>(&QSpinBox::valueChanged),
                this, [this]() { _ui->delayRadioButton->setChecked(true); });
        connect(_ui->secondsSpinBox, qOverload<int>(&QSpinBox::valueChanged),
                this, [this]() { _ui->delayRadioButton->setChecked(true); });

        connect(_playDurationCalculator, &PlayDurationCalculator::resultChanged,
                this, &DelayedStartDialog::updateEstimatedEndTime);

        updateEstimatedEndTime();
    }

    DelayedStartDialog::~DelayedStartDialog()
    {
        delete _ui;
    }

    void DelayedStartDialog::done(int r)
    {
        if (r == Rejected)
        {
            QDialog::done(r);
            return;
        }

        if (!_ui->clockTimeRadioButton->isChecked()
            && !_ui->delayRadioButton->isChecked())
        {
            QMessageBox::warning(this, tr("Delayed start"),
                                 tr("Please select one of the two options."));
            return;
        }

        if (_ui->clockTimeRadioButton->isChecked())
        {
            auto now = QDateTime::currentDateTime();
            auto deadline = _ui->dateTimeEdit->dateTime();
            if (deadline <= now)
            {
                QMessageBox::warning(this, tr("Delayed start"),
                                     tr("The date/time must be in the future."));
                return;
            }

            auto future =
                _serverInterface->playerController().activateDelayedStart(deadline);

            future.addResultListener(
                this,
                [this](AnyResultMessageCode code) { activationResultReceived(code); }
            );
        }
        else
        {
            auto hours = _ui->hoursSpinBox->value();
            auto minutes = _ui->minutesSpinBox->value();
            auto seconds = _ui->secondsSpinBox->value();

            auto millisecondsTotal =
                    hours * 60 * 60 * 1000
                     + minutes * 60 * 1000
                     + seconds * 1000;

            if (millisecondsTotal <= 0)
            {
                QMessageBox::warning(this, tr("Delayed start"),
                                     tr("The waiting time must be non-zero."));
                return;
            }

            auto future =
                _serverInterface->playerController().activateDelayedStart(
                                                                       millisecondsTotal);
            future.addResultListener(
                this,
                [this](AnyResultMessageCode code) { activationResultReceived(code); }
            );
        }

        _ui->buttonBox->setEnabled(false);
    }

    void DelayedStartDialog::updateEstimatedEndTime()
    {
        if (!_playDurationCalculator->calculationFinished())
        {
            _ui->estimatedTracksDurationValueLabel->setText(tr("calculating..."));
            _ui->estimatedStopTimeValueLabel->setText("");
            return;
        }

        auto duration = _playDurationCalculator->duration();
        if (!duration.hasValue())
        {
            _ui->estimatedTracksDurationValueLabel->setText(tr("N/A"));
            _ui->estimatedStopTimeValueLabel->setText(tr("N/A"));
            return;
        }

        auto durationMilliseconds = duration.value();
        auto end = _ui->dateTimeEdit->dateTime().addMSecs(durationMilliseconds);

        QLocale locale;

        _ui->estimatedTracksDurationValueLabel->setText(
                    Util::millisecondsToShortDisplayTimeText(durationMilliseconds));
        _ui->estimatedStopTimeValueLabel->setText(
                    end.toString(locale.dateTimeFormat(QLocale::LongFormat)));
    }

    void DelayedStartDialog::activationResultReceived(AnyResultMessageCode errorCode)
    {
        if (succeeded(errorCode))
        {
            QDialog::done(Accepted);
            return;
        }

        QString failureDetail;
        if (errorCode == ResultMessageErrorCode::OperationAlreadyRunning)
        {
            failureDetail = tr("Delayed start is already active.");
        }
        else
        {
            failureDetail =
                tr("Unspecified error (code %1).").arg(errorCodeString(errorCode));
        }

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Failed to activate delayed start."));
        msgBox.setInformativeText(failureDetail);
        msgBox.exec();

        QDialog::reject();
    }
}

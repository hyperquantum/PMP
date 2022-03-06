/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/clientserverinterface.h"
#include "common/playercontroller.h"

#include <QLocale>
#include <QMessageBox>

namespace PMP
{
    DelayedStartDialog::DelayedStartDialog(QWidget* parent,
                                           ClientServerInterface* clientServerInterface)
     : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
       _ui(new Ui::DelayedStartDialog),
       _clientServerInterface(clientServerInterface)
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

        connect(_ui->hoursSpinBox, qOverload<int>(&QSpinBox::valueChanged),
                this, [this]() { _ui->delayRadioButton->setChecked(true); });
        connect(_ui->minutesSpinBox, qOverload<int>(&QSpinBox::valueChanged),
                this, [this]() { _ui->delayRadioButton->setChecked(true); });
        connect(_ui->secondsSpinBox, qOverload<int>(&QSpinBox::valueChanged),
                this, [this]() { _ui->delayRadioButton->setChecked(true); });

        auto* playerController = &_clientServerInterface->playerController();
        connect(playerController, &PlayerController::delayedStartActivationResultEvent,
                this, &DelayedStartDialog::activationResultReceived);
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

            _requestId =
                _clientServerInterface->playerController().activateDelayedStart(deadline);
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

            _requestId =
                _clientServerInterface->playerController().activateDelayedStart(
                                                                       millisecondsTotal);
        }

        _ui->buttonBox->setEnabled(false);
    }

    void DelayedStartDialog::activationResultReceived(ResultMessageErrorCode errorCode)
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
            failureDetail = tr("Unspecified error (code %1).").arg(int(errorCode));
        }

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Failed to activate delayed start."));
        msgBox.setInformativeText(failureDetail);
        msgBox.exec();

        QDialog::reject();
    }
}

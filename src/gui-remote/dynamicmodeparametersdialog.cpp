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

#include "dynamicmodeparametersdialog.h"
#include "ui_dynamicmodeparametersdialog.h"

#include "client/dynamicmodecontroller.h"

#include <QtDebug>

using namespace PMP::Client;

namespace PMP
{
    DynamicModeParametersDialog::DynamicModeParametersDialog(QWidget* parent,
                                             DynamicModeController* dynamicModeController)
     : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
       _ui(new Ui::DynamicModeParametersDialog),
       _dynamicModeController(dynamicModeController)
    {
        _ui->setupUi(this);

        connect(
            dynamicModeController, &DynamicModeController::dynamicModeEnabledChanged,
            this, &DynamicModeParametersDialog::dynamicModeEnabledChanged
        );
        connect(
            _ui->enableDynamicModeCheckBox, &QCheckBox::stateChanged,
            this, &DynamicModeParametersDialog::changeDynamicModeEnabled
        );

        connect(
            dynamicModeController, &DynamicModeController::waveStatusChanged,
            this, &DynamicModeParametersDialog::highScoredModeStatusChanged
        );
        connect(
            _ui->terminateButton, &QPushButton::clicked,
            this, &DynamicModeParametersDialog::terminateHighScoredTracksMode
        );
        connect(
            _ui->startHighScoredModeButton, &QPushButton::clicked,
            this, &DynamicModeParametersDialog::startHighScoredTracksMode
        );

        connect(
            dynamicModeController, &DynamicModeController::noRepetitionSpanSecondsChanged,
            this, &DynamicModeParametersDialog::noRepetitionSpanSecondsChanged
        );
        connect(
            _ui->trackRepetitionComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DynamicModeParametersDialog::noRepetitionIndexChanged
        );

        connect(
            _ui->closeButton, &QPushButton::clicked,
            this, &DynamicModeParametersDialog::close
        );

        dynamicModeEnabledChanged();
        highScoredModeStatusChanged();
        noRepetitionSpanSecondsChanged();
    }

    DynamicModeParametersDialog::~DynamicModeParametersDialog()
    {
        delete _ui;
    }

    void DynamicModeParametersDialog::dynamicModeEnabledChanged()
    {
        auto enabled = _dynamicModeController->dynamicModeEnabled();
        _ui->enableDynamicModeCheckBox->setEnabled(enabled.isKnown());
        _ui->enableDynamicModeCheckBox->setChecked(enabled.isTrue());
    }

    void DynamicModeParametersDialog::changeDynamicModeEnabled(int checkState)
    {
        if (checkState == Qt::Checked)
        {
            if (!_dynamicModeController->dynamicModeEnabled().isTrue())
            {
                _dynamicModeController->enableDynamicMode();
            }
        }
        else
        {
            if (!_dynamicModeController->dynamicModeEnabled().isFalse())
            {
                _dynamicModeController->disableDynamicMode();
            }
        }
    }

    void DynamicModeParametersDialog::startHighScoredTracksMode()
    {
        _dynamicModeController->startHighScoredTracksWave();
    }

    void DynamicModeParametersDialog::terminateHighScoredTracksMode()
    {
        _dynamicModeController->terminateHighScoredTracksWave();
    }

    void DynamicModeParametersDialog::highScoredModeStatusChanged()
    {
        auto highScoredModeActive = _dynamicModeController->waveActive();

        if (highScoredModeActive.isUnknown())
        {
            _ui->modeValueLabel->clear();
            _ui->terminateButton->setVisible(false);
            _ui->progressValueLabel->clear();
            _ui->startHighScoredModeButton->setEnabled(false);
        }
        else if (highScoredModeActive.isFalse())
        {
            _ui->modeValueLabel->setText(tr("normal mode"));
            _ui->terminateButton->setVisible(false);
            _ui->progressValueLabel->setText(tr("N/A"));
            _ui->startHighScoredModeButton->setEnabled(true);
        }
        else
        {
            int progress = _dynamicModeController->waveProgress();
            int progressTotal = _dynamicModeController->waveProgressTotal();

            _ui->modeValueLabel->setText(tr("high-scored tracks mode"));
            _ui->terminateButton->setVisible(true);

            if (progress < 0 || progressTotal <= 0)
            {
                _ui->progressValueLabel->clear();
            }
            else
            {
                auto progressText =
                    QString(tr("%1 / %2"))
                        .arg(QString::number(progress),
                             QString::number(progressTotal));

                _ui->progressValueLabel->setText(progressText);
            }

            _ui->startHighScoredModeButton->setEnabled(false);
        }
    }

    void DynamicModeParametersDialog::noRepetitionSpanSecondsChanged()
    {
        auto noRepetitionSpanSeconds = _dynamicModeController->noRepetitionSpanSeconds();

        _ui->trackRepetitionComboBox->setEnabled(noRepetitionSpanSeconds >= 0);

        if (noRepetitionSpanSeconds < 0)
        {
            _noRepetitionUpdating++;
            _ui->trackRepetitionComboBox->setCurrentIndex(-1);
            _noRepetitionUpdating--;
            return;
        }

        int noRepetitionIndex = _ui->trackRepetitionComboBox->currentIndex();

        if (noRepetitionIndex >= 0
                && _noRepetitionList[noRepetitionIndex] == noRepetitionSpanSeconds)
        {
            return; /* the right item is selected already */
        }

        /* search for non-repetition span in list of choices */
        noRepetitionIndex = -1;
        for (int i = 0; i < _noRepetitionList.size(); ++i)
        {
            if (_noRepetitionList[i] == noRepetitionSpanSeconds)
            {
                noRepetitionIndex = i;
                break;
            }
        }

        if (noRepetitionIndex >= 0)
        {
            /* found in list */
            _noRepetitionUpdating++;
            _ui->trackRepetitionComboBox->setCurrentIndex(noRepetitionIndex);
            _noRepetitionUpdating--;
        }
        else
        {
            /* not found in list */
            buildNoRepetitionList(noRepetitionSpanSeconds);
        }
    }

    void DynamicModeParametersDialog::noRepetitionIndexChanged(int index)
    {
        if (_noRepetitionUpdating > 0
            || index < 0 || index >= _noRepetitionList.size())
        {
            return;
        }

        int newSpan = _noRepetitionList[index];

        qDebug() << "noRepetitionIndexChanged: index" << index << "  value" << newSpan;

        _dynamicModeController->setNoRepetitionSpan(newSpan);
    }

    void DynamicModeParametersDialog::buildNoRepetitionList(int spanToSelect)
    {
        _noRepetitionUpdating++;

        _noRepetitionList.clear();
        _ui->trackRepetitionComboBox->clear();

        _noRepetitionList.append(0);
        _noRepetitionList.append(3600); // 1 hour
        _noRepetitionList.append(2 * 3600); // 2 hours
        _noRepetitionList.append(4 * 3600); // 4 hours
        _noRepetitionList.append(6 * 3600); // 6 hours
        _noRepetitionList.append(10 * 3600); // 10 hours
        _noRepetitionList.append(24 * 3600); // 24 hours
        _noRepetitionList.append(2 * 24 * 3600); // 48 hours (2 days)
        _noRepetitionList.append(3 * 24 * 3600); // 72 hours (3 days)
        _noRepetitionList.append(7 * 24 * 3600); // 7 days
        _noRepetitionList.append(2 * 7 * 24 * 3600); // 2 weeks
        _noRepetitionList.append(3 * 7 * 24 * 3600); // 3 weeks
        _noRepetitionList.append(4 * 7 * 24 * 3600); // 4 weeks
        _noRepetitionList.append(8 * 7 * 24 * 3600); // 8 weeks

        int indexOfSpanToSelect = -1;

        if (spanToSelect >= 0)
        {
            indexOfSpanToSelect = _noRepetitionList.indexOf(spanToSelect);
            if (indexOfSpanToSelect < 0)
            {
                _noRepetitionList.append(spanToSelect);
                std::sort(_noRepetitionList.begin(), _noRepetitionList.end());
                indexOfSpanToSelect = _noRepetitionList.indexOf(spanToSelect);
            }
        }

        for (auto span : qAsConst(_noRepetitionList))
        {
            _ui->trackRepetitionComboBox->addItem(noRepetitionTimeString(span));
        }

        if (indexOfSpanToSelect >= 0)
        {
            _ui->trackRepetitionComboBox->setCurrentIndex(indexOfSpanToSelect);
        }

        _noRepetitionUpdating--;
    }

    QString DynamicModeParametersDialog::noRepetitionTimeString(int seconds)
    {
        QString output;

        if (seconds > 7 * 24 * 60 * 60)
        {
            int weeks = seconds / (7 * 24 * 60 * 60);
            seconds -= weeks * (7 * 24 * 60 * 60);
            output += QString::number(weeks);
            output += (weeks == 1) ? " week" : " weeks";
        }

        if (seconds > 24 * 60 * 60)
        {
            int days = seconds / (24 * 60 * 60);
            seconds -= days * (24 * 60 * 60);
            if (output.size() > 0) output += " ";
            output += QString::number(days);
            output += (days == 1) ? " day" : " days";
        }

        if (seconds > 60 * 60)
        {
            int hours = seconds / (60 * 60);
            seconds -= hours * (60 * 60);
            if (output.size() > 0) output += " ";
            output += QString::number(hours);
            output += (hours == 1) ? " hour" : " hours";
        }

        if (seconds > 60)
        {
            int minutes = seconds / 60;
            seconds -= minutes * 60;
            if (output.size() > 0) output += " ";
            output += QString::number(minutes);
            output += (minutes == 1) ? " minute" : " minutes";
        }

        if (seconds > 0 || output.size() == 0)
        {
            if (output.size() > 0) output += " ";
            output += QString::number(seconds);
            output += (seconds == 1) ? " second" : " seconds";
        }

        return output;
    }
}

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

#ifndef PMP_DELAYEDSTARTDIALOG_H
#define PMP_DELAYEDSTARTDIALOG_H

#include "common/nullable.h"
#include "common/resultmessageerrorcode.h"
#include "common/tribool.h"

#include <QDialog>

namespace Ui
{
    class DelayedStartDialog;
}

namespace PMP
{
    class ClientServerInterface;
    class PlayDurationCalculator;

    class DelayedStartDialog : public QDialog
    {
        Q_OBJECT
    public:
        DelayedStartDialog(QWidget* parent, ClientServerInterface* clientServerInterface);
        ~DelayedStartDialog();

    public Q_SLOTS:
        void done(int r) override;

    private Q_SLOTS:
        void updateEstimatedEndTime();
        void activationResultReceived(ResultMessageErrorCode errorCode);

    private:
        Ui::DelayedStartDialog* _ui;
        ClientServerInterface* _clientServerInterface;
        PlayDurationCalculator* _playDurationCalculator;
    };

    class PlayDurationCalculator : public QObject
    {
        Q_OBJECT
    public:
        PlayDurationCalculator(QObject* parent,
                               ClientServerInterface* clientServerInterface);

        bool calculationFinished() const { return !_calculating; }
        Nullable<qint64> duration() const { return _duration; }

    Q_SIGNALS:
        void resultChanged();

    private Q_SLOTS:
        void onDynamicModeEnabledChanged();
        void triggerRecalculation();

    private:
        static void calculate(PlayDurationCalculator* calculator);

        ClientServerInterface* _clientServerInterface;
        TriBool _dynamicModeEnabled;
        Nullable<int> _breakIndex;
        Nullable<qint64> _duration;
        bool _calculating;
        bool _mustRestartCalculation;
    };
}
#endif

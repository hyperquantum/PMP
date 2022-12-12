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

#ifndef PMP_DYNAMICMODEPARAMETERSDIALOG_H
#define PMP_DYNAMICMODEPARAMETERSDIALOG_H

#include <QDialog>
#include <QList>

namespace Ui
{
    class DynamicModeParametersDialog;
}

namespace PMP::Client
{
    class DynamicModeController;
}

namespace PMP
{
    class DynamicModeParametersDialog : public QDialog
    {
        Q_OBJECT
    public:
        DynamicModeParametersDialog(QWidget* parent,
                                    Client::DynamicModeController* dynamicModeController);
        ~DynamicModeParametersDialog();

    private Q_SLOTS:
        void dynamicModeEnabledChanged();
        void changeDynamicModeEnabled(int checkState);

        void startHighScoredTracksMode();
        void terminateHighScoredTracksMode();
        void highScoredModeStatusChanged();

        void noRepetitionSpanSecondsChanged();
        void noRepetitionIndexChanged(int index);

    private:
        void buildNoRepetitionList(int spanToSelect);
        QString noRepetitionTimeString(int seconds);

        Ui::DynamicModeParametersDialog* _ui;
        Client::DynamicModeController* _dynamicModeController;
        int _noRepetitionUpdating { 0 };
        QList<int> _noRepetitionList;
    };
}
#endif

/*
    Copyright (C) 2025, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_INACTIVITYTIMER_H
#define PMP_CLIENT_INACTIVITYTIMER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP::Client
{
    class InactivityTimer : public QObject
    {
        Q_OBJECT
    public:
        explicit InactivityTimer(QObject* parent);

        void start();
        void stop();

    public Q_SLOTS:
        void reportActivity();

    Q_SIGNALS:
        void keepAliveTimeout();
        void inactivityTimeout();

    private:
        void onTimerTimeout();

    private:
        static constexpr int KeepAliveIntervalMs = 30 * 1000;
        static constexpr int SecondTimeoutStepTimeMs = 1000;
        static constexpr int SecondTimeoutMaximumTimeMs = 5 * SecondTimeoutStepTimeMs;

        QTimer* _timer;
        bool _started { false };
        bool _waitingForSecondTimeout { false };
        int _secondTimeoutTimePassedMs { 0 };
    };
}
#endif

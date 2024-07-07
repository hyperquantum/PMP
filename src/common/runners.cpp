/*
    Copyright (C) 2024, Kevin André <hyperquantum@gmail.com>

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

#include "runners.h"

#include <QTimer>

namespace PMP
{
    EventLoopRunner::EventLoopRunner(QObject* receiver)
        : _receiver(receiver)
    {
        //
    }

    void EventLoopRunner::run(std::function<void()> f)
    {
        QTimer::singleShot(0, _receiver, f);
    }

    // =================================================================== //

    ThreadPoolRunner::ThreadPoolRunner(QThreadPool* threadPool)
        : _threadPool(threadPool)
    {
        //
    }

    void ThreadPoolRunner::run(std::function<void()> f)
    {
        _threadPool->start(f);
    }
}

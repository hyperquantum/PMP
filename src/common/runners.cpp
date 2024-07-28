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

#include <QObject>
#include <QThreadPool>
#include <QTimer>

namespace PMP
{
    QThreadPool* ThreadPoolSpecifier::threadPool() const
    {
        if (_threadPool)
            return _threadPool;

        return QThreadPool::globalInstance();
    }

    // =================================================================== //

    EventLoopRunner::EventLoopRunner(QObject* receiver)
        : _receiver(receiver)
    {
        //
    }

    bool EventLoopRunner::canContinueInThreadFrom(Runner const* otherRunner) const
    {
        Q_UNUSED(otherRunner)

        // TODO: return true when appropriate
        return false;
    }

    void EventLoopRunner::run(std::function<void()> work)
    {
        QTimer::singleShot(0, _receiver, work);
    }

    // =================================================================== //

    ThreadPoolRunner::ThreadPoolRunner(ThreadPoolSpecifier threadPoolSpecifier)
        : _threadPoolSpecifier(threadPoolSpecifier)
    {
        //
    }

    bool ThreadPoolRunner::canContinueInThreadFrom(Runner const* otherRunner) const
    {
        Q_UNUSED(otherRunner)

        // TODO: return true when appropriate
        return false;
    }

    void ThreadPoolRunner::run(std::function<void()> work)
    {
        _threadPoolSpecifier.threadPool()->start(work);
    }

    // =================================================================== //

    bool AnyThreadContinuationRunner::canContinueInThreadFrom(
        Runner const* otherRunner) const
    {
        Q_UNUSED(otherRunner)

        /* continuing in the thread of the previous runner is the whole point of
           this class */
        return true;
    }

    void AnyThreadContinuationRunner::run(std::function<void ()> work)
    {
        /* not supposed to be run independently, but hey, just use the global thread pool
           instance in this case */
        QThreadPool::globalInstance()->start(work);
    }
}

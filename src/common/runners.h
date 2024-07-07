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

#ifndef PMP_COMMON_RUNNERS_H
#define PMP_COMMON_RUNNERS_H

// EventLoopRunner
#include <QObject>
#include <QTimer>

// ThreadPoolRunner
#include <QThreadPool>

#include <functional>

namespace PMP
{
    class Runner
    {
    public:
        virtual ~Runner() {}

        virtual void run(std::function<void()> f) = 0;

    protected:
        Runner() {}
    };

    // =================================================================== //

    class EventLoopRunner : public Runner
    {
    public:
        EventLoopRunner(QObject* receiver);

        void run(std::function<void()> f) override;

    private:
        QObject* _receiver;
    };

    inline EventLoopRunner::EventLoopRunner(QObject* receiver)
        : _receiver(receiver)
    {
        //
    }

    void EventLoopRunner::run(std::function<void()> f)
    {
        QTimer::singleShot(0, _receiver, f);
    }

    // =================================================================== //

    class ThreadPoolRunner : public Runner
    {
    public:
        ThreadPoolRunner(QThreadPool* threadPool);

        void run(std::function<void()> f) override;

    private:
        QThreadPool* _threadPool;
    };

    inline ThreadPoolRunner::ThreadPoolRunner(QThreadPool* threadPool)
        : _threadPool(threadPool)
    {
        //
    }

    inline void ThreadPoolRunner::run(std::function<void()> f)
    {
        _threadPool->start(f);
    }
}
#endif

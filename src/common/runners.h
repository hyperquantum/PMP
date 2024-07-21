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

#include <QtGlobal>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QObject)
QT_FORWARD_DECLARE_CLASS(QThreadPool)

namespace PMP
{
    class Runner
    {
    public:
        virtual ~Runner() {}

        virtual void run(std::function<void()> work) = 0;

    protected:
        Runner() {}
    };

    class EventLoopRunner : public Runner
    {
    public:
        EventLoopRunner(QObject* receiver);

        void run(std::function<void()> work) override;

    private:
        QObject* _receiver;
    };

    class ThreadPoolRunner : public Runner
    {
    public:
        ThreadPoolRunner(QThreadPool* threadPool);

        void run(std::function<void()> work) override;

    private:
        QThreadPool* _threadPool;
    };
}
#endif

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
    class GlobalThreadPoolType
    {
    public:
        constexpr GlobalThreadPoolType() {}

        constexpr bool operator==(GlobalThreadPoolType) { return true; }
        constexpr bool operator!=(GlobalThreadPoolType) { return false; }
    };

    constexpr GlobalThreadPoolType globalThreadPool = {};

    class ThreadPoolSpecifier
    {
    public:
        constexpr ThreadPoolSpecifier(QThreadPool* threadPool)
            : _threadPool(threadPool)
        {
            //
        }

        constexpr ThreadPoolSpecifier(GlobalThreadPoolType)
            : _threadPool(nullptr)
        {
            //
        }

        QThreadPool* threadPool() const;

    private:
        QThreadPool* _threadPool;
    };

    class Runner
    {
    public:
        virtual ~Runner() {}

        virtual bool canContinueInThreadFrom(Runner const* otherRunner) const = 0;

        virtual void run(std::function<void()> work) = 0;

    protected:
        Runner() {}
    };

    class EventLoopRunner : public Runner
    {
    public:
        EventLoopRunner(QObject* receiver);

        bool canContinueInThreadFrom(Runner const* otherRunner) const override;

        void run(std::function<void()> work) override;

    private:
        QObject* _receiver;
    };

    class ThreadPoolRunner : public Runner
    {
    public:
        ThreadPoolRunner(ThreadPoolSpecifier threadPoolSpecifier);

        bool canContinueInThreadFrom(Runner const* otherRunner) const override;

        void run(std::function<void()> work) override;

    private:
        ThreadPoolSpecifier _threadPoolSpecifier;
    };

    class AnyThreadContinuationRunner : public Runner
    {
    public:
        bool canContinueInThreadFrom(Runner const* otherRunner) const override;

        void run(std::function<void()> work) override;
    };
}
#endif

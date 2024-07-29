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

#ifndef PMP_COMMON_NEWASYNC_H
#define PMP_COMMON_NEWASYNC_H

#include "newfuture.h"
#include "newpromise.h"
#include "runners.h"

namespace PMP
{
    class NewAsync
    {
    public:
        template<class TResult, class TError>
        static NewPromise<TResult, TError> createPromise();

        template<class TOutcome>
        static NewSimplePromise<TOutcome> createSimplePromise();

        template<class TResult, class TError>
        static NewFuture<TResult, TError> runOnEventLoop(
            QObject* receiver,
            std::function<ResultOrError<TResult, TError>()> f);

        template<class TResult, class TError>
        static NewFuture<TResult, TError> runOnEventLoop(
            QObject* receiver,
            std::function<NewFuture<TResult, TError>()> f);

        template<class TOutcome>
        static NewSimpleFuture<TOutcome> runOnEventLoop(
            QObject* receiver,
            std::function<NewSimpleFuture<TOutcome>()> f);

    private:
        NewAsync();
    };

    template<class TResult, class TError>
    NewPromise<TResult, TError> NewAsync::createPromise()
    {
        return NewPromise<TResult, TError>();
    }

    template<class TOutcome>
    NewSimplePromise<TOutcome> NewAsync::createSimplePromise()
    {
        return NewSimplePromise<TOutcome>();
    }

    template<class TResult, class TError>
    NewFuture<TResult, TError> NewAsync::runOnEventLoop(
        QObject* receiver,
        std::function<ResultOrError<TResult, TError> ()> f)
    {
        auto runner = QSharedPointer<EventLoopRunner>::create(receiver);

        return NewFuture<TResult, TError>::createForRunnerDirect(runner, f);
    }

    template<class TResult, class TError>
    NewFuture<TResult, TError> NewAsync::runOnEventLoop(
        QObject* receiver,
        std::function<NewFuture<TResult, TError>()> f)
    {
        auto runner = QSharedPointer<EventLoopRunner>::create(receiver);

        return NewFuture<TResult, TError>::createForRunnerIndirect(runner, f);
    }

    template<class TOutcome>
    NewSimpleFuture<TOutcome> NewAsync::runOnEventLoop(
        QObject* receiver,
        std::function<NewSimpleFuture<TOutcome> ()> f)
    {
        auto runner = QSharedPointer<EventLoopRunner>::create(receiver);

        return NewSimpleFuture<TOutcome>::createForRunnerIndirect(runner, f);
    }
}
#endif

/*
    Copyright (C) 2024-2025, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_COMMON_CONCURRENT_H
#define PMP_COMMON_CONCURRENT_H

#include "future.h"
#include "runners.h"

namespace PMP
{
    class Concurrent
    {
    public:
        template<class TResult, class TError>
        static Future<TResult, TError> runOnThreadPool(
            ThreadPoolSpecifier threadPool,
            std::function<ResultOrError<TResult, TError>()> f);

        template<class TOutcome>
        static SimpleFuture<TOutcome> runOnThreadPool(
            ThreadPoolSpecifier threadPool,
            std::function<TOutcome ()> f);

    private:
        Concurrent();
    };

    template<class TResult, class TError>
    inline Future<TResult, TError> Concurrent::runOnThreadPool(
        ThreadPoolSpecifier threadPool,
        std::function<ResultOrError<TResult, TError> ()> f)
    {
        auto runner = QSharedPointer<ThreadPoolRunner>::create(threadPool);

        return Future<TResult, TError>::createForRunnerDirect(runner, f);
    }

    template<class TOutcome>
    inline SimpleFuture<TOutcome> Concurrent::runOnThreadPool(
        ThreadPoolSpecifier threadPool,
        std::function<TOutcome ()> f)
    {
        auto runner = QSharedPointer<ThreadPoolRunner>::create(threadPool);

        return SimpleFuture<TOutcome>::createForRunnerDirect(runner, f);
    }
}
#endif

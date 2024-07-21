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

#ifndef PMP_COMMON_NEWCONCURRENT_H
#define PMP_COMMON_NEWCONCURRENT_H

#include "newfuture.h"
#include "runners.h"

namespace PMP
{
    class NewConcurrent
    {
    public:
        template<class TResult, class TError>
        static NewFuture<TResult, TError> runOnThreadPool(
            ThreadPoolSpecifier threadPool,
            std::function<ResultOrError<TResult, TError>()> f);

    private:
        NewConcurrent();
    };

    template<class TResult, class TError>
    inline NewFuture<TResult, TError> NewConcurrent::runOnThreadPool(
        ThreadPoolSpecifier threadPool,
        std::function<ResultOrError<TResult, TError> ()> f)
    {
        auto runner = QSharedPointer<ThreadPoolRunner>::create(threadPool);

        return NewFuture<TResult, TError>::createForRunner(runner, f);
    }
}
#endif

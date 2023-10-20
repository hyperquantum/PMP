/*
    Copyright (C) 2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_ASYNC_H
#define PMP_ASYNC_H

#include "promise.h"
#include "resultorerror.h"

#include <QObject>
#include <QSharedPointer>
#include <QTimer>

#include <functional>

namespace PMP::Async
{
    template<class ResultType, class ErrorType>
    static Future<ResultType, ErrorType> invokeResultOrError(
        QObject* target,
        std::function<ResultOrError<ResultType, ErrorType>()> function)
    {
        auto promisePtr = QSharedPointer<Promise<ResultType, ErrorType>>::create();
        auto future = promisePtr->future();

        auto f =
            [promisePtr, function]()
            {
                promisePtr->setOutcome(function());
            };

        QTimer::singleShot(0, target, f);

        return future;
    }

    template<class T>
    static SimpleFuture<T> invokeSimpleFuture(
        QObject* target,
        std::function<SimpleFuture<T>()> function)
    {
        auto promisePtr = QSharedPointer<SimplePromise<T>>::create();
        auto future = promisePtr->future();

        auto f =
            [promisePtr, function]()
            {
                promisePtr->connectToResultFrom(function());
            };

        QTimer::singleShot(0, target, f);

        return future;
    }
}
#endif

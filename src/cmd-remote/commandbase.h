/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COMMANDBASE_H
#define PMP_COMMANDBASE_H

#include "common/future.h"
#include "common/resultmessageerrorcode.h"

#include "command.h"

#include <functional>

#include <QVector>

namespace PMP
{
    class CommandBase : public Command
    {
        Q_OBJECT
    public:
        virtual bool willCauseDisconnect() const override;

        virtual void execute(Client::ServerInterface* serverInterface) final;

    protected:
        CommandBase();

        // TODO : make return type of each step an enum instead of bool
        void addStep(std::function<bool ()> step);
        void setStepDelay(int milliseconds);
        void setCommandExecutionSuccessful(QString output = "");
        void setCommandExecutionFailed(int resultCode, QString errorOutput);
        void setCommandExecutionResult(ResultMessageErrorCode errorCode);
        void addCommandExecutionFutureListener(
                                             SimpleFuture<ResultMessageErrorCode> future);

        virtual void run(Client::ServerInterface* serverInterface) = 0;

    protected Q_SLOTS:
        void listenerSlot();

    private:
        int _currentStep;
        int _stepDelayMilliseconds;
        QVector<std::function<bool ()>> _steps;
        bool _finishedOrFailed;
    };
}
#endif

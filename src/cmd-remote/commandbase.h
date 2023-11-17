/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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
#include "common/nullable.h"
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
        struct CredentialsPrompt
        {
            QString providerName;
        };

        struct CredentialsEntered
        {
            QString username;
            QString password;
        };

        class StepResult;
        enum class StepResultType { StepIncomplete, StepCompleted, CommandFinished };

        CommandBase();

        void enableInteractiveCredentialsPrompt(CredentialsPrompt prompt);
        CredentialsEntered getCredentialsEntered();

        void addStep(std::function<StepResult ()> step);
        void setStepDelay(int milliseconds);
        void setCommandExecutionSuccessful(QString output = "");
        void setCommandExecutionFailed(int resultCode, QString errorOutput);
        void setCommandExecutionResult(AnyResultMessageCode code);
        void setCommandExecutionResult(ResultMessageErrorCode errorCode);
        void setCommandExecutionResult(ScrobblingResultMessageCode code);
        void addCommandExecutionFutureListener(SimpleFuture<AnyResultMessageCode> future);

        virtual void run(Client::ServerInterface* serverInterface) = 0;

    protected Q_SLOTS:
        void listenerSlot();

    private:
        void promptForCredentials(CredentialsPrompt const& prompt);
        void reportInternalError();
        void applyFinishedCommandFromStepResult(StepResult const& stepResult);

        int _currentStep;
        int _stepDelayMilliseconds;
        Nullable<CredentialsPrompt> _credentialsToAsk;
        Nullable<CredentialsEntered> _credentialsEntered;
        QVector<std::function<StepResult ()>> _steps;
        bool _finishedOrFailed;
        bool _stepsCompleted;
    };

    class CommandBase::StepResult
    {
    public:
        static StepResult stepCompleted()
        {
            return StepResult(StepResultType::StepCompleted);
        }

        static StepResult stepIncomplete()
        {
            return StepResult(StepResultType::StepIncomplete);
        }

        static StepResult commandCompleted()
        {
            return StepResult(StepResultType::CommandFinished);
        }

        static StepResult commandSuccessful()
        {
            return StepResult(0);
        }

        static StepResult commandSuccessful(QString output)
        {
            return StepResult(0, output);
        }

        static StepResult commandFailed(int errorCode, QString errorMessage)
        {
            return StepResult(errorCode, errorMessage);
        }

        static StepResult commandFailed(ResultMessageErrorCode error)
        {
            return StepResult(error);
        }

        StepResultType type() const { return _type; }
        Nullable<ResultMessageErrorCode> commandResult() const { return _commandResult; }
        Nullable<int> commandExitCode() const { return _commandExitCode; }
        QString commandOutput() const { return _commandOutput; }

    private:
        StepResult(StepResultType type)
         : _type(type)
        {
            //
        }

        StepResult(int commandExitCode)
         : _type(StepResultType::CommandFinished),
           _commandExitCode(commandExitCode)
        {
            //
        }

        StepResult(int commandExitCode, QString output)
         : _type(StepResultType::CommandFinished),
           _commandExitCode(commandExitCode),
           _commandOutput(output)
        {
            //
        }

        StepResult(ResultMessageErrorCode error)
         : _type(StepResultType::CommandFinished),
           _commandResult(error)
        {
            //
        }

        StepResultType _type;
        Nullable<ResultMessageErrorCode> _commandResult;
        Nullable<int> _commandExitCode;
        QString _commandOutput;
    };
}
#endif

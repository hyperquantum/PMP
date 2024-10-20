/*
    Copyright (C) 2020-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "commandbase.h"

#include "console.h"

#include <QtDebug>
#include <QTimer>

using namespace PMP::Client;

namespace PMP
{
    bool CommandBase::requiresAuthentication() const
    {
        // most commands require authentication
        return true;
    }

    bool CommandBase::willCauseDisconnect() const
    {
        // most commands won't
        return false;
    }

    void CommandBase::execute(ServerInterface* serverInterface)
    {
        if (_credentialsToAsk.hasValue())
        {
            promptForCredentials(_credentialsToAsk.value());
        }

        run(serverInterface);
        qDebug() << "CommandBase: called run()";

        // initial quick check
        if (!_steps.isEmpty())
            QTimer::singleShot(0, this, &CommandBase::listenerSlot);

        // set up timeout timer
        QTimer::singleShot(
            1000, this,
            [this]()
            {
                if (!_finishedOrFailed)
                {
                    qWarning() << "CommandBase: timeout triggered";
                    setCommandExecutionFailed(3, "Command timed out");
                }
            }
        );
    }

    CommandBase::CommandBase()
     : _currentStep(0),
       _stepDelayMilliseconds(0),
       _finishedOrFailed(false),
       _stepsCompleted(true)
    {
        //
    }

    void CommandBase::enableInteractiveCredentialsPrompt(CredentialsPrompt prompt)
    {
        _credentialsToAsk = prompt;
    }

    CommandBase::CredentialsEntered CommandBase::getCredentialsEntered()
    {
        return _credentialsEntered.value();
    }

    void CommandBase::addStep(std::function<StepResult ()> step)
    {
        _stepsCompleted = false;

        _steps.append(step);
    }

    void CommandBase::setStepDelay(int milliseconds)
    {
        _stepDelayMilliseconds = milliseconds;
    }

    void CommandBase::setCommandExecutionSuccessful(QString output)
    {
        qDebug() << "CommandBase: command reported success";
        _finishedOrFailed = true;
        Q_EMIT executionSuccessful(output);
    }

    void CommandBase::setCommandExecutionFailed(int resultCode, QString errorOutput)
    {
        qDebug() << "CommandBase: command reported failure, code:" << resultCode;
        _finishedOrFailed = true;
        Q_EMIT executionFailed(resultCode, errorOutput);
    }

    void CommandBase::setCommandExecutionResult(AnyResultMessageCode code)
    {
        if (std::holds_alternative<ResultMessageErrorCode>(code))
            setCommandExecutionResult(std::get<ResultMessageErrorCode>(code));
        else if (std::holds_alternative<ScrobblingResultMessageCode>(code))
            setCommandExecutionResult(std::get<ScrobblingResultMessageCode>(code));
        else
            Q_UNREACHABLE();
    }

    void CommandBase::setCommandExecutionResult(ResultMessageErrorCode errorCode)
    {
        QString errorOutput;

        switch (errorCode)
        {
        case ResultMessageErrorCode::NoError:
        case ResultMessageErrorCode::AlreadyDone:
            setCommandExecutionSuccessful();
            return;

        case ResultMessageErrorCode::NotLoggedIn:
            errorOutput = "not logged in";
            break;

        case ResultMessageErrorCode::InvalidUserAccountName:
            errorOutput = "invalid name for user account";
            break;

        case ResultMessageErrorCode::UserAccountAlreadyExists:
            errorOutput = "user account already exists";
            break;

        case ResultMessageErrorCode::UserLoginAuthenticationFailed:
            errorOutput = "invalid username or password";
            break;

        case ResultMessageErrorCode::AlreadyLoggedIn:
            errorOutput = "already logged in";
            break;

        case ResultMessageErrorCode::QueueIdNotFound:
            errorOutput = "queue ID not found";
            break;

        case ResultMessageErrorCode::UnknownAction:
            errorOutput = "server does not know how to handle this action";
            break;

        case PMP::ResultMessageErrorCode::InvalidHash:
            errorOutput = "invalid file hash";
            break;

        case PMP::ResultMessageErrorCode::InvalidQueueIndex:
            errorOutput = "invalid queue index";
            break;

        case ResultMessageErrorCode::InvalidQueueItemType:
            errorOutput = "invalid queue item type";
            break;

        case ResultMessageErrorCode::InvalidTimeSpan:
            errorOutput = "invalid time span";
            break;

        case ResultMessageErrorCode::InvalidUserId:
            errorOutput = "invalid user ID";
            break;

        case PMP::ResultMessageErrorCode::MaximumQueueSizeExceeded:
            errorOutput = "maximum queue size would be exceeded";
            break;

        case ResultMessageErrorCode::OperationAlreadyRunning:
            errorOutput = "operation cannot be started because it is already running";
            break;

        case ResultMessageErrorCode::DatabaseProblem:
            errorOutput = "problem with the server database";
            break;

        case ResultMessageErrorCode::ServerTooOld:
            errorOutput = "server is too old and does not support this action";
            break;

        case ResultMessageErrorCode::ExtensionNotSupported:
            errorOutput = "server does not support this feature";
            break;

        case ResultMessageErrorCode::ConnectionToServerBroken:
            errorOutput = "connection to the server was lost";
            break;

        case ResultMessageErrorCode::NonFatalInternalServerError:
            errorOutput = "internal server error (non-fatal)";
            break;

        case ResultMessageErrorCode::InvalidMessageStructure:
        case ResultMessageErrorCode::UserAccountRegistrationMismatch:
        case ResultMessageErrorCode::UserAccountLoginMismatch:
        case ResultMessageErrorCode::TooMuchDataToReturn:
        case ResultMessageErrorCode::NumberTooBigToReturn:
            errorOutput =
                    QString("client-server communication error (code %1)")
                       .arg(static_cast<int>(errorCode));
            break;

        case ResultMessageErrorCode::UnknownError:
            errorOutput = "unknown error";
            break;
        }

        if (errorOutput.isEmpty())
        {
            errorOutput = QString("error code %1").arg(errorCodeString(errorCode));
        }

        setCommandExecutionFailed(3, "Command failed: " + errorOutput);
    }

    void CommandBase::setCommandExecutionResult(ScrobblingResultMessageCode code)
    {
        QString errorOutput;

        switch (code)
        {
        case ScrobblingResultMessageCode::NoError:
            setCommandExecutionSuccessful();
            return;
        case ScrobblingResultMessageCode::ScrobblingSystemDisabled:
            errorOutput = "scrobbling system in the server is disabled";
            break;
        case ScrobblingResultMessageCode::ScrobblingProviderInvalid:
            errorOutput = "invalid scrobbling provider";
            break;
        case ScrobblingResultMessageCode::ScrobblingProviderNotEnabled:
            errorOutput = "scrobbling provider not enabled";
            break;
        case ScrobblingResultMessageCode::ScrobblingAuthenticationFailed:
            errorOutput = "scrobbling authentication failed";
            break;
        case ScrobblingResultMessageCode::UnspecifiedScrobblingBackendError:
            errorOutput = "unspecified scrobbling error";
            break;
        }

        if (errorOutput.isEmpty())
        {
            errorOutput = QString("error code %1").arg(errorCodeString(code));
        }

        setCommandExecutionFailed(3, "Command failed: " + errorOutput);
    }

    void CommandBase::setCommandExecutionResultFuture(
        SimpleFuture<AnyResultMessageCode> future)
    {
        future.handleOnEventLoop(
            this,
            [this](AnyResultMessageCode code) { setCommandExecutionResult(code); }
        );
    }

    void CommandBase::listenerSlot()
    {
        if (_finishedOrFailed || _stepsCompleted)
            return;

        auto const& stepResult = _steps[_currentStep]();

        switch (stepResult.type())
        {
        case StepResultType::StepIncomplete:
            return;

        case StepResultType::StepCompleted:
            if (_currentStep < _steps.size() - 1)
            {
                break; /* we will go to the next step */
            }
            else
            {
                _stepsCompleted = true;
                return;
            }

        case StepResultType::CommandFinished:
            applyFinishedCommandFromStepResult(stepResult);
            return;
        }

        if (_finishedOrFailed)
            return;

        _currentStep++;

        // we advanced, so try the next step right now, don't wait for signals
        QTimer::singleShot(_stepDelayMilliseconds, this, &CommandBase::listenerSlot);
    }

    void CommandBase::promptForCredentials(const CredentialsPrompt& prompt)
    {
        QString usernamePrompt, passwordPrompt;

        if (prompt.providerName.isEmpty())
        {
            usernamePrompt = "username: ";
            passwordPrompt = "password: ";
        }
        else
        {
            usernamePrompt = prompt.providerName + " username: ";
            passwordPrompt = prompt.providerName + " password: ";
        }

        CredentialsEntered credentials;
        credentials.username = Console::prompt(usernamePrompt);
        credentials.password = Console::promptForPassword(passwordPrompt);

        _credentialsEntered = credentials;
    }

    void CommandBase::reportInternalError()
    {
        _finishedOrFailed = true;
        Q_EMIT executionFailed(3, "internal error");
    }

    void CommandBase::applyFinishedCommandFromStepResult(const StepResult& stepResult)
    {
        auto resultCodeOrNull = stepResult.commandResult();
        if (resultCodeOrNull != null)
        {
            setCommandExecutionResult(resultCodeOrNull.value());
            return;
        }

        auto exitCodeOrNull = stepResult.commandExitCode();
        if (exitCodeOrNull != null)
        {
            auto errorCode = exitCodeOrNull.value();
            if (errorCode == 0)
            {
                setCommandExecutionSuccessful(stepResult.commandOutput());
            }
            else
            {
                setCommandExecutionFailed(errorCode, stepResult.commandOutput());
            }
            return;
        }

        if (_finishedOrFailed)
            return; /* result/error has been set already */

        qWarning() << "Step reported command completion, but no result/error was set";
        reportInternalError();
    }
}

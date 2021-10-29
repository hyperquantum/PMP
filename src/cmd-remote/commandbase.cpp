/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include <QtDebug>
#include <QTimer>

namespace PMP
{

    bool CommandBase::willCauseDisconnect() const
    {
        // most commands won't
        return false;
    }

    void CommandBase::execute(ClientServerInterface* clientServerInterface)
    {
        setUp(clientServerInterface);

        start(clientServerInterface);
        qDebug() << "CommandBase: called start()";

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
       _finishedOrFailed(false)
    {
        //
    }

    void CommandBase::addStep(std::function<bool ()> step)
    {
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

    void CommandBase::setCommandExecutionResult(ResultMessageErrorCode errorCode)
    {
        QString errorOutput;

        switch (errorCode)
        {
        case ResultMessageErrorCode::NoError:
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

        case ResultMessageErrorCode::DatabaseProblem:
            errorOutput = "problem with the server database";
            break;

        case ResultMessageErrorCode::ServerTooOld:
            errorOutput = "server is too old and does not support this action";
            break;

        case ResultMessageErrorCode::NonFatalInternalServerError:
            errorOutput = "internal server error (non-fatal)";
            break;

        case ResultMessageErrorCode::InvalidMessageStructure:
        case ResultMessageErrorCode::UserAccountRegistrationMismatch:
        case ResultMessageErrorCode::UserAccountLoginMismatch:
            errorOutput =
                    QString("client-server communication error (code %1)")
                       .arg(static_cast<int>(errorCode));
            break;

        case ResultMessageErrorCode::UnknownError:
            errorOutput = "unknown error";
            break;
        }

        setCommandExecutionFailed(3, "Command failed: " + errorOutput);
    }

    void CommandBase::listenerSlot()
    {
        if (_finishedOrFailed)
            return;

        bool canAdvance = _steps[_currentStep]();

        if (_finishedOrFailed)
            return;

        if (!canAdvance)
            return;

        _currentStep++;

        if (_currentStep >= _steps.size())
        {
            qWarning() << "Step number" << _currentStep
                       << "should be less than" << _steps.size();
            Q_EMIT executionFailed(3, "internal error");
            return;
        }

        // we advanced, so try the next step right now, don't wait for signals
        QTimer::singleShot(_stepDelayMilliseconds, this, &CommandBase::listenerSlot);
    }

}

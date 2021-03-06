/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "commandparser.h"

#include "commands.h"

#include <limits>

namespace PMP {

    CommandParser::CommandParser()
     : _command(nullptr),
       _authenticationMode(AuthenticationMode::Implicit)
    {
        //
    }

    void CommandParser::reset()
    {
        _command = nullptr;
        _authenticationMode = AuthenticationMode::Implicit;
        _errorMessage.clear();
        _username.clear();
    }

    template <class SomeCommand>
    void CommandParser::handleCommandNotRequiringArguments(
                                                         QVector<QString> commandWithArgs)
    {
        auto argumentsCount = commandWithArgs.size() - 1;

        if (argumentsCount > 0)
        {
            _errorMessage =
                    "Command '" + commandWithArgs[0] + "' does not accept arguments!";
            return;
        }

        _command = new SomeCommand();
    }

    void CommandParser::parse(QVector<QString> commandWithArgs)
    {
        reset();

        splitMultipleCommandsInOne(commandWithArgs);

        if (commandWithArgs[0] == "login")
        {
            parseExplicitLoginAndSeparator(commandWithArgs);
        }

        if (gotParseError()) return;

        parseCommand(commandWithArgs);
    }

    void CommandParser::splitMultipleCommandsInOne(QVector<QString>& commandWithArgs)
    {
        if (commandWithArgs.isEmpty())
            return;

        auto command = commandWithArgs[0];

        auto colonIndex = command.indexOf(':');
        if (colonIndex < 0)
            return;

        auto firstPart = command.left(colonIndex);
        if (firstPart.contains('"') || firstPart.contains('\''))
            return;

        auto remainingPart = command.mid(colonIndex + 1);
        QVector<QString> newCommandWithArgs;

        if (!firstPart.isEmpty())
            newCommandWithArgs.append(firstPart);

        newCommandWithArgs.append(":");

        if (!remainingPart.isEmpty())
            newCommandWithArgs.append(remainingPart);

        newCommandWithArgs.append(commandWithArgs.mid(1));

        commandWithArgs = newCommandWithArgs;
    }

    void CommandParser::parseExplicitLoginAndSeparator(QVector<QString>& commandWithArgs)
    {
        //auto command = commandWithArgs[0];
        /* we already know that the command is "login" */

        bool allCredentialsFromStdIn = false;

        int separatorIndex = -1;
        for (int argIndex = 1; argIndex < commandWithArgs.size(); ++argIndex)
        {
            auto arg = commandWithArgs[argIndex];
            if (arg == ":")
            {
                separatorIndex = argIndex;
                break;
            }

            switch (argIndex)
            {
                case 1:
                    if (arg == "-")
                        allCredentialsFromStdIn = true;
                    else
                        _username = arg;
                    break;
                case 2:
                    if (arg != "-")
                    {
                        _errorMessage =
                            "Password for \"login\" command must be specified as \"-\"";
                        return;
                    }
                    break;
                case 3:
                    _errorMessage =
                        "\"login\" command has too many arguments,"
                        " or a \":\" separator is missing";
                    return;
            }
        }

        if (separatorIndex < 0)
        {
            _errorMessage = "There must be a \":\" separator after the \"login command\"";
            return;
        }

        auto argCount = separatorIndex - 1;
        if (argCount == 0)
        {
            _authenticationMode = AuthenticationMode::ExplicitAllInteractive;
        }
        else if (argCount == 1)
        {
            _authenticationMode =
                allCredentialsFromStdIn ? AuthenticationMode::ExplicitAllFromStdIn
                                        : AuthenticationMode::ExplicitPasswordInteractive;
        }
        else if (argCount == 2)
        {
            _authenticationMode =
                allCredentialsFromStdIn ? AuthenticationMode::ExplicitAllFromStdIn
                                        : AuthenticationMode::ExplicitPasswordFromStdIn;
        }

        commandWithArgs = commandWithArgs.mid(separatorIndex + 1);
    }

    void CommandParser::parseCommand(QVector<QString> commandWithArgs)
    {
        if (commandWithArgs.isEmpty())
        {
            _errorMessage = "No command specified";
            return;
        }

        auto command = commandWithArgs[0];
        auto args = commandWithArgs.mid(1);
        auto argsCount = args.size();

        if (command == "play")
        {
            handleCommandNotRequiringArguments<PlayCommand>(commandWithArgs);
        }
        else if (command == "pause")
        {
            handleCommandNotRequiringArguments<PauseCommand>(commandWithArgs);
        }
        else if (command == "skip")
        {
            handleCommandNotRequiringArguments<SkipCommand>(commandWithArgs);
        }
        else if (command == "break")
        {
            handleCommandNotRequiringArguments<BreakCommand>(commandWithArgs);
        }
        else if (command == "nowplaying")
        {
            handleCommandNotRequiringArguments<NowPlayingCommand>(commandWithArgs);
        }
        else if (command == "queue")
        {
            handleCommandNotRequiringArguments<QueueCommand>(commandWithArgs);
        }
        else if (command == "shutdown")
        {
            if (argsCount == 0)
            {
                _command = new ShutdownCommand();
            }
            /*
            else if (argsCount == 1)
            {
                _command = new ShutdownCommand(args[0]);
            }
            */
            else
            {
                _errorMessage = "Command 'shutdown' requires zero arguments!";
            }
        }
        else if (command == "volume")
        {
            if (argsCount == 0)
            {
                _command = new GetVolumeCommand();
            }
            else if (argsCount > 1)
            {
                _errorMessage = "Command 'volume' cannot have more than one argument!";
            }
            else /* one argument */
            {
                bool ok;
                uint volume = args[0].toUInt(&ok);
                if (!ok || volume > 100)
                {
                    _errorMessage =
                        "Command 'volume' requires a volume argument in the range 0-100!";
                    return;
                }

                _command = new SetVolumeCommand(volume);
            }
        }
        else if (command == "qdel")
        {
            if (argsCount != 1)
            {
                _errorMessage = "Command 'qdel' requires one argument, a queue ID";
                return;
            }

            bool ok;
            uint queueId = args[0].toUInt(&ok);
            if (!ok || queueId > std::numeric_limits<quint32>::max())
            {
                _errorMessage =
                        "Command 'qdel' requires a valid queue ID as its first argument!";
                return;
            }

            _command = new QueueDeleteCommand(queueId);
        }
        else if (command == "qmove")
        {
            if (argsCount != 2)
            {
                _errorMessage = "Command 'qmove' requires two arguments!";
                return;
            }

            bool ok;
            uint queueId = args[0].toUInt(&ok);
            if (!ok || queueId > std::numeric_limits<quint32>::max())
            {
                _errorMessage =
                    "Command 'qmove' requires a valid queue ID as its first argument!";
                return;
            }

            if (!args[1].startsWith("+") && !args[1].startsWith("-"))
            {
                _errorMessage =
                    "Second argument of command 'qmove' must start with \"+\" or \"-\"!";
                return;
            }
            int moveDiff = args[1].toInt(&ok);
            if (!ok || moveDiff == 0)
            {
                _errorMessage =
                    "Second argument of command 'qmove' must be a positive or negative number!";
                return;
            }
            else if (moveDiff < std::numeric_limits<qint16>::min()
                     || moveDiff > std::numeric_limits<qint16>::max())
            {
                _errorMessage =
                    "Second argument of command 'qmove' must be in the range -32768 to +32767!";
                return;
            }

            _command = new QueueMoveCommand(queueId, moveDiff);
        }
        else if (command == ":")
        {
            _errorMessage = "Expected command before \":\" separator";
        }
        else
        {
            _errorMessage = QString("Command not recognized: \"%1\"").arg(command);

            if (command.contains(':'))
                _errorMessage +=
                        " (did you forget to put spaces around the \":\" separator?)";
        }
    }

}

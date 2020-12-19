/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP {

    CommandParser::CommandParser()
     : _command(nullptr)
    {
        //
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
        _command = nullptr;
        _errorMessage.clear();

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
        /*
        else if (command == "nowplaying")
        {
            handleCommandNotRequiringArguments<NowPlayingCommand>(commandWithArgs);
        }
        else if (command == "queue")
        {
            handleCommandNotRequiringArguments<QueueCommand>(commandWithArgs);
        }
        */
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
                return;
            }
        }
        else if (command == "volume")
        {
            if (argsCount == 0)
            {
                //_command = new GetVolumeCommand();
                _errorMessage = "Command 'volume' without arguments is currently broken!";
                return;
            }
            else if (argsCount > 1)
            {
                _errorMessage = "Command 'volume' cannot have more than one argument!";
                return;
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
        /*
        else if (command == "qmove")
        {
            if (argsCount != 2)
            {
                _errorMessage = "Command 'qmove' requires two arguments!";
                return;
            }

            bool ok;
            int queueId = args[0].toInt(&ok);
            if (!ok || queueId <= 0)
            {
                _errorMessage =
                        "Command 'qmove' requires a valid QID as its first argument!";
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

            _command = new QueueMoveCommand(queueId, moveDiff);
        }
        */
        else
        {
            _errorMessage = "Command not recognized: " + command;
            return;
        }
    }

}

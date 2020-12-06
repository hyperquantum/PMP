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

namespace PMP {

    CommandParser::CommandParser()
     : _waitForResponse(false)
    {
        //
    }

    void CommandParser::parse(QVector<QString> commandWithArgs)
    {
        _errorMessage.clear();
        _commandToSend.clear();
        _waitForResponse = false;

        auto command = commandWithArgs[0];
        auto args = commandWithArgs.mid(1);
        auto argsCount = args.size();

        if (command == "pause" || command == "play" || command == "skip"
            || command == "nowplaying" || command == "queue")
        {
            if (argsCount > 0)
            {
                _errorMessage = "Command '" + command + "' does not accept arguments!";
                return;
            }

            /* OK */
            _commandToSend = command;
            _waitForResponse = (command == "nowplaying" || command == "queue");
        }
        else if (command == "shutdown")
        {
            if (argsCount > 1)
            {
                _errorMessage = "Command 'shutdown' requires exactly one argument!";
                return;
            }
            else if (argsCount == 0)
            {
                _errorMessage = "Command 'shutdown' now requires the server password!";
                return;
            }

            /* OK */
            _commandToSend = command + " " + args[0];
            _waitForResponse = false;
        }
        else if (command == "volume")
        {
            if (argsCount > 1)
            {
                _errorMessage = "Command 'volume' cannot have more than one argument!";
                return;
            }
            else if (argsCount == 1)
            {
                bool ok;
                uint volume = args[0].toUInt(&ok);
                if (!ok || volume > 100)
                {
                    _errorMessage =
                        "Command 'volume' requires a volume argument in the range 0-100!";
                    return;
                }

                /* OK */
                _commandToSend = command + " " + args[0];
                _waitForResponse = true;
            }
            else
            { /* zero args */
                _commandToSend = command;
                _waitForResponse = true;
            }
        }
        else if (command == "qmove")
        {
            if (argsCount != 2)
            {
                _errorMessage = "Command 'qmove' requires two arguments!";
                return;
            }

            bool ok;
            uint queueID = args[0].toUInt(&ok);
            if (!ok || queueID == 0)
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

            /* OK */
            _commandToSend = command + " " + args[0] + " " + args[1];
            _waitForResponse = false;
        }
        else
        {
            _errorMessage = "Command not recognized: " + command;
            return;
        }
    }

}

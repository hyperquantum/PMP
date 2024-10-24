/*
    Copyright (C) 2014-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "administrativecommands.h"
#include "historycommands.h"
#include "miscellaneouscommands.h"
#include "playercommands.h"
#include "queuecommands.h"
#include "scrobblingcommands.h"

#include <limits>

namespace PMP
{
    /* ===== CommandArguments ===== */

    bool CommandParser::CommandArguments::currentIsOneOf(QVector<QString> options) const
    {
        return options.contains(current());
    }

    bool CommandParser::CommandArguments::tryParseInt(int& number) const
    {
        bool ok;
        number = current().toInt(&ok);
        return ok;
    }

    bool CommandParser::CommandArguments::tryParseTime(QTime& time) const
    {
        time = QTime::fromString(current(), "H:m");
        if (time.isValid())
            return true;

        time = QTime::fromString(current(), "H:m:s");
        return time.isValid();
    }

    bool CommandParser::CommandArguments::tryParseDate(QDate& date) const
    {
        date = QDate::fromString(current(), "yyyy-MM-dd");
        return date.isValid();
    }

    FileHash CommandParser::CommandArguments::tryParseTrackHash() const
    {
        return FileHash::tryParse(current());
    }

    /* ===== CommandParser ===== */

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

        if (command() == nullptr && !gotParseError())
        {
            /* this actually indicates a bug in the command parser */
            _errorMessage = "Command not understood (internal error)";
        }
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

        if (command == "status")
        {
            handleCommandNotRequiringArguments<StatusCommand>(commandWithArgs);
        }
        else if (command == "play")
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
        else if (command == "history")
        {
            handleCommandNotRequiringArguments<HistoryCommand>(commandWithArgs);
        }
        else if (command == "personalmode")
        {
            handleCommandNotRequiringArguments<PersonalModeCommand>(commandWithArgs);
        }
        else if (command == "publicmode")
        {
            handleCommandNotRequiringArguments<PublicModeCommand>(commandWithArgs);
        }
        else if (command == "dynamicmode")
        {
            parseDynamicModeCommand(args);
        }
        else if (command == "reloadserversettings")
        {
            handleCommandNotRequiringArguments<ReloadServerSettingsCommand>(
                                                                         commandWithArgs);
        }
        else if (command == "insert")
        {
            parseInsertCommand(args);
        }
        else if (command == "start")
        {
            parseStartCommand(args);
        }
        else if (command == "delayedstart")
        {
            parseDelayedStartCommand(args);
        }
        else if (command == "trackinfo")
        {
            parseTrackInfoCommand(args);
        }
        else if (command == "trackstats")
        {
            parseTrackStatsCommand(args);
        }
        else if (command == "trackhistory")
        {
            parseTrackHistoryCommand(args);
        }
        else if (command == "serverversion")
        {
            handleCommandNotRequiringArguments<ServerVersionCommand>(commandWithArgs);
        }
        else if (command == "scrobbling")
        {
            parseScrobblingCommand(args);
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
        else if (command == "login")
        {
            _errorMessage = "The 'login' command can only be used as the first command";
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

    void CommandParser::parseInsertCommand(CommandArguments arguments)
    {
        if (arguments.noCurrent())
        {
            _errorMessage = "Command 'insert' requires arguments!";
            return;
        }

        InsertCommandBuilder commandBuilder;

        if (arguments.current() == "break")
        {
            commandBuilder.setItem(SpecialQueueItemType::Break);
        }
        else if (arguments.current() == "barrier")
        {
            commandBuilder.setItem(SpecialQueueItemType::Barrier);
        }
        else
        {
            auto hash = arguments.tryParseTrackHash();
            if (hash.isNull())
            {
                _errorMessage =
                    "First argument of command 'insert' must be 'break' or 'barrier' or a hash!";
                return;
            }
            commandBuilder.setItem(hash);
        }

        if (arguments.currentIsLast())
        {
            _errorMessage = "Command 'insert' requires at least one more argument!";
            return;
        }

        arguments.advance();

        if (arguments.current() == "front")
        {
            commandBuilder.setPosition(QueueIndexType::Normal, 0);
        }
        else if (arguments.current() == "end")
        {
            commandBuilder.setPosition(QueueIndexType::Reverse, 0);
        }
        else if (arguments.current() == "index")
        {
            if (arguments.currentIsLast())
            {
                _errorMessage = "No actual index provided after 'index'!";
                return;
            }
            arguments.advance();

            int insertionIndex;
            if (!arguments.tryParseInt(insertionIndex) || insertionIndex < 0)
            {
                _errorMessage = "Index must be a non-negative number!";
                return;
            }
            commandBuilder.setPosition(QueueIndexType::Normal, insertionIndex);
        }
        else
        {
            _errorMessage = "Position indicator must be 'front', 'end' or 'index'!";
            return;
        }

        if (arguments.haveMore())
        {
            _errorMessage = "Command has too many arguments!";
            return;
        }

        _command = commandBuilder.buildCommand();
    }

    void CommandParser::parseStartCommand(CommandArguments arguments)
    {
        if (arguments.noCurrent())
        {
            _errorMessage = "Command 'start' requires arguments!";
            return;
        }

        if (arguments.current() == "indexation")
        {
            arguments.advance();
            parseStartIndexationCommand(arguments);
        }
        else
        {
            _errorMessage = "Expected 'indexation' after 'start'!";
        }
    }

    void CommandParser::parseStartIndexationCommand(CommandArguments& arguments)
    {
        if (arguments.noCurrent())
        {
            _command = new StartFullIndexationCommand();
            return;
        }

        if (arguments.current() == "full")
        {
            _command = new StartFullIndexationCommand();
        }
        else if (arguments.current() == "new")
        {
            _command = new StartQuickScanForNewFilesCommand();
        }
        else
        {
            _errorMessage = "Expected 'full', 'new', or no arguments after 'indexation'!";
        }
    }

    void CommandParser::parseDelayedStartCommand(CommandArguments arguments)
    {
        if (arguments.noCurrent())
        {
            _errorMessage = "Command 'delayedstart' requires arguments!";
            return;
        }

        if (arguments.currentIsOneOf({"abort", "cancel"}))
        {
            if (arguments.haveMore())
            {
                _errorMessage = "Command has too many arguments!";
                return;
            }

            _command = new DelayedStartCancelCommand();
        }
        else if (arguments.current() == "at")
        {
            arguments.advance();
            parseDelayedStartAt(arguments);
        }
        else if (arguments.current() == "wait")
        {
            arguments.advance();
            parseDelayedStartWait(arguments);
        }
        else
        {
            _errorMessage =
                "Expected 'abort' or 'cancel' or 'at' or 'wait' after 'delayedstart'!";
        }
    }

    void CommandParser::parseDelayedStartAt(CommandArguments& arguments)
    {
        if (arguments.noCurrent())
        {
            _errorMessage = "Expected more arguments after 'at'!";
            return;
        }

        bool dateSpecified;
        QDate date;
        if (arguments.tryParseDate(date))
        {
            dateSpecified = true;
            arguments.advance();
        }
        else
        {
            dateSpecified = false;
            date = QDate::currentDate();
        }

        QTime time;
        if (!arguments.tryParseTime(time))
        {
            if (dateSpecified)
                _errorMessage = "Expected time after date!";
            else
                _errorMessage = "Expected date or time after 'at'!";

            return;
        }

        if (arguments.haveMore())
        {
            _errorMessage = "Command has too many arguments!";
            return;
        }

        QDateTime dateTime(date, time);
        if (!isInFuture(dateTime))
        {
            _errorMessage = "Start time must be in the future!";
            return;
        }

        _command = new DelayedStartAtCommand(dateTime);
    }

    void CommandParser::parseDelayedStartWait(CommandArguments& arguments)
    {
        if (arguments.noCurrent())
        {
            _errorMessage = "Expected more arguments after 'wait'!";
            return;
        }

        int number;
        if (!arguments.tryParseInt(number))
        {
            _errorMessage = "Expected valid number after 'wait'!";
            return;
        }
        if (number <= 0 || number > 1000000)
        {
            _errorMessage = "Number after 'wait' must be in the range 1 - 1000000!";
            return;
        }

        if (arguments.currentIsLast())
        {
            _errorMessage = "Expected time unit after the number!";
            return;
        }

        arguments.advance();

        qint64 unitMilliseconds = -1;
        if (arguments.currentIsOneOf({"s", "seconds", "second"}))
            unitMilliseconds = 1000;
        else if (arguments.currentIsOneOf({"min", "minutes", "minute"}))
            unitMilliseconds = 60 * 1000;
        else if (arguments.currentIsOneOf({"h", "hours", "hour"}))
            unitMilliseconds = 60 * 60 * 1000;
        else if (arguments.currentIsOneOf({"ms", "milliseconds", "millisecond"}))
            unitMilliseconds = 1;
        else
        {
            _errorMessage = QString("Invalid time unit: '%1'").arg(arguments.current());
            return;
        }

        if (arguments.haveMore())
        {
            _errorMessage = "Command has too many arguments!";
            return;
        }

        _command = new DelayedStartWaitCommand(number * unitMilliseconds);
    }

    void CommandParser::parseTrackInfoCommand(CommandArguments arguments)
    {
        if (arguments.noCurrent() || arguments.haveMore())
        {
            _errorMessage = "Command 'trackinfo' requires exactly one argument!";
            return;
        }

        auto hash = arguments.tryParseTrackHash();
        if (hash.isNull())
        {
            _errorMessage = QString("Not a track hash: %1").arg(arguments.current());
            return;
        }

        _command = new TrackInfoCommand(hash);
    }

    void CommandParser::parseTrackStatsCommand(CommandArguments arguments)
    {
        if (arguments.noCurrent() || arguments.haveMore())
        {
            _errorMessage = "Command 'trackstats' requires exactly one argument!";
            return;
        }

        auto hash = arguments.tryParseTrackHash();
        if (hash.isNull())
        {
            _errorMessage = QString("Not a track hash: %1").arg(arguments.current());
            return;
        }

        _command = new TrackStatsCommand(hash);
    }

    void CommandParser::parseTrackHistoryCommand(CommandArguments arguments)
    {
        if (arguments.noCurrent() || arguments.haveMore())
        {
            _errorMessage = "Command 'trackhistory' requires exactly one argument!";
            return;
        }

        auto hash = arguments.tryParseTrackHash();
        if (hash.isNull())
        {
            _errorMessage = QString("Not a track hash: %1").arg(arguments.current());
            return;
        }

        _command = new TrackHistoryCommand(hash);
    }

    void CommandParser::parseScrobblingCommand(CommandArguments arguments)
    {
        if (arguments.noCurrent())
        {
            _errorMessage = "Command 'scrobbling' requires arguments!";
            return;
        }

        if (arguments.current() == "enable")
        {
            parseScrobblingEnableOrDisableCommand(arguments, true);
        }
        else if (arguments.current() == "disable")
        {
            parseScrobblingEnableOrDisableCommand(arguments, false);
        }
        else if (arguments.current() == "status")
        {
            parseScrobblingStatusCommand(arguments);
        }
        else if (arguments.current() == "authenticate")
        {
            parseScrobblingAuthenticateCommand(arguments);
        }
        else
        {
            _errorMessage =
                "Expected 'enable' or 'disable' or 'status' or 'authenticate' after 'scrobbling'!";
        }
    }

    void CommandParser::parseScrobblingEnableOrDisableCommand(CommandArguments& arguments,
                                                              bool enable)
    {
        // current is "enable" or "disable"
        arguments.advance();

        auto providerOrNull = parseScrobblingProviderName(arguments);
        if (providerOrNull == null)
            return;

        auto provider = providerOrNull.value();

        if (arguments.haveMore())
        {
            _errorMessage = "Command has too many arguments!";
            return;
        }

        _command = new ScrobblingActivationCommand(provider, enable);
    }

    void CommandParser::parseScrobblingStatusCommand(CommandArguments& arguments)
    {
        // current is "status"
        arguments.advance();

        auto providerOrNull = parseScrobblingProviderName(arguments);
        if (providerOrNull == null)
            return;

        auto provider = providerOrNull.value();

        if (arguments.haveMore())
        {
            _errorMessage = "Command has too many arguments!";
            return;
        }

        _command = new ScrobblingStatusCommand(provider);
    }

    void CommandParser::parseScrobblingAuthenticateCommand(CommandArguments& arguments)
    {
        // current is "authenticate"
        arguments.advance();

        auto providerOrNull = parseScrobblingProviderName(arguments);
        if (providerOrNull == null)
            return;

        auto provider = providerOrNull.value();

        if (arguments.haveMore())
        {
            _errorMessage = "Command has too many arguments!";
            return;
        }

        _command = new ScrobblingAuthenticateCommand(provider);
    }

    Nullable<ScrobblingProvider> CommandParser::parseScrobblingProviderName(
                                                              CommandArguments& arguments)
    {
        if (arguments.haveCurrent())
        {
            if (arguments.currentIsOneOf({"lastfm", "last.fm"}))
            {
                return ScrobblingProvider::LastFm;
            }
        }

        if (arguments.noCurrent())
        {
            _errorMessage =
                QString("Expected 'lastfm' or 'last.fm' after '%1'")
                    .arg(arguments.previous());
        }
        else
        {
            _errorMessage =
                QString("Expected 'lastfm' or 'last.fm' instead of '%1'")
                    .arg(arguments.current());
        }

        return null;
    }

    void CommandParser::parseDynamicModeCommand(CommandArguments arguments)
    {
        if (arguments.noCurrent())
        {
            _errorMessage = "Command 'dynamicmode' requires at least one argument!";
            return;
        }

        if (arguments.currentIsOneOf({"on", "off"}))
        {
            bool isOn = arguments.current() == "on";
            arguments.advance();
            parseDynamicModeOnOrOff(arguments, isOn);
        }
        else
        {
            _errorMessage = "Expected 'on' or 'off' after 'dynamicmode'!";
        }
    }

    void CommandParser::parseDynamicModeOnOrOff(CommandArguments& arguments, bool isOn)
    {
        if (arguments.haveCurrent())
        {
            _errorMessage = "Command has too many arguments!";
            return;
        }

        _command = new DynamicModeActivationCommand(isOn);
    }

    bool CommandParser::isInFuture(QDateTime time)
    {
        return QDateTime::currentDateTimeUtc() < time.toUTC();
    }
}

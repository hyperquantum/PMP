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

#include "test_commandparser.h"

#include "cmd-remote/administrativecommands.h"
#include "cmd-remote/command.h"
#include "cmd-remote/commands.h"
#include "cmd-remote/commandparser.h"
#include "cmd-remote/queuecommands.h"

#include <QtTest/QTest>

using namespace PMP;

template<class CommandType>
void verifySuccessfulParsingOf(QVector<QString> commandWithArgs)
{
    CommandParser parser;
    parser.parse(commandWithArgs);

    QVERIFY(parser.parsedSuccessfully());
    QCOMPARE(parser.errorMessage(), "");

    auto* commandObject = dynamic_cast<CommandType*>(parser.command());
    QVERIFY(commandObject != nullptr);
}

void verifyParseError(QVector<QString> commandWithArgs)
{
    CommandParser parser;
    parser.parse(commandWithArgs);

    QVERIFY(!parser.parsedSuccessfully());
    QVERIFY(parser.errorMessage().length() > 0);
    QVERIFY(parser.command() == nullptr);
}

void TestCommandParser::statusCommandCanBeParsed()
{
    verifySuccessfulParsingOf<StatusCommand>({"status"});
}

void TestCommandParser::statusCommandDoesNotAcceptArguments()
{
    verifyParseError({"status", "xyz"});
}

void TestCommandParser::playCommandCanBeParsed()
{
    verifySuccessfulParsingOf<PlayCommand>({"play"});
}

void TestCommandParser::playCommandDoesNotAcceptArguments()
{
    verifyParseError({"play", "xyz"});
}

void TestCommandParser::pauseCommandCanBeParsed()
{
    verifySuccessfulParsingOf<PauseCommand>({"pause"});
}

void TestCommandParser::pauseCommandDoesNotAcceptArguments()
{
    verifyParseError({"pause", "xyz"});
}

void TestCommandParser::skipCommandCanBeParsed()
{
    verifySuccessfulParsingOf<SkipCommand>({"skip"});
}

void TestCommandParser::skipCommandDoesNotAcceptArguments()
{
    verifyParseError({"skip", "xyz"});
}

void TestCommandParser::breakCommandCanBeParsed()
{
    verifySuccessfulParsingOf<BreakCommand>({"break"});
}

void TestCommandParser::breakCommandDoesNotAcceptArguments()
{
    verifyParseError({"break", "xyz"});
}

void TestCommandParser::nowplayingCommandCanBeParsed()
{
    verifySuccessfulParsingOf<NowPlayingCommand>({"nowplaying"});
}

void TestCommandParser::nowplayingCommandDoesNotAcceptArguments()
{
    verifyParseError({"nowplaying", "xyz"});
}

void TestCommandParser::queueCommandCanBeParsed()
{
    verifySuccessfulParsingOf<QueueCommand>({"queue"});
}

void TestCommandParser::queueCommandDoesNotAcceptArguments()
{
    verifyParseError({"queue", "xyz"});
}

void TestCommandParser::personalmodeCommandCanBeParsed()
{
    verifySuccessfulParsingOf<PersonalModeCommand>({"personalmode"});
}

void TestCommandParser::personalmodeCommandDoesNotAcceptArguments()
{
    verifyParseError({"personalmode", "xyz"});
}

void TestCommandParser::publicmodeCommandCanBeParsed()
{
    verifySuccessfulParsingOf<PublicModeCommand>({"publicmode"});
}

void TestCommandParser::publicmodeCommandDoesNotAcceptArguments()
{
    verifyParseError({"publicmode", "xyz"});
}

void TestCommandParser::dynamicmodeCommandTestValid()
{
    verifySuccessfulParsingOf<DynamicModeActivationCommand>({"dynamicmode", "on"});
    verifySuccessfulParsingOf<DynamicModeActivationCommand>({"dynamicmode", "off"});
}

void TestCommandParser::dynamicmodeCommandTestInvalid()
{
    verifyParseError({"dynamicmode"});

    verifyParseError({"dynamicmode", "abcd"});
    verifyParseError({"dynamicmode", "on", "on"});
    verifyParseError({"dynamicmode", "off", "off"});
}

void TestCommandParser::reloadserversettingsCommandCanBeParsed()
{
    verifySuccessfulParsingOf<ReloadServerSettingsCommand>({"reloadserversettings"});
}

void TestCommandParser::reloadserversettingsCommandDoesNotAcceptArguments()
{
    verifyParseError({"reloadserversettings", "xyz"});
}

void TestCommandParser::shutdownCommandCanBeParsed()
{
    verifySuccessfulParsingOf<ShutdownCommand>({"shutdown"});
}

void TestCommandParser::shutdownCommandDoesNotAcceptArguments()
{
    verifyParseError({"shutdown", "xyz"});
}

void TestCommandParser::insertCommandTestValid()
{
    const auto hash =
        "12345-abcdef123456abcdef123456abcdef1234567890-abcdef123456abcdef123456abcdef00";

    verifySuccessfulParsingOf<QueueInsertSpecialItemCommand>({"insert","break","front"});
    verifySuccessfulParsingOf<QueueInsertSpecialItemCommand>({"insert","barrier","front"});
    verifySuccessfulParsingOf<QueueInsertTrackCommand>({"insert",hash,"front"});

    verifySuccessfulParsingOf<QueueInsertSpecialItemCommand>({"insert","break","end"});
    verifySuccessfulParsingOf<QueueInsertSpecialItemCommand>({"insert","barrier","end"});
    verifySuccessfulParsingOf<QueueInsertTrackCommand>({"insert",hash,"end"});

    verifySuccessfulParsingOf<QueueInsertSpecialItemCommand>({"insert","break","index","0"});
    verifySuccessfulParsingOf<QueueInsertSpecialItemCommand>({"insert","break","index","12"});
    verifySuccessfulParsingOf<QueueInsertSpecialItemCommand>({"insert","barrier","index","0"});
    verifySuccessfulParsingOf<QueueInsertSpecialItemCommand>({"insert","barrier","index","12"});
    verifySuccessfulParsingOf<QueueInsertTrackCommand>({"insert",hash,"index","0"});
    verifySuccessfulParsingOf<QueueInsertTrackCommand>({"insert",hash,"index","12"});
}

void TestCommandParser::insertCommandTestInvalid()
{
    const auto hash =
        "12345-abcdef123456abcdef123456abcdef1234567890-abcdef123456abcdef123456abcdef00";

    verifyParseError({"insert"});

    verifyParseError({"insert", "xyz"});
    verifyParseError({"insert", "break"});
    verifyParseError({"insert", "barrier"});
    verifyParseError({"insert", hash});

    verifyParseError({"insert", "xyz", "front"});
    verifyParseError({"insert", "xyz", "end"});
    verifyParseError({"insert", "xyz", "index"});
    verifyParseError({"insert", "xyz", "index", "3"});

    verifyParseError({"insert", "break", "xyz"});
    verifyParseError({"insert", "barrier", "xyz"});
    verifyParseError({"insert", hash, "xyz"});

    verifyParseError({"insert", "break", "front", "xyz"});
    verifyParseError({"insert", "barrier", "end", "xyz"});
    verifyParseError({"insert", hash, "end", "xyz"});

    verifyParseError({"insert", "break", "index"});
    verifyParseError({"insert", "barrier", "index"});
    verifyParseError({"insert", hash, "index"});

    verifyParseError({"insert", "break", "index", "xyz"});
    verifyParseError({"insert", "barrier", "index", "xyz"});
    verifyParseError({"insert", hash, "index", "xyz"});

    verifyParseError({"insert", "break", "index", "-2"});
    verifyParseError({"insert", "barrier", "index", "-2"});
    verifyParseError({"insert", hash, "index", "-2"});

    verifyParseError({"insert", "break", "index", "3", "xyz"});
    verifyParseError({"insert", "barrier", "index", "3", "xyz"});
    verifyParseError({"insert", hash, "index", "3", "xyz"});
}

QTEST_MAIN(TestCommandParser)

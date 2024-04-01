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

#ifndef PMP_COMMANDPARSER_H
#define PMP_COMMANDPARSER_H

#include "common/filehash.h"
#include "common/nullable.h"
#include "common/scrobblingprovider.h"

#include <QDate>
#include <QString>
#include <QTime>
#include <QVector>

namespace PMP
{
    class Command;

    enum class AuthenticationMode
    {
        Implicit,
        ExplicitAllInteractive,
        ExplicitPasswordInteractive,
        ExplicitPasswordFromStdIn,
        ExplicitAllFromStdIn,
    };

    class CommandParser
    {
    public:
        CommandParser();

        void parse(QVector<QString> commandWithArgs);

        Command* command() const { return _command; }
        AuthenticationMode authenticationMode() const { return _authenticationMode; }
        QString explicitLoginUsername() const { return _username; }

        bool parsedSuccessfully() const { return _command != nullptr; }
        QString errorMessage() const { return _errorMessage; }

    private:
        class CommandArguments
        {
        public:
            CommandArguments(QVector<QString> arguments)
             : _arguments(arguments),
               _currentIndex(0)
            {
                //
            }

            void advance() { _currentIndex++; }

            int remainingCount() const
            {
                if (_currentIndex >= _arguments.size())
                    return 0;

                return _arguments.size() - 1 - _currentIndex;
            }

            bool haveCurrent() const { return _currentIndex < _arguments.size(); }
            bool noCurrent() const { return !haveCurrent(); }

            bool haveMore() const { return remainingCount() > 0; }
            bool currentIsLast() const { return !haveMore(); }

            QString current() const
            {
                if (_currentIndex >= _arguments.size())
                    return {};

                return _arguments[_currentIndex];
            }

            bool currentIsOneOf(QVector<QString> options) const;

            QString previous() const
            {
                if (_currentIndex <= 0)
                    return {};

                return _arguments[_currentIndex - 1];
            }

            bool tryParseInt(int& number) const;
            bool tryParseTime(QTime& time) const;
            bool tryParseDate(QDate& date) const;
            FileHash tryParseTrackHash() const;

            static QByteArray tryDecodeHexWithExpectedLength(QString const& text,
                                                             int expectedLength);
            static bool isHexEncoded(QByteArray const& bytes);

        private:
            QVector<QString> _arguments;
            int _currentIndex;
        };

        void reset();

        bool gotParseError() const { return !_errorMessage.isEmpty(); }

        void splitMultipleCommandsInOne(QVector<QString>& commandWithArgs);
        void parseExplicitLoginAndSeparator(QVector<QString>& commandWithArgs);
        void parseCommand(QVector<QString> commandWithArgs);
        void parseInsertCommand(CommandArguments arguments);
        void parseStartCommand(CommandArguments arguments);
        void parseStartIndexationCommand(CommandArguments& arguments);
        void parseDelayedStartCommand(CommandArguments arguments);
        void parseDelayedStartAt(CommandArguments& arguments);
        void parseDelayedStartWait(CommandArguments& arguments);
        void parseTrackInfoCommand(CommandArguments arguments);
        void parseTrackStatsCommand(CommandArguments arguments);
        void parseTrackHistoryCommand(CommandArguments arguments);
        void parseScrobblingCommand(CommandArguments arguments);
        void parseScrobblingEnableOrDisableCommand(CommandArguments& arguments,
                                                   bool enable);
        void parseScrobblingStatusCommand(CommandArguments& arguments);
        void parseScrobblingAuthenticateCommand(CommandArguments& arguments);
        Nullable<ScrobblingProvider> parseScrobblingProviderName(
                                                             CommandArguments& arguments);
        void parseDynamicModeCommand(CommandArguments arguments);
        void parseDynamicModeOnOrOff(CommandArguments& arguments, bool isOn);

        template <class SomeCommand>
        void handleCommandNotRequiringArguments(QVector<QString> commandWithArgs);

        static bool isInFuture(QDateTime time);

        Command* _command;
        QString _errorMessage;
        QString _username;
        AuthenticationMode _authenticationMode;
    };
}
#endif

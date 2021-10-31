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

#ifndef PMP_COMMANDPARSER_H
#define PMP_COMMANDPARSER_H

#include <QString>
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
        void reset();

        bool gotParseError() const { return !_errorMessage.isEmpty(); }

        void splitMultipleCommandsInOne(QVector<QString>& commandWithArgs);
        void parseExplicitLoginAndSeparator(QVector<QString>& commandWithArgs);
        void parseCommand(QVector<QString> commandWithArgs);

        template <class SomeCommand>
        void handleCommandNotRequiringArguments(QVector<QString> commandWithArgs);

        Command* _command;
        QString _errorMessage;
        QString _username;
        AuthenticationMode _authenticationMode;
    };
}
#endif

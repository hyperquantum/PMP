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

#ifndef PMP_TESTCOMMANDPARSER_H
#define PMP_TESTCOMMANDPARSER_H

#include <QObject>

namespace PMP
{
    class CommandParser;
}

class TestCommandParser : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void statusCommandCanBeParsed();
    void statusCommandDoesNotAcceptArguments();

    void playCommandCanBeParsed();
    void playCommandDoesNotAcceptArguments();

    void pauseCommandCanBeParsed();
    void pauseCommandDoesNotAcceptArguments();

    void skipCommandCanBeParsed();
    void skipCommandDoesNotAcceptArguments();

    void breakCommandCanBeParsed();
    void breakCommandDoesNotAcceptArguments();

    void nowplayingCommandCanBeParsed();
    void nowplayingCommandDoesNotAcceptArguments();

    void queueCommandCanBeParsed();
    void queueCommandDoesNotAcceptArguments();

    void personalmodeCommandCanBeParsed();
    void personalmodeCommandDoesNotAcceptArguments();

    void publicmodeCommandCanBeParsed();
    void publicmodeCommandDoesNotAcceptArguments();

    void dynamicmodeCommandTestValid();
    void dynamicmodeCommandTestInvalid();

    void reloadserversettingsCommandCanBeParsed();
    void reloadserversettingsCommandDoesNotAcceptArguments();

    void shutdownCommandCanBeParsed();
    void shutdownCommandDoesNotAcceptArguments();

    void insertCommandTestValid();
    void insertCommandTestInvalid();
};
#endif

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

#include "console.h"

#include <QTextStream>

#ifdef Q_OS_WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace PMP
{

    QString Console::promptForPassword(QString prompt)
    {
        QTextStream out(stdout);
        QTextStream in(stdin);

        out << prompt;
        out.flush();

        enableConsoleEcho(false);
        QString password = in.readLine();
        enableConsoleEcho(true);

        /* The newline that was typed by the user was not printed because we turned off
           echo to the console, so we have to write a newline ourselves now */
        out << endl;

        return password;
    }

    QString Console::prompt(QString prompt)
    {
        QTextStream out(stdout);
        QTextStream in(stdin);

        out << prompt;
        out.flush();

        QString input = in.readLine();

        return input;
    }

    QVector<QString> Console::readLinesFromStdIn(int lineCount)
    {
        if (lineCount < 1)
            return {};

        QTextStream in(stdin);

        QVector<QString> lines;
        lines.reserve(lineCount);

        for (int i = 0; i < lineCount; ++i)
        {
            QString line = in.readLine();
            if (line.isNull())
                return lines; /* result is incomplete */

            lines << line;
        }

        return lines;
    }

    void Console::enableConsoleEcho(bool enable)
    {
#ifdef Q_OS_WIN32

        HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode;

        GetConsoleMode(stdinHandle, &mode);

        if (!enable)
        {
            mode &= ~ENABLE_ECHO_INPUT;
        }
        else
        {
            mode |= ENABLE_ECHO_INPUT;
        }

        SetConsoleMode(stdinHandle, mode);

#else

        struct termios tty;

        tcgetattr(STDIN_FILENO, &tty);

        if (!enable)
        {
            tty.c_lflag &= ~ECHO;
        }
        else
        {
            tty.c_lflag |= ECHO;
        }

        (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);

#endif
    }

}

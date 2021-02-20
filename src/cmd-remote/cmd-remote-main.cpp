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

#include "common/logging.h"
#include "common/version.h"

#include "client.h"
#include "command.h"
#include "commandparser.h"

#include <QByteArray>
#include <QtCore>
#include <QTcpSocket>

#ifdef Q_OS_WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace PMP;

void enableConsoleEcho(bool enable)
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

QString promptForPassword(QString prompt)
{
    QTextStream out(stdout);
    QTextStream in(stdin);

    out << prompt;
    out.flush();

    enableConsoleEcho(false);
    QString password = in.readLine();
    enableConsoleEcho(true);

    /* The newline that was typed by the user was not printed because we turned off echo
       to the console, so we have to write a newline ourselves now */
    out << endl;

    return password;
}

QString prompt(QString prompt)
{
    QTextStream out(stdout);
    QTextStream in(stdin);

    out << prompt;
    out.flush();

    QString input = in.readLine();

    return input;
}

void printUsage(QTextStream& out, QString const& programName)
{
    out << "usage: " << programName
              << " <server-name-or-ip> [<server-port>] <command> [<command args>]" << endl
        << endl
        << "  commands:" << endl
        << endl
        << "    play: start/resume playback" << endl
        << "    pause: pause playback" << endl
        << "    skip: jump to next track in the queue" << endl
        << "    volume: get current volume percentage (0-100)" << endl
        << "    volume <number>: set volume percentage (0-100)" << endl
        << "    nowplaying: get info about the track currently playing" << endl
        << "    queue: get queue length and the first tracks waiting in the queue" << endl
        << "    break: insert a break at the front of the queue if not present there yet" << endl
        << "    qdel <QID>: delete an entry from the queue" << endl
        << "    qmove <QID> <-diff>: move a track up in the queue (e.g. -3)" << endl
        << "    qmove <QID> <+diff>: move a track down in the queue (eg. +2)" << endl
        << "    shutdown: shut down the server program" << endl
        << endl
        << "  NOTICE:" << endl
        << "    The 'shutdown' command no longer supports arguments." << endl
        << endl
        << "  Authentication:" << endl
        << "    All commands that have side-effects require authentication. They will" << endl
        << "    prompt for username and password in the console. One exception to this" << endl
        << "    principle is the 'queue' command; it requires authentication although it" << endl
        << "    has no side-effects. This may change in the future." << endl
        << "    It used to be possible to run the 'shutdown' command with the " << endl
        << "    server password as its argument and without logging in as a PMP user," << endl
        << "    but that is no longer possible. Support for this could be added again" << endl
        << "    in the future, but that would not be compatible with older PMP servers." << endl
        << endl
        << "  Server Password:" << endl
        << "    This is a global password for the server, printed to stdout at" << endl
        << "    server startup. It is no longer relevant for the PMP command-line client." << endl
        << endl
        << "  Examples:" << endl
        << "    " << programName << " localhost queue" << endl
        << "    " << programName << " ::1 volume 100" << endl
        << "    " << programName << " 127.0.0.1 play" << endl
        << "    " << programName << " localhost qmove 42 +3" << endl
        ;
}

bool looksLikePortNumber(QString const& string)
{
    return string.size() > 0 && string[0].isDigit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Remote");
    QCoreApplication::setApplicationVersion(PMP_VERSION_DISPLAY);
    QCoreApplication::setOrganizationName(PMP_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(PMP_ORGANIZATION_DOMAIN);

    /* make sure that log messages do not go to stdout/stderr */
    Logging::enableTextFileOnlyLogging();
    Logging::setFilenameTag("CR"); /* CR = CMD-Remote */

    QTextStream out(stdout);
    QTextStream err(stderr);

    QStringList args = QCoreApplication::arguments();

    if (args.size() < 3)
    {
        err << "Not enough arguments specified!" << endl;
        printUsage(err, QFileInfo(QCoreApplication::applicationFilePath()).fileName());
        return 1;
    }

    QVector<QString> commandWithArgs;

    QString server = args[1];

    quint16 portNumber;
    QString maybePortArg = args[2];
    if (looksLikePortNumber(maybePortArg))
    {
        /* Validate port number */
        bool ok = true;
        uint number = maybePortArg.toUInt(&ok);
        if (!ok || number > 0xFFFFu)
        {
            err << "Invalid port number: " << maybePortArg << endl;
            return 1;
        }

        portNumber = number;
        commandWithArgs = args.mid(3).toVector();
    }
    else
    {
        portNumber = 23432;
        commandWithArgs = args.mid(2).toVector();
    }

    if (commandWithArgs.size() == 0)
    {
        err << "Not enough arguments specified!" << endl;
        printUsage(err, QFileInfo(QCoreApplication::applicationFilePath()).fileName());
        return 1;
    }

    CommandParser commandParser;
    commandParser.parse(commandWithArgs);

    if (!commandParser.parsedSuccessfully())
    {
        err << commandParser.errorMessage() << endl;
        return 1;
    }

    Command* command = commandParser.command();

    QString username;
    QString password;
    if (command->requiresAuthentication())
    {
        username = prompt("PMP username: ");
        password = promptForPassword("password: ");
    }

    Client client(nullptr, &out, &err, server, portNumber, username, password, command);
    QObject::connect(
        &client, &Client::exitClient,
        &app, &QCoreApplication::exit
    );

    client.start();

    return app.exec();
}

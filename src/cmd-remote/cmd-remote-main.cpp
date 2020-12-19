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
                << " <server-name-or-ip> <server-port> <command> [<command args>]" << endl
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
        << "    qmove <QID> <-diff>: move a track up in the queue (e.g. -3)" << endl
        << "    qmove <QID> <+diff>: move a track down in the queue (eg. +2)" << endl
        << "    shutdown <server password>: shutdown the server program" << endl
        << endl
        << "  NOTICE: most commands are broken because the command line remote" << endl
        << "    does not support user authentication yet, and all commands having" << endl
        << "    side-effects are ignored by the server as long as no successful" << endl
        << "    authentication has taken place. The only exception is the new" << endl
        << "    'shutdown' command because it requires the server password." << endl
        << endl
        << "  Server Password:" << endl
        << "    This is a global password for the server, printed to stdout at" << endl
        << "    server startup."
        << endl;
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

    if (args.size() < 4)
    {
        err << "Not enough arguments specified!" << endl;
        printUsage(err, QFileInfo(QCoreApplication::applicationFilePath()).fileName());
        return 1;
    }

    QString server = args[1];
    QString port = args[2];
    auto commandWithArgs = args.mid(3).toVector();

    /* Validate port number */
    bool ok = true;
    uint portNumber = port.toUInt(&ok);
    if (!ok || portNumber > 0xFFFFu)
    {
        err << "Invalid port number: " << port << endl;
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

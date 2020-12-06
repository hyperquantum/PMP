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

#include "command.h"
#include "commandparser.h"

#include "common/version.h"

#include <QByteArray>
#include <QtCore>
#include <QTcpSocket>

using namespace PMP;

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

    QTcpSocket socket;
    socket.connectToHost(server, static_cast<quint16>(portNumber));

    if (!socket.waitForConnected(2000))
    {
        err << "Failed to connect to the server: code " << socket.error() << endl;
        return 2;
    }

    while (socket.bytesAvailable() < 3)
    {
        if (!socket.waitForReadyRead(2000))
        {
            err << "No timely response from the server!" << endl;
            return 2;
        }
    }

    QByteArray dataReceived;
    dataReceived.append(socket.readAll());

    if (dataReceived.at(0) != 'P'
        || dataReceived.at(1) != 'M'
        || dataReceived.at(2) != 'P')
    {
        err << "This is not a PMP server!" << endl;
        return 2;
    }

    int semicolonIndex = -1;
    while ((semicolonIndex = dataReceived.indexOf(';')) < 0)
    {
        if (!socket.waitForReadyRead(2000))
        {
            err << "Server handshake not complete!" << endl;
            return 2;
        }

        dataReceived.append(socket.readAll());
    }

    QString serverHelloString = QString::fromUtf8(dataReceived.data(), semicolonIndex);
    dataReceived.remove(0, semicolonIndex + 1); /* +1 to remove the semicolon too */

    out << " server greeting: " << serverHelloString << endl;
    out << " sending command: " << command->commandStringToSend() << endl;

    socket.write((command->commandStringToSend() + ";").toUtf8());

    if (!socket.waitForBytesWritten(5000))
    {
        err << "Failed to send data to the server." << endl;
        return 2;
    }

    if (command->mustWaitForResponseAfterSending())
    {
        semicolonIndex = -1;
        while ((semicolonIndex = dataReceived.indexOf(';')) < 0)
        {
            if (!socket.waitForReadyRead(2000))
            {
                err << "Server sent incomplete response!" << endl;
                return 2;
            }

            dataReceived.append(socket.readAll());
        }

        QString response = QString::fromUtf8(dataReceived.data(), semicolonIndex);
        out << " server response: " << response << endl;
    }
    else
    {
        /* Wait a little bit before closing the socket. This allows the server to clean up
           the connection properly after we exit. */
        socket.waitForReadyRead(100);
    }

    return 0;
}

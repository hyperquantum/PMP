/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#include <QByteArray>
#include <QtCore>
#include <QTcpSocket>

void printUsage(QTextStream& out) {
    out << "usage: Remote <servername> <server-port> <command>" << endl
        << endl
        << "  commands: play,pause,skip,shutdown" << endl;
}

int main(int argc, char *argv[]) {

    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Remote");
    QCoreApplication::setApplicationVersion("0.0.0.1");
    QCoreApplication::setOrganizationName("Party Music Player");
    QCoreApplication::setOrganizationDomain("hyperquantum.be");

    QTextStream out(stdout);

    QStringList args = QCoreApplication::arguments();

    if (args.size() < 4) {
        out << "Not enough arguments specified!" << endl;
        printUsage(out);
        return 1;
    }

    QString server = args[1];
    QString port = args[2];
    QString command = args[3];

    bool ok = true;
    uint portNumber = port.toUInt(&ok);
    if (!ok)  {
        out << "Invalid port number: " << port << endl;
        return 1;
    }

    if (command == "pause" || command == "play" || command == "skip"
        || command == "shutdown")
    {
        // OK
    }
    else {
        out << "Command not recognized: " << command << endl;
        return 1;
    }

    QTcpSocket socket;
    socket.connectToHost(server, (quint16)portNumber);

    if (!socket.waitForConnected(2000)) {
        out << "Failed to connect to the server: code " << socket.error() << endl;
        return 2;
    }

    while (socket.bytesAvailable() < 3) {
        if (!socket.waitForReadyRead(2000)) {
            out << "No timely response from the server!" << endl;
            return 2;
        }
    }

    QByteArray dataReceived;
    dataReceived.append(socket.readAll());

    if (dataReceived.at(0) != 'P'
        || dataReceived.at(1) != 'M'
        || dataReceived.at(2) != 'P')
    {
        out << "This is not a PMP server!" << endl;
        return 2;
    }

    int semicolonIndex = -1;
    while ((semicolonIndex = dataReceived.indexOf(';')) < 0) {
        if (!socket.waitForReadyRead(2000)) {
            out << "Server handshake not complete!" << endl;
            return 2;
        }

        dataReceived.append(socket.readAll());
    }

    QString serverHelloString = QString::fromUtf8(dataReceived.data(), semicolonIndex);
    out << " server greeting: " << serverHelloString << endl;
    out << " sending command: " << command << endl;

    socket.write((command + ";").toUtf8());

    if (!socket.waitForBytesWritten(5000)) {
        out << "Failed to send data to the server." << endl;
        return 2;
    }




    return 0;
}

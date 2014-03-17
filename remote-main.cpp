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

#include <QtCore>
#include <QTcpSocket>

int main(int argc, char *argv[]) {

    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Remote");
    QCoreApplication::setApplicationVersion("0.0.0.1");
    QCoreApplication::setOrganizationName("Party Music Player");
    QCoreApplication::setOrganizationDomain("hyperquantum.be");

    QTextStream out(stdout);

    QStringList args = QCoreApplication::arguments();

    if (args.size() < 4) {
        out << "Not enough arguments specified!" << endl
            << "usage: Remote <servername> <server-port> <command>" << endl;
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

    if (command == "pause" || command == "play" || command == "shutdown") {
        // OK
    }
    else {
        out << "Command not recognized: " << command << endl;
        return 1;
    }

    QTcpSocket socket;
    socket.connectToHost(server, (quint16)portNumber);

    if (!socket.waitForConnected(1000)) {
        out << "Failed to connect to the server: code " << socket.error() << endl;
        return 2;
    }

    socket.write((command + ";").toUtf8());

    if (!socket.waitForBytesWritten(5000)) {
        out << "Failed to send data to the server." << endl;
        return 2;
    }




    return 0;
}

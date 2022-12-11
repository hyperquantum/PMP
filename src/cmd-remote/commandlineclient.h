/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COMMANDLINECLIENT_H
#define PMP_COMMANDLINECLIENT_H

#include "common/userloginerror.h"

#include <QObject>
#include <QPointer>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace PMP
{
    class ClientServerInterface;
    class Command;
    class ServerConnection;

    class CommandlineClient : public QObject
    {
        Q_OBJECT
    public:
        CommandlineClient(QObject* parent, QTextStream* out, QTextStream* err,
               QString server, quint16 port, QString username, QString password,
               Command* command);

    public Q_SLOTS:
        void start();

    Q_SIGNALS:
        void exitClient(int exitCode);

    private Q_SLOTS:
        void connected();
        void executeCommand();

    private:
        static QString toString(UserLoginError error);

        QTextStream* _out;
        QTextStream* _err;
        QString _server;
        quint16 _port;
        QString _username;
        QString _password;
        QPointer<ServerConnection> _serverConnection;
        QPointer<ClientServerInterface> _clientServerInterface;
        QPointer<Command> _command;
        bool _expectingDisconnect;
    };
}
#endif

/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "client.h"

#include "common/clientserverinterface.h"
#include "common/serverconnection.h"

#include "command.h"

#include <QTimer>

namespace PMP {

    Client::Client(QObject* parent, QTextStream* out, QTextStream* err,
                   QString server, quint16 port, QString username, QString password,
                   Command* command)
     : QObject(parent),
       _out(out),
       _err(err),
       _server(server),
       _port(port),
       _username(username),
       _password(password),
       _serverConnection(nullptr),
       _clientServerInterface(nullptr),
       _command(command)
    {
        // TODO : don't subscribe to _all_ events anymore
        _serverConnection =
                new ServerConnection(this, ServerEventSubscription::AllEvents);
        _clientServerInterface = new ClientServerInterface(_serverConnection);

        connect(
            _serverConnection, &ServerConnection::connected,
            this, &Client::connected
        );
        connect(
            _serverConnection, &ServerConnection::cannotConnect,
            this,
            [this](QAbstractSocket::SocketError error)
            {
                *_err << "Failed to connect to the server: " << error << endl;
                Q_EMIT exitClient(2);
            }
        );
        connect(
            _serverConnection, &ServerConnection::invalidServer,
            this,
            [this]()
            {
                *_err << "Server does not appear to be a PMP server!" << endl;
                Q_EMIT exitClient(2);
            }
        );
        connect(
            _serverConnection, &ServerConnection::connectionBroken,
            this,
            [this]()
            {
                // TODO : only if this happens before we have finished
                *_err << "Lost connection to the server unexpectedly!" << endl;
                Q_EMIT exitClient(2);
            }
        );

        connect(
            _serverConnection, &ServerConnection::userLoggedInSuccessfully,
            this, &Client::executeCommand
        );
        connect(
            _serverConnection, &ServerConnection::userLoginError,
            this,
            [this](QString username, UserLoginError error)
            {
                Q_UNUSED(username)

                *_err << "Login failed: " << toString(error) << endl;
                Q_EMIT exitClient(2);
            }
        );

        connect(
            _command, &Command::executionSuccessful,
            this,
            [this](QString output)
            {
                if (!output.isEmpty())
                    *_out << output << endl;
                else
                    *_out << "Command executed successfully" << endl;

                Q_EMIT exitClient(0);
            }
        );
        connect(
            _command, &Command::executionFailed,
            this,
            [this](int resultCode, QString errorOutput)
            {
                if (!errorOutput.isEmpty())
                    *_err << errorOutput << endl;
                else
                    *_err << "Unknown error, command failed" << endl;

                Q_EMIT exitClient(resultCode);
            }
        );
    }

    void Client::start()
    {
        _serverConnection->connectToHost(_server, _port);
    }

    void Client::connected()
    {
        if (_username.isEmpty())
        {
            executeCommand();
        }
        else
        {
            _serverConnection->login(_username, _password);
        }
    }

    void Client::executeCommand()
    {
        _command->execute(_clientServerInterface);
    }

    QString Client::toString(UserLoginError error)
    {
        switch (error)
        {
            case UserLoginError::AuthenticationFailed:
                return "username/password combination not valid";

            case UserLoginError::UnknownError:
                return "unknown error";
        }

        return "error " + QString::number(int(error));
    }
}

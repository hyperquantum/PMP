/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "commandlineclient.h"

#include "client/localhashidrepository.h"
#include "client/serverconnection.h"
#include "client/serverinterface.h"

#include "command.h"

#include <QTimer>

using namespace PMP::Client;

namespace PMP
{
    CommandlineClient::CommandlineClient(QObject* parent, QTextStream* out,
                                         QTextStream* err, QString server, quint16 port,
                                         QString username, QString password,
                                         Command* command)
     : QObject(parent),
       _out(out),
       _err(err),
       _server(server),
       _port(port),
       _username(username),
       _password(password),
       _hashIdRepository(new LocalHashIdRepository()),
       _serverConnection(nullptr),
       _serverInterface(nullptr),
       _command(command),
       _expectingDisconnect(false)
    {
        // TODO : don't subscribe to _all_ events anymore
        _serverConnection =
                new ServerConnection(this,
                                     _hashIdRepository,
                                     ServerEventSubscription::AllEvents);
        _serverInterface = new ServerInterfaceImpl(_serverConnection);

        connect(
            _serverConnection, &ServerConnection::connected,
            this, &CommandlineClient::connected
        );
        connect(
            _serverConnection, &ServerConnection::cannotConnect,
            this,
            [this](QAbstractSocket::SocketError error)
            {
                *_err << "Failed to connect to the server: " << error << Qt::endl;
                Q_EMIT exitClient(2);
            }
        );
        connect(
            _serverConnection, &ServerConnection::invalidServer,
            this,
            [this]()
            {
                *_err << "Server does not appear to be a PMP server!" << Qt::endl;
                Q_EMIT exitClient(2);
            }
        );
        connect(
            _serverConnection, &ServerConnection::disconnected,
            this,
            [this]()
            {
                if (_expectingDisconnect)
                    return;

                *_err << "Lost connection to the server unexpectedly!" << Qt::endl;
                Q_EMIT exitClient(2);
            }
        );

        connect(
            _serverConnection, &ServerConnection::userLoggedInSuccessfully,
            this, &CommandlineClient::executeCommand
        );
        connect(
            _serverConnection, &ServerConnection::userLoginError,
            this,
            [this](QString username, UserLoginError error)
            {
                Q_UNUSED(username)

                *_err << "Login failed: " << toString(error) << Qt::endl;
                Q_EMIT exitClient(2);
            }
        );

        connect(
            _command, &Command::executionSuccessful,
            this,
            [this](QString output)
            {
                if (!output.isEmpty())
                    *_out << output << Qt::endl;
                else
                    *_out << "Command executed successfully" << Qt::endl;

                Q_EMIT exitClient(0);
            }
        );
        connect(
            _command, &Command::executionFailed,
            this,
            [this](int resultCode, QString errorOutput)
            {
                if (!errorOutput.isEmpty())
                    *_err << errorOutput << Qt::endl;
                else
                    *_err << "Unknown error, command failed" << Qt::endl;

                Q_EMIT exitClient(resultCode);
            }
        );
    }

    CommandlineClient::~CommandlineClient()
    {
        delete _hashIdRepository;
    }

    void CommandlineClient::start()
    {
        _serverConnection->connectToHost(_server, _port);
    }

    void CommandlineClient::connected()
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

    void CommandlineClient::executeCommand()
    {
        _expectingDisconnect = _command->willCauseDisconnect();

        _command->execute(_serverInterface);
    }

    QString CommandlineClient::toString(UserLoginError error)
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

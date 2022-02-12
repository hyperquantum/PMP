/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "serverconnection.h"

#include "collectionfetcher.h"
#include "containerutil.h"
#include "networkprotocol.h"
#include "networkutil.h"
#include "servercapabilitiesimpl.h"
#include "util.h"

#include <QtDebug>
#include <QTimer>

namespace PMP
{
    class ServerConnection::ResultMessageData
    {
    public:
        ResultMessageData(ResultMessageErrorCode errorType,
                          quint32 clientReference, quint32 intData,
                          QByteArray const& blobData)
         : errorType(errorType),
           clientReference(clientReference),
           intData(intData),
           blobData(blobData)
        {
            //
        }

        bool isSuccess() const { return succeeded(errorType); }
        bool isFailure() const { return !isSuccess(); }

        RequestID toRequestID() const { return RequestID(clientReference); }

        const ResultMessageErrorCode errorType;
        const quint32 clientReference;
        const quint32 intData;
        const QByteArray blobData;
    };

    class ServerConnection::ResultHandler
    {
    public:
        virtual ~ResultHandler();

        virtual void handleResult(ResultMessageData const& data) = 0;

        virtual void handleQueueEntryAdditionConfirmation(quint32 clientReference,
                                                          qint32 index, quint32 queueID);

    protected:
        ResultHandler(ServerConnection* parent);

        QString errorDescription(ResultMessageData const& data) const;

        ServerConnection* const _parent;
    };

    ServerConnection::ResultHandler::~ResultHandler()
    {
        //
    }

    ServerConnection::ResultHandler::ResultHandler(ServerConnection* parent)
     : _parent(parent)
    {
        //
    }

    void ServerConnection::ResultHandler::handleQueueEntryAdditionConfirmation(
                                                                  quint32 clientReference,
                                                                  qint32 index,
                                                                  quint32 queueID)
    {
        qWarning() << "ResultHandler does not handle queue entry addition confirmation;"
                   << " ref:" << clientReference << " index:" << index
                   << " QID:" << queueID;
    }

    QString ServerConnection::ResultHandler::errorDescription(
                                                      ResultMessageData const& data) const
    {
        QString text = "client-ref " + QString::number(data.clientReference) + ": ";

        switch (data.errorType)
        {
            case ResultMessageErrorCode::NoError:
            case ResultMessageErrorCode::AlreadyDone:
                text += "unknown error (code indicates success)";
                break;

            case ResultMessageErrorCode::InvalidMessageStructure:
                text += "invalid message structure";
                break;

            case ResultMessageErrorCode::NotLoggedIn:
                text += "not logged in";
                break;

            case ResultMessageErrorCode::QueueIdNotFound:
                text += "QID " + QString::number(data.intData) + " not found";
                return text;

            case ResultMessageErrorCode::InvalidLanguage:
                text += "invalid language";
                break;

            case ResultMessageErrorCode::InvalidCompatibilityInterfaceId:
                text += "invalid compatibility interface ID: "
                        + QString::number(data.intData);
                return text;

            case ResultMessageErrorCode::LanguageNotSet:
                text += "language not set";
                break;

            case ResultMessageErrorCode::DatabaseProblem:
                text += "database problem";
                break;

            case ResultMessageErrorCode::NonFatalInternalServerError:
                text += "non-fatal internal server error";
                break;

            case ResultMessageErrorCode::UnknownError:
                text += "unknown error";
                break;

            default:
                text += "error code " + QString::number(int(data.errorType));
                break;
        }

        /* nothing interesting to add? */
        if (data.intData == 0 && data.blobData.size() == 0)
            return text;

        text +=
            ": intData=" + QString::number(data.intData)
                + ", blobData.size=" + QString::number(data.blobData.size());

        return text;
    }

    /* ============================================================================ */

    class ServerConnection::StandardResultHandler : public ResultHandler
    {
    public:
        StandardResultHandler(ServerConnection* parent,
                           void (ServerConnection::* resultSignal)(ResultMessageErrorCode,
                                                                   RequestID));

        void handleResult(ResultMessageData const& data) override;

    private:
        void (ServerConnection::* _resultSignal)(ResultMessageErrorCode, RequestID);
    };

    ServerConnection::StandardResultHandler::StandardResultHandler(
                                                ServerConnection* parent,
                                                void (ServerConnection::* resultSignal)(
                                                        ResultMessageErrorCode,RequestID))
     : ResultHandler(parent), _resultSignal(resultSignal)
    {
        //
    }

    void ServerConnection::StandardResultHandler::handleResult(
                                                            ResultMessageData const& data)
    {
        if (data.isFailure())
        {
            qWarning() << "StandardResultHandler:" << errorDescription(data);
        }

        (_parent->*_resultSignal)(data.errorType, data.toRequestID());
    }

    /* ============================================================================ */

    class ServerConnection::ParameterlessActionResultHandler : public ResultHandler
    {
    public:
        ParameterlessActionResultHandler(ServerConnection* parent,
                                         ParameterlessActionCode code);

        void handleResult(ResultMessageData const& data) override;

    private:
        ParameterlessActionCode _code;
    };

    ServerConnection::ParameterlessActionResultHandler::ParameterlessActionResultHandler(
                                                             ServerConnection* parent,
                                                             ParameterlessActionCode code)
     : ResultHandler(parent), _code(code)
    {
        //
    }

    void ServerConnection::ParameterlessActionResultHandler::handleResult(
                                                            ResultMessageData const& data)
    {
        if (data.isFailure())
        {
            qWarning() << "ParameterlessActionResultHandler:"
                       << "action" << static_cast<int>(_code) << ":"
                       << errorDescription(data);
        }

        switch (_code)
        {
        case ParameterlessActionCode::Reserved:
            break; /* not supposed to be used */

        case ParameterlessActionCode::ReloadServerSettings:
            Q_EMIT _parent->serverSettingsReloadResultEvent(data.errorType,
                                                            data.toRequestID());
            return;

        case ParameterlessActionCode::DeactivateDelayedStart:
            Q_EMIT _parent->delayedStartDeactivationResultEvent(data.errorType,
                                                                data.toRequestID());
            return;
        }

        qWarning() << "ParameterlessActionResultHandler: unhandled action"
                   << static_cast<int>(_code)
                   << "with client-ref" << data.clientReference;
    }

    /* ============================================================================ */

    class ServerConnection::CollectionFetchResultHandler : public ResultHandler
    {
    public:
        CollectionFetchResultHandler(ServerConnection* parent,
                                     CollectionFetcher* fetcher);

        void handleResult(ResultMessageData const& data) override;

    private:
        CollectionFetcher* _fetcher;
    };

    ServerConnection::CollectionFetchResultHandler::CollectionFetchResultHandler(
                                                               ServerConnection* parent,
                                                               CollectionFetcher* fetcher)
     : ResultHandler(parent), _fetcher(fetcher)
    {
        //
    }

    void ServerConnection::CollectionFetchResultHandler::handleResult(
                                                            ResultMessageData const& data)
    {
        _parent->_collectionFetchers.remove(data.clientReference);

        if (data.isSuccess())
        {
            Q_EMIT _fetcher->completed();
        }
        else
        {
            qWarning() << "CollectionFetchResultHandler:" << errorDescription(data);
            Q_EMIT _fetcher->errorOccurred();
        }

        _fetcher->deleteLater();
    }

    /* ============================================================================ */

    class ServerConnection::TrackInsertionResultHandler : public ResultHandler
    {
    public:
        TrackInsertionResultHandler(ServerConnection* parent, qint32 index);

        void handleResult(ResultMessageData const& data) override;

        void handleQueueEntryAdditionConfirmation(quint32 clientReference, qint32 index,
                                                  quint32 queueID) override;

    private:
        qint32 _index;
    };

    ServerConnection::TrackInsertionResultHandler::TrackInsertionResultHandler(
                                                                 ServerConnection* parent,
                                                                 qint32 index)
     : ResultHandler(parent), _index(index)
    {
        //
    }

    void ServerConnection::TrackInsertionResultHandler::handleResult(
                                                            ResultMessageData const& data)
    {
        if (data.isSuccess())
        {
            /* this is how older servers report a successful insertion */
            auto queueId = data.intData;
            Q_EMIT _parent->queueEntryAdded(_index, queueId, data.toRequestID());
        }
        else
        {
            qWarning() << "TrackInsertionResultHandler:" << errorDescription(data);
            Q_EMIT _parent->queueEntryInsertionFailed(data.errorType, data.toRequestID());
        }
    }

    void
    ServerConnection::TrackInsertionResultHandler::handleQueueEntryAdditionConfirmation(
                                  quint32 clientReference, qint32 index, quint32 queueID)
    {
        Q_EMIT _parent->queueEntryAdded(index, queueID, RequestID(clientReference));
    }

    /* ============================================================================ */

    class ServerConnection::QueueEntryInsertionResultHandler : public ResultHandler
    {
    public:
        QueueEntryInsertionResultHandler(ServerConnection* parent);

        void handleResult(ResultMessageData const& data) override;

        void handleQueueEntryAdditionConfirmation(quint32 clientReference, qint32 index,
                                                  quint32 queueID) override;
    };

    ServerConnection::QueueEntryInsertionResultHandler::QueueEntryInsertionResultHandler(
                                                                 ServerConnection* parent)
     : ResultHandler(parent)
    {
        //
    }

    void ServerConnection::QueueEntryInsertionResultHandler::handleResult(
                                                            ResultMessageData const& data)
    {
        qWarning() << "QueueEntryInsertionResultHandler:" << errorDescription(data);
        Q_EMIT _parent->queueEntryInsertionFailed(data.errorType, data.toRequestID());
    }

    void ServerConnection::QueueEntryInsertionResultHandler::handleQueueEntryAdditionConfirmation(
                                                                  quint32 clientReference,
                                                                  qint32 index,
                                                                  quint32 queueID)
    {
        Q_EMIT _parent->queueEntryAdded(index, queueID, RequestID(clientReference));
    }

    /* ============================================================================ */

    class ServerConnection::DuplicationResultHandler : public ResultHandler
    {
    public:
        DuplicationResultHandler(ServerConnection* parent);

        void handleResult(ResultMessageData const& data) override;

        void handleQueueEntryAdditionConfirmation(quint32 clientReference, qint32 index,
                                                  quint32 queueID) override;
    };

    ServerConnection::DuplicationResultHandler::DuplicationResultHandler(
                                                                 ServerConnection* parent)
     : ResultHandler(parent)
    {
        //
    }

    void ServerConnection::DuplicationResultHandler::handleResult(
                                                            ResultMessageData const& data)
    {
        qWarning() << "DuplicationResultHandler:" << errorDescription(data);
        Q_EMIT _parent->queueEntryInsertionFailed(data.errorType, data.toRequestID());
    }

    void ServerConnection::DuplicationResultHandler::handleQueueEntryAdditionConfirmation(
                                                                  quint32 clientReference,
                                                                  qint32 index,
                                                                  quint32 queueID)
    {
        Q_EMIT _parent->queueEntryAdded(index, queueID, RequestID(clientReference));
    }

    /* ============================================================================ */

    class ServerConnection::CompatibilityInterfaceLanguageSelectionResultHandler
        : public ResultHandler
    {
    public:
        CompatibilityInterfaceLanguageSelectionResultHandler(ServerConnection* parent);

        void handleResult(ResultMessageData const& data) override;
    };

    ServerConnection::CompatibilityInterfaceLanguageSelectionResultHandler
                    ::CompatibilityInterfaceLanguageSelectionResultHandler(
                                                                 ServerConnection* parent)
     : ResultHandler(parent)
    {
        //
    }

    void ServerConnection::CompatibilityInterfaceLanguageSelectionResultHandler
                         ::handleResult(ResultMessageData const& data)
    {
        qWarning() << "CompatibilityInterfaceLanguageSelectionResultHandler:"
                   << errorDescription(data);

        // TODO: report error to the originator of the request
    }

    /* ============================================================================ */

    class ServerConnection::CompatibilityInterfaceActionTriggerResultHandler
        : public ResultHandler
    {
    public:
        CompatibilityInterfaceActionTriggerResultHandler(ServerConnection* parent,
                                                         int interfaceId, int actionId);

        void handleResult(ResultMessageData const& data) override;

    private:
        int _interfaceId;
        int _actionId;
    };

    ServerConnection::CompatibilityInterfaceActionTriggerResultHandler
                    ::CompatibilityInterfaceActionTriggerResultHandler(
                                                                 ServerConnection* parent,
                                                                 int interfaceId,
                                                                 int actionId)
     : ResultHandler(parent),
       _interfaceId(interfaceId),
       _actionId(actionId)
    {
        //
    }

    void ServerConnection::CompatibilityInterfaceActionTriggerResultHandler
                         ::handleResult(ResultMessageData const& data)
    {
        if (data.isSuccess())
        {
            qDebug() << "Compatibility action reported success; interface:"
                     << _interfaceId << " action:" << _actionId;

            Q_EMIT _parent->compatibilityInterfaceActionSucceeded(_interfaceId, _actionId,
                                                                  data.toRequestID());
            return;
        }

        qWarning() << "Compatibility interface" << _interfaceId << "action" << _actionId
                   << "reported failure:"
                   << errorDescription(data);

        Q_EMIT _parent->compatibilityInterfaceActionFailed(_interfaceId, _actionId,
                                                           data.toRequestID());
    }

    /* ============================================================================ */

    const quint16 ServerConnection::ClientProtocolNo = 21;

    const int ServerConnection::KeepAliveIntervalMs = 30 * 1000;
    const int ServerConnection::KeepAliveReplyTimeoutMs = 5 * 1000;

    ServerConnection::ServerConnection(QObject* parent,
                                       ServerEventSubscription eventSubscription)
     : QObject(parent),
       _serverCapabilities(new ServerCapabilitiesImpl()),
       _disconnectReason(DisconnectReason::Unknown),
       _keepAliveTimer(new QTimer(this)),
       _autoSubscribeToEventsAfterConnect(eventSubscription),
       _state(ServerConnection::NotConnected),
       _binarySendingMode(false),
       _serverProtocolNo(-1),
       _nextRef(1),
       _userAccountRegistrationRef(0), _userLoginRef(0), _userLoggedInId(0)
    {
        connect(&_socket, &QTcpSocket::connected, this, &ServerConnection::onConnected);
        connect(
            &_socket, &QTcpSocket::disconnected,
            this, &ServerConnection::onDisconnected
        );
        connect(&_socket, &QTcpSocket::readyRead, this, &ServerConnection::onReadyRead);
        connect(
            &_socket, &QTcpSocket::errorOccurred,
            this, &ServerConnection::onSocketError
        );

        _keepAliveTimer->setSingleShot(true);
        connect(
            _keepAliveTimer, &QTimer::timeout,
            this, &ServerConnection::onKeepAliveTimerTimeout
        );
    }

    ServerConnection::~ServerConnection()
    {
        delete _serverCapabilities;
    }

    void ServerConnection::connectToHost(QString const& host, quint16 port)
    {
        qDebug() << "connecting to" << host << "on port" << port;
        _state = Connecting;
        _readBuffer.clear();
        _socket.connectToHost(host, port);
    }

    QVector<int> ServerConnection::getCompatibilityInterfaceIds()
    {
        return ContainerUtil::toVector(_compatibilityInterfaceIds);
    }

    void ServerConnection::disconnect()
    {
        qDebug() << "disconnect() called";
        breakConnection(DisconnectReason::ClientInitiated);
    }

    ServerCapabilities const& ServerConnection::serverCapabilities() const
    {
        return *_serverCapabilities;
    }

    bool ServerConnection::isConnected() const
    {
        return _state == BinaryMode;
    }

    quint32 ServerConnection::userLoggedInId() const
    {
        return _userLoggedInId;
    }

    QString ServerConnection::userLoggedInName() const
    {
        return _userLoggedInName;
    }

    void ServerConnection::onConnected()
    {
        qDebug() << "connected to host";
        _state = Handshake;
    }

    void ServerConnection::onDisconnected()
    {
        qDebug() << "socket disconnected";

        if (_state == NotConnected)
            return;

        if (_state != Aborting && _state != Disconnecting)
            _disconnectReason = DisconnectReason::Unknown;

        bool wasConnected = _state == Disconnecting;
        _state = NotConnected;

        _readBuffer.clear();
        _binarySendingMode = false;
        _serverProtocolNo = -1;
        _compatibilityInterfaceIds.clear();

        if (wasConnected)
            Q_EMIT disconnected(_disconnectReason);
    }

    void ServerConnection::onReadyRead()
    {
        State state;
        do {
            state = _state;

            switch (state) {
            case NotConnected:
                break; /* problem */
            case Connecting: /* fall-through */
            case Handshake:
            {
                char heading[3];

                if (_socket.peek(heading, 3) < 3) {
                    /* not enough data */
                    break;
                }

                if (heading[0] != 'P'
                    || heading[1] != 'M'
                    || heading[2] != 'P')
                {
                    _state = HandshakeFailure;
                    Q_EMIT invalidServer();
                    breakConnection(DisconnectReason::ProtocolError);
                    return;
                }

                bool hadSemicolon = false;
                char c;
                while (_socket.getChar(&c)) {
                    if (c == ';') {
                        hadSemicolon = true;
                        break;
                    }
                    _readBuffer.append(c);
                }

                if (!hadSemicolon) break; /* not enough data yet */

                QString serverHelloString = QString::fromUtf8(_readBuffer);
                _readBuffer.clear(); /* text consumed */

                qDebug() << "server hello:" << serverHelloString;
                bool supportsNewBinaryCommandWithArg =
                    !serverHelloString.endsWith(" Welcome!");

                /* TODO: other checks */

                /* TODO: maybe extract server version */

                _state = TextMode;

                /* immediately switch to binary mode */
                if (supportsNewBinaryCommandWithArg)
                    sendTextCommand("binary NUxwyGR3ivTcB27VGYdy");
                else
                    sendTextCommand("binary");

                /* send binary hello */
                QByteArray binaryHeader;
                binaryHeader.reserve(5);
                binaryHeader.append("PMP", 3);
                NetworkUtil::append2Bytes(binaryHeader, ClientProtocolNo);
                _socket.write(binaryHeader);
                _socket.flush();

                _binarySendingMode = true;
            }
                break;
            case TextMode:
                readTextCommands();
                break;
            case BinaryHandshake:
            {
                if (_socket.bytesAvailable() < 5) {
                    /* not enough data */
                    break;
                }

                QByteArray heading = _socket.read(5);

                if (!heading.startsWith("PMP"))
                {
                    _state = HandshakeFailure;
                    Q_EMIT invalidServer();
                    breakConnection(DisconnectReason::ProtocolError);
                    return;
                }

                _serverProtocolNo = NetworkUtil::get2Bytes(heading, 3);
                qDebug() << "server protocol version:" << _serverProtocolNo;
                _serverCapabilities->setServerProtocolNumber(_serverProtocolNo);

                _state = BinaryMode;

                _timeSinceLastMessageReceived.start();
                _keepAliveTimer->start(KeepAliveIntervalMs);

                if (_serverProtocolNo >= 12) {
                    /* if interested in protocol extensions... */
                    sendSingleByteAction(18); /*18 = request list of protocol extensions*/
                    sendProtocolExtensionsMessage();
                }

                if (_autoSubscribeToEventsAfterConnect
                        == ServerEventSubscription::AllEvents)
                {
                    sendSingleByteAction(50); /* 50 = subscribe to all server events */
                }
                else if (_autoSubscribeToEventsAfterConnect
                            == ServerEventSubscription::ServerHealthMessages)
                {
                    if (_serverProtocolNo >= 10) {
                         /* 51 = subscribe to server health events */
                        sendSingleByteAction(51);
                    }
                }

                Q_EMIT connected();
            }
                break;
            case BinaryMode:
                readBinaryCommands();
                break;
            case HandshakeFailure:
            case Aborting:
            case Disconnecting:
                /* do nothing */
                break;
            }

            /* maybe a new state will consume the available bytes... */
        } while (_state != state && _socket.bytesAvailable() > 0);
    }

    void ServerConnection::onSocketError(QAbstractSocket::SocketError error)
    {
        qDebug() << "socket error:" << error;

        switch (_state)
        {
        case NotConnected:
        case Aborting:
        case Disconnecting:
            break; /* ignore error */

        case Connecting:
        case Handshake:
        case HandshakeFailure: /* just in case this one here too */
            Q_EMIT cannotConnect(error);
            breakConnection(DisconnectReason::SocketError);
            break;

        case TextMode:
        case BinaryHandshake:
        case BinaryMode:
            breakConnection(DisconnectReason::SocketError);
            break;
        }
    }

    void ServerConnection::onKeepAliveTimerTimeout()
    {
        if (!isConnected())
            return;

        QTimer::singleShot(
            KeepAliveReplyTimeoutMs,
            this,
            [this]()
            {
                if (!_timeSinceLastMessageReceived.hasExpired(KeepAliveIntervalMs))
                    return; /* received a reply in time */

                qDebug() << "server is not responding, going to disconnect now";
                breakConnection(DisconnectReason::KeepAliveTimeout);
            }
        );

        qDebug() << "received nothing from the server for a while, sending keep-alive";

        if (_serverProtocolNo < 19)
        {
            /* for older servers, send a cheap data request */
            requestDynamicModeStatus();
        }
        else
        {
            sendKeepAliveMessage();
        }
    }

    void ServerConnection::breakConnection(DisconnectReason reason)
    {
        qDebug() << "breakConnection() called with reason:" << reason;

        if (_state == NotConnected)
        {
            /* don't change state */
        }
        else if (_state != Aborting && _state != Disconnecting)
        {
            _disconnectReason = reason;

            if (isConnected())
                _state = Disconnecting;
            else
                _state = Aborting;
        }

        _socket.abort();

        _keepAliveTimer->stop();
        _readBuffer.clear();
        _binarySendingMode = false;
        _serverProtocolNo = -1;
    }

    void ServerConnection::readTextCommands()
    {
        do {
            bool hadSemicolon = false;
            char c;
            while (_socket.getChar(&c)) {
                if (c == ';') {
                    hadSemicolon = true;
                    break;
                }
                _readBuffer.append(c);
            }

            if (!hadSemicolon) break; /* not enough data yet */

            QString commandString = QString::fromUtf8(_readBuffer);
            _readBuffer.clear();

            executeTextCommand(commandString);
        } while (_state == TextMode);
    }

    void ServerConnection::executeTextCommand(QString const& commandText)
    {
        if (commandText == "binary") {
            /* switch to binary communication mode */
            _state = BinaryHandshake;
        }
        else {
            qDebug() << "ignoring text command:" << commandText;
        }
    }

    void ServerConnection::sendTextCommand(QString const& command)
    {
        if (!_socket.isValid())
        {
            qWarning() << "cannot send text command when socket not in valid state";
            return;
        }

        qDebug() << "sending command" << command;
        _socket.write((command + ";").toUtf8());
        _socket.flush();
    }

    void ServerConnection::sendBinaryMessage(QByteArray const& message)
    {
        if (!_socket.isValid())
        {
            qWarning() << "cannot send binary message when socket not in valid state";
            return;
        }
        if (!_binarySendingMode)
        {
            qWarning() << "cannot send binary message when not connected in binary mode";
            return;
        }

        auto messageLength = message.length();
        if (messageLength > std::numeric_limits<qint32>::max() - 1)
        {
            qWarning() << "Message too long for sending; length:" << messageLength;
            return;
        }

        QByteArray lengthBytes;
        NetworkUtil::append4BytesSigned(lengthBytes, messageLength);

        _socket.write(lengthBytes);
        _socket.write(message);
        _socket.flush();
    }

    void ServerConnection::sendKeepAliveMessage()
    {
        QByteArray message;
        message.reserve(4);
        NetworkProtocol::append2Bytes(message, ClientMessageType::KeepAliveMessage);
        NetworkUtil::append2Bytes(message, 0); /* payload, unused for now */

        sendBinaryMessage(message);
    }

    void ServerConnection::sendProtocolExtensionsMessage()
    {
        if (_serverProtocolNo < 12) return; /* server will not understand this message */

        QVector<const NetworkProtocol::ProtocolExtension*> extensions;
        //extensions << &_knownExtensionThis;

        quint8 extensionCount = static_cast<quint8>(extensions.size());

        QByteArray message;
        message.reserve(4 + extensionCount * 16); /* estimate */
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::ClientExtensionsMessage);
        NetworkUtil::appendByte(message, 0); /* filler */
        NetworkUtil::appendByte(message, extensionCount);

        for(auto extension : extensions) {
            QByteArray nameBytes = extension->name.toUtf8();
            quint8 nameBytesCount = static_cast<quint8>(nameBytes.size());

            NetworkUtil::appendByte(message, extension->id);
            NetworkUtil::appendByte(message, extension->version);
            NetworkUtil::appendByte(message, nameBytesCount);
            message += nameBytes;
        }

        sendBinaryMessage(message);
    }

    void ServerConnection::sendSingleByteAction(quint8 action)
    {
        qDebug() << "sending single byte action" << static_cast<uint>(action);

        QByteArray message;
        message.reserve(3);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::SingleByteActionMessage);
        NetworkUtil::appendByte(message, action); /* single byte action type */

        sendBinaryMessage(message);
    }

    RequestID ServerConnection::sendParameterlessActionRequest(
                                                             ParameterlessActionCode code)
    {
        auto handler = new ParameterlessActionResultHandler(this, code);
        auto ref = getNewReference();
        _resultHandlers[ref] = handler;

        // TODO : generate error if server does not support this request

        quint16 numericActionCode = static_cast<quint16>(code);

        qDebug() << "sending parameterless action request with action"
                 << numericActionCode << "and client-ref" << ref;

        QByteArray message;
        message.reserve(2 + 2 + 4);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::ParameterlessActionMessage);
        NetworkUtil::append2Bytes(message, numericActionCode);
        NetworkUtil::append4Bytes(message, ref);

        sendBinaryMessage(message);

        return RequestID(ref);
    }

    void ServerConnection::sendQueueFetchRequest(uint startOffset, quint8 length)
    {
        qDebug() << "sending queue fetch request; startOffset=" << startOffset
                 << "; length=" << static_cast<uint>(length);

        QByteArray message;
        message.reserve(7);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::QueueFetchRequestMessage);
        NetworkUtil::append4Bytes(message, startOffset);
        NetworkUtil::appendByte(message, length);

        sendBinaryMessage(message);
    }

    void ServerConnection::deleteQueueEntry(uint queueID)
    {
        QByteArray message;
        message.reserve(6);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::QueueEntryRemovalRequestMessage);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::moveQueueEntry(uint queueID, qint16 offsetDiff)
    {
        QByteArray message;
        message.reserve(8);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::QueueEntryMoveRequestMessage);
        NetworkUtil::append2BytesSigned(message, offsetDiff);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::insertQueueEntryAtFront(FileHash const& hash)
    {
        QByteArray message;
        message.reserve(2 + 2 + NetworkProtocol::FILEHASH_BYTECOUNT);
        NetworkProtocol::append2Bytes(message,
                                  ClientMessageType::AddHashToFrontOfQueueRequestMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkProtocol::appendHash(message, hash);

        sendBinaryMessage(message);
    }

    void ServerConnection::insertQueueEntryAtEnd(FileHash const& hash)
    {
        QByteArray message;
        message.reserve(2 + 2 + NetworkProtocol::FILEHASH_BYTECOUNT);
        NetworkProtocol::append2Bytes(message,
                                    ClientMessageType::AddHashToEndOfQueueRequestMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkProtocol::appendHash(message, hash);

        sendBinaryMessage(message);
    }

    uint ServerConnection::getNewReference()
    {
        auto ref = _nextRef;
        _nextRef++;

        if (_nextRef >= 0x80000000u)
        {
            qWarning() << "client references getting really big, going to disconnect";
            QTimer::singleShot(
                0, this, [this]() { breakConnection(DisconnectReason::Unknown); }
            );
        }

        return ref;
    }

    RequestID ServerConnection::getNewRequestId()
    {
        return RequestID(getNewReference());
    }

    RequestID ServerConnection::signalRequestError(ResultMessageErrorCode errorCode,
                             void (ServerConnection::*errorSignal)(ResultMessageErrorCode,
                                                                   RequestID))
    {
        auto requestId = getNewRequestId();

        QTimer::singleShot(
            0,
            this,
            [this, errorSignal, errorCode, requestId]()
            {
                Q_EMIT (this->*errorSignal)(errorCode, requestId);
            }
        );

        return requestId;
    }

    RequestID ServerConnection::signalServerTooOldError(
                             void (ServerConnection::*errorSignal)(ResultMessageErrorCode,
                                                                   RequestID))
    {
        return signalRequestError(ResultMessageErrorCode::ServerTooOld, errorSignal);
    }

    RequestID ServerConnection::reloadServerSettings()
    {
        if (!serverCapabilities().supportsReloadingServerSettings())
        {
            return signalServerTooOldError(
                                      &ServerConnection::serverSettingsReloadResultEvent);
        }

        qDebug() << "sending request to reload server settings";

        return sendParameterlessActionRequest(
                                           ParameterlessActionCode::ReloadServerSettings);
    }

    RequestID ServerConnection::activateDelayedStart(qint64 delayMilliseconds)
    {
        if (!serverCapabilities().supportsDelayedStart())
        {
            return signalServerTooOldError(
                                    &ServerConnection::delayedStartActivationResultEvent);
        }

        auto handler =
            new StandardResultHandler(this,
                                    &ServerConnection::delayedStartActivationResultEvent);
        auto ref = getNewReference();
        _resultHandlers[ref] = handler;

        qDebug() << "sending request to activate delayed start; delay:"
                 << delayMilliseconds << "ms; ref:" << ref;

        QByteArray message;
        message.reserve(2 + 2 + 4 + 8);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::ActivateDelayedStartRequest);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append4Bytes(message, ref);
        NetworkUtil::append8BytesSigned(message, delayMilliseconds);

        sendBinaryMessage(message);

        return RequestID(ref);
    }

    RequestID ServerConnection::deactivateDelayedStart()
    {
        if (!serverCapabilities().supportsDelayedStart())
        {
            return signalServerTooOldError(
                                  &ServerConnection::delayedStartDeactivationResultEvent);
        }

        qDebug() << "sending request to deactivate delayed start";

        return sendParameterlessActionRequest(
                                         ParameterlessActionCode::DeactivateDelayedStart);
    }

    RequestID ServerConnection::insertQueueEntryAtIndex(const FileHash& hash,
                                                        quint32 index)
    {
        if (hash.isNull())
            return signalRequestError(ResultMessageErrorCode::InvalidHash,
                                      &ServerConnection::queueEntryInsertionFailed);

        auto ref = registerResultHandler(new TrackInsertionResultHandler(this, index));

        qDebug() << "sending request to add a track at index" << index << "; ref=" << ref;

        QByteArray message;
        message.reserve(2 + 2 + 4 + 4 + NetworkProtocol::FILEHASH_BYTECOUNT);
        NetworkProtocol::append2Bytes(message,
                                    ClientMessageType::InsertHashIntoQueueRequestMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append4Bytes(message, ref);
        NetworkUtil::append4Bytes(message, index);
        NetworkProtocol::appendHash(message, hash);

        sendBinaryMessage(message);

        return RequestID(ref);
    }

    RequestID ServerConnection::insertSpecialQueueItemAtIndex(
                                                            SpecialQueueItemType itemType,
                                                            int index,
                                                            QueueIndexType indexType)
    {
        if (!serverCapabilities().supportsInsertingBreaksAtAnyIndex()
                || (itemType == SpecialQueueItemType::Barrier
                    && !serverCapabilities().supportsInsertingBarriers()))
        {
            return signalServerTooOldError(&ServerConnection::queueEntryInsertionFailed);
        }

        auto handler = new QueueEntryInsertionResultHandler(this);
        auto ref = getNewReference();
        _resultHandlers[ref] = handler;

        qDebug() << "sending request to insert" << itemType << "at index" << index
                 << "; ref=" << ref;

        quint8 itemTypeByte = (itemType == SpecialQueueItemType::Barrier) ? 2 : 1;
        quint8 indexTypeByte = indexType == QueueIndexType::Normal ? 0 : 1;

        QByteArray message;
        message.reserve(2 + 1 + 1 + 4 + 4);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::InsertSpecialQueueItemRequest);
        NetworkUtil::appendByte(message, itemTypeByte);
        NetworkUtil::appendByte(message, indexTypeByte);
        NetworkUtil::append4Bytes(message, ref);
        NetworkUtil::append4Bytes(message, index);

        sendBinaryMessage(message);

        return RequestID(ref);
    }

    RequestID ServerConnection::duplicateQueueEntry(quint32 queueID)
    {
        auto ref = registerResultHandler(new DuplicationResultHandler(this));

        qDebug() << "sending request to duplicate QID" << queueID << "; ref=" << ref;

        QByteArray message;
        message.reserve(2 + 2 + 4 + 4);
        NetworkProtocol::append2Bytes(message,
                                  ClientMessageType::QueueEntryDuplicationRequestMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append4Bytes(message, ref);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);

        return RequestID(ref);
    }

    void ServerConnection::sendQueueEntryInfoRequest(uint queueID)
    {
        if (queueID == 0) return;

        qDebug() << "sending request for track info of QID" << queueID;

        QByteArray message;
        message.reserve(6);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::TrackInfoRequestMessage);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::sendQueueEntryInfoRequest(QList<uint> const& queueIDs)
    {
        if (queueIDs.empty()) return;

        if (queueIDs.size() == 1) {
            qDebug() << "sending bulk request for track info of QID" << queueIDs[0];
        }
        else {
            qDebug() << "sending bulk request for track info of"
                     << queueIDs.size() << "QIDs";
        }

        QByteArray message;
        message.reserve(2 + 4 * queueIDs.size());
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::BulkTrackInfoRequestMessage);

        for (auto QID : queueIDs)
        {
            NetworkUtil::append4Bytes(message, QID);
        }

        sendBinaryMessage(message);
    }

    void ServerConnection::sendQueueEntryHashRequest(QList<uint> const& queueIDs)
    {
        if (queueIDs.empty()) return;

        if (queueIDs.size() == 1) {
            qDebug() << "sending bulk request for hash info of QID" << queueIDs[0];
        }
        else {
            qDebug() << "sending bulk request for hash info of"
                     << queueIDs.size() << "QIDs";
        }

        QByteArray message;
        message.reserve(2 + 2 + 4 * queueIDs.size());
        NetworkProtocol::append2Bytes(message,
                                     ClientMessageType::BulkQueueEntryHashRequestMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */

        for (auto QID : queueIDs)
        {
            NetworkUtil::append4Bytes(message, QID);
        }

        sendBinaryMessage(message);
    }

    void ServerConnection::sendHashUserDataRequest(quint32 userId,
                                                   const QList<FileHash>& hashes)
    {
        if (hashes.empty()) return;

        if (hashes.size() == 1) {
            qDebug() << "sending bulk user data request for hash" << hashes[0]
                     << "for user" << userId;
        }
        else {
            qDebug() << "sending bulk user data request for" << hashes.size()
                     << "hashes for user" << userId;
        }

        QByteArray message;
        message.reserve(2 + 2 + 4 + hashes.size() * NetworkProtocol::FILEHASH_BYTECOUNT);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::HashUserDataRequestMessage);
        NetworkUtil::append2Bytes(message, 2 | 1); /* request prev. heard & score */
        NetworkUtil::append4Bytes(message, userId); /* user ID */

        for (auto& hash : hashes)
        {
            if (hash.isNull())
                qWarning() << "request contains null hash";

            NetworkProtocol::appendHash(message, hash);
        }

        sendBinaryMessage(message);
    }

    void ServerConnection::sendPossibleFilenamesRequest(uint queueID)
    {
        qDebug() << "sending request for possible filenames of QID" << queueID;

        QByteArray message;
        message.reserve(6);
        NetworkProtocol::append2Bytes(message,
                         ClientMessageType::PossibleFilenamesForQueueEntryRequestMessage);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::createNewUserAccount(QString login, QString password)
    {
        _userAccountRegistrationRef = getNewReference();
        _userAccountRegistrationLogin = login;
        _userAccountRegistrationPassword = password;

        sendInitiateNewUserAccountMessage(login, _userAccountRegistrationRef);
    }

    void ServerConnection::login(QString login, QString password)
    {
        _userLoginRef = getNewReference();
        _userLoggingIn = login;
        _userLoggingInPassword = password;

        sendInitiateLoginMessage(login, _userLoginRef);
    }

    void ServerConnection::switchToPublicMode()
    {
        sendSingleByteAction(30);
    }

    void ServerConnection::switchToPersonalMode()
    {
        sendSingleByteAction(31);
    }

    void ServerConnection::requestUserPlayingForMode()
    {
        sendSingleByteAction(14);
    }

    void ServerConnection::handleNewUserSalt(QString login, QByteArray salt)
    {
        if (login != _userAccountRegistrationLogin) return;

        uint reference = _userAccountRegistrationRef;

        QByteArray hashedPassword =
            NetworkProtocol::hashPassword(salt, _userAccountRegistrationPassword);

        sendFinishNewUserAccountMessage(login, salt, hashedPassword, reference);
    }

    void ServerConnection::handleLoginSalt(QString login, QByteArray userSalt,
                                           QByteArray sessionSalt)
    {
        if (login != _userLoggingIn) return;

        uint reference = _userLoginRef;

        QByteArray hashedPassword =
            NetworkProtocol::hashPasswordForSession(
                userSalt, sessionSalt, _userLoggingInPassword
            );

        sendFinishLoginMessage(login, userSalt, sessionSalt, hashedPassword, reference);
    }

    void ServerConnection::handleUserRegistrationResult(ResultMessageErrorCode errorCode,
                                                        quint32 intData,
                                                        QByteArray const& blobData)
    {
        Q_UNUSED(blobData);

        QString login = _userAccountRegistrationLogin;

        /* clean up potentially sensitive information */
        _userAccountRegistrationLogin = "";
        _userAccountRegistrationPassword = "";

        if (succeeded(errorCode))
        {
            Q_EMIT userAccountCreatedSuccessfully(login, intData);
        }
        else
        {
            UserRegistrationError error;

            switch (errorCode)
            {
            case ResultMessageErrorCode::UserAccountAlreadyExists:
                error = UserRegistrationError::AccountAlreadyExists;
                break;
            case ResultMessageErrorCode::InvalidUserAccountName:
                error = UserRegistrationError::InvalidAccountName;
                break;
            default:
                error = UserRegistrationError::UnknownError;
                break;
            }

            Q_EMIT userAccountCreationError(login, error);
        }
    }

    void ServerConnection::handleUserLoginResult(ResultMessageErrorCode errorCode,
                                                 quint32 intData,
                                                 const QByteArray& blobData)
    {
        Q_UNUSED(blobData);

        QString login = _userLoggingIn;
        quint32 userId = intData;

        qDebug() << " received login result: errorType =" << static_cast<int>(errorCode)
                 << "; login =" << login << "; id =" << userId;

        /* clean up potentially sensitive information */
        _userLoggingInPassword = "";

        if (succeeded(errorCode))
        {
            _userLoggedInId = userId;
            _userLoggedInName = _userLoggingIn;
            _userLoggingIn = "";
            Q_EMIT userLoggedInSuccessfully(login, intData);
        }
        else
        {
            _userLoggingIn = "";

            UserLoginError error;

            switch (errorCode) {
            case ResultMessageErrorCode::InvalidUserAccountName:
            case ResultMessageErrorCode::UserLoginAuthenticationFailed:
                error = UserLoginError::AuthenticationFailed;
                break;
            default:
                error = UserLoginError::UnknownError;
                break;
            }

            Q_EMIT userLoginError(login, error);
        }
    }

    void ServerConnection::onFullIndexationRunningStatusReceived(bool running)
    {
        auto oldValue = _doingFullIndexation;
        _doingFullIndexation = running;

        Q_EMIT fullIndexationStatusReceived(running);

        if (oldValue.isKnown()) {
            if (running)
                Q_EMIT fullIndexationStarted();
            else
                Q_EMIT fullIndexationFinished();
        }
    }

    void ServerConnection::sendPlayerHistoryRequest(int limit)
    {
        if (limit < 0) limit = 0;
        if (limit > 255) { limit = 255; }

        quint8 limitUnsigned = static_cast<quint8>(limit);

        QByteArray message;
        message.reserve(2 + 2);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::PlayerHistoryRequestMessage);
        NetworkUtil::appendByte(message, 0); /* filler */
        NetworkUtil::appendByte(message, limitUnsigned);

        sendBinaryMessage(message);
    }

    void ServerConnection::sendUserAccountsFetchRequest()
    {
        sendSingleByteAction(13); /* 13 = fetch list of user accounts */
    }

    void ServerConnection::shutdownServer()
    {
        sendSingleByteAction(99); /* 99 = shutdown server */
    }

    void ServerConnection::sendDatabaseIdentifierRequest()
    {
        sendSingleByteAction(17); /* 17 = request for database UUID */
    }

    void ServerConnection::sendServerInstanceIdentifierRequest()
    {
        sendSingleByteAction(12); /* 12 = request for server instance UUID */
    }

    void ServerConnection::sendServerNameRequest()
    {
        sendSingleByteAction(16); /* 16 = request for server name */
    }

    void ServerConnection::requestPlayerState()
    {
        sendSingleByteAction(10); /* 10 = request player state */
    }

    void ServerConnection::play()
    {
        sendSingleByteAction(1); /* 1 = play */
    }

    void ServerConnection::pause()
    {
        sendSingleByteAction(2); /* 2 = pause */
    }

    void ServerConnection::skip()
    {
        sendSingleByteAction(3); /* 3 = skip */
    }

    void ServerConnection::insertBreakAtFrontIfNotExists()
    {
        sendSingleByteAction(4); /* 4 = insert break at front */
    }

    void ServerConnection::seekTo(uint queueID, qint64 position)
    {
        if (position < 0)
        {
            qWarning() << "Position out of range:" << position;
            return;
        }

        QByteArray message;
        message.reserve(14);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::PlayerSeekRequestMessage);
        NetworkUtil::append4Bytes(message, queueID);
        NetworkUtil::append8BytesSigned(message, position);

        sendBinaryMessage(message);
    }

    void ServerConnection::setVolume(int percentage)
    {
        if (percentage < 0 || percentage > 100) {
            qWarning() << "Invalid percentage:" << percentage;
            return;
        }

        quint8 percentageUnsigned = static_cast<quint8>(percentage);

        sendSingleByteAction(100 + percentageUnsigned); /* 100 to 200 = set volume */
    }

    void ServerConnection::enableDynamicMode()
    {
        sendSingleByteAction(20); /* 20 = enable dynamic mode */
    }

    void ServerConnection::disableDynamicMode()
    {
        sendSingleByteAction(21); /* 21 = disable dynamic mode */
    }

    void ServerConnection::expandQueue()
    {
        sendSingleByteAction(22); /* 22 = request queue expansion */
    }

    void ServerConnection::trimQueue()
    {
        sendSingleByteAction(23); /* 23 = request queue trim */
    }

    void ServerConnection::requestDynamicModeStatus()
    {
        sendSingleByteAction(11); /* 11 = request status of dynamic mode */
    }

    void ServerConnection::setDynamicModeNoRepetitionSpan(int seconds)
    {
        if (seconds < 0 || seconds > std::numeric_limits<qint32>::max() - 1)
        {
            qWarning() << "Repetition span out of range:" << seconds;
            return;
        }

        QByteArray message;
        message.reserve(6);
        NetworkProtocol::append2Bytes(message,
                                  ClientMessageType::GeneratorNonRepetitionChangeMessage);
        NetworkUtil::append4BytesSigned(message, seconds);

        sendBinaryMessage(message);
    }

    void ServerConnection::startDynamicModeWave()
    {
        sendSingleByteAction(24); /* 24 = start dynamic mode wave */
    }

    void ServerConnection::terminateDynamicModeWave()
    {
        sendSingleByteAction(25); /* 25 = terminate dynamic mode wave */
    }

    void ServerConnection::startFullIndexation()
    {
        sendSingleByteAction(40); /* 40 = start full indexation */
    }

    void ServerConnection::requestFullIndexationRunningStatus()
    {
        sendSingleByteAction(15); /* 15 = request for full indexation running status */
    }

    void ServerConnection::fetchCollection(CollectionFetcher* fetcher)
    {
        fetcher->setParent(this);

        auto handler = new CollectionFetchResultHandler(this, fetcher);
        auto fetcherReference = registerResultHandler(handler);
        _collectionFetchers[fetcherReference] = fetcher;
        sendCollectionFetchRequestMessage(fetcherReference);
    }

    void ServerConnection::sendInitiateNewUserAccountMessage(QString login,
                                                             quint32 clientReference)
    {
        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(2 + 2 + 4 + loginBytes.size());
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::InitiateNewUserAccountMessage);
        NetworkUtil::appendByteUnsigned(message, loginBytes.size());
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::append4Bytes(message, clientReference);
        message += loginBytes;

        sendBinaryMessage(message);
    }

    void ServerConnection::sendInitiateLoginMessage(QString login,
                                                    quint32 clientReference)
    {
        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(2 + 2 + 4 + loginBytes.size());
        NetworkProtocol::append2Bytes(message, ClientMessageType::InitiateLoginMessage);
        NetworkUtil::appendByteUnsigned(message, loginBytes.size());
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::append4Bytes(message, clientReference);
        message += loginBytes;

        sendBinaryMessage(message);
    }

    void ServerConnection::sendFinishNewUserAccountMessage(QString login, QByteArray salt,
                                                           QByteArray hashedPassword,
                                                           quint32 clientReference)
    {
        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(4 + 4 + loginBytes.size() + salt.size() + hashedPassword.size());
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::FinishNewUserAccountMessage);
        NetworkUtil::appendByteUnsigned(message, loginBytes.size());
        NetworkUtil::appendByteUnsigned(message, salt.size());
        NetworkUtil::append4Bytes(message, clientReference);
        message += loginBytes;
        message += salt;
        message += hashedPassword;

        sendBinaryMessage(message);
    }

    void ServerConnection::sendFinishLoginMessage(QString login, QByteArray userSalt,
                                                  QByteArray sessionSalt,
                                                  QByteArray hashedPassword,
                                                  quint32 clientReference)
    {
        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(
            4 + 4 + 4 + loginBytes.size() + userSalt.size() + sessionSalt.size()
            + hashedPassword.size()
        );
        NetworkProtocol::append2Bytes(message, ClientMessageType::FinishLoginMessage);
        NetworkUtil::append2Bytes(message, 0); /* unused */
        NetworkUtil::appendByteUnsigned(message, loginBytes.size());
        NetworkUtil::appendByteUnsigned(message, userSalt.size());
        NetworkUtil::appendByteUnsigned(message, sessionSalt.size());
        NetworkUtil::appendByteUnsigned(message, hashedPassword.size());
        NetworkUtil::append4Bytes(message, clientReference);
        message += loginBytes;
        message += userSalt;
        message += sessionSalt;
        message += hashedPassword;

        sendBinaryMessage(message);
    }

    void ServerConnection::sendCollectionFetchRequestMessage(uint clientReference)
    {
        QByteArray message;
        message.reserve(4 + 4);
        NetworkProtocol::append2Bytes(message,
                                      ClientMessageType::CollectionFetchRequestMessage);
        NetworkUtil::append2Bytes(message, 0); /* unused */
        NetworkUtil::append4Bytes(message, clientReference);

        sendBinaryMessage(message);
    }

    void ServerConnection::parseCompatibilityInterfaceAnnouncement(
                                                                const QByteArray& message)
    {
        qDebug() << "received compatibility interfaces announcement";

        if (message.length() < 4)
            return; /* invalid message */

        int interfaceCount = NetworkUtil::get2BytesUnsignedToInt(message, 2);

        int countWithPaddingIncluded =
                (interfaceCount % 2 == 1) ? (interfaceCount + 1) : interfaceCount;

        if (message.length() != 4 + 2 * countWithPaddingIncluded)
            return; /* invalid message */

        QVector<int> ids;
        ids.reserve(interfaceCount);

        int offset = 4;
        for (int i = 0; i < interfaceCount; ++i)
        {
            int interfaceId = NetworkUtil::get2BytesUnsignedToInt(message, offset);
            offset += 2;

            qDebug() << "compatibility interface:" << interfaceId;

            ids.append(interfaceId);
            _compatibilityInterfaceIds << interfaceId;
        }

        Q_EMIT compatibilityInterfaceAnnouncementReceived(ids);
    }

    void ServerConnection::parseCompatibilityInterfaceLanguageSelectionConfirmation(
                                                                const QByteArray& message)
    {
        qDebug() << "received compatibility interface language selection confirmation";

        if (message.length() != 12)
            return; /* invalid message */

        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
        auto languageEncoded = message.mid(9, 3);

        auto language = NetworkProtocol::decodeLanguage(languageEncoded);

        qDebug() << "language confirmation: language:" << language
                 << " reference:" << clientReference;

        discardResultHandler(clientReference);

        Q_EMIT compatibilityInterfaceLanguageSelectionSucceeded(language);
    }

    void ServerConnection::parseCompatibilityInterfaceDefinition(
                                                                const QByteArray& message)
    {
        qDebug() << "received compatibility interface definition";

        if (message.length() < 16)
            return; /* invalid message */

        int interfaceId = NetworkUtil::get2BytesUnsignedToInt(message, 2);
        quint16 interfaceStateEncoded = NetworkUtil::get2Bytes(message, 4);
        int actionCount = NetworkUtil::get2BytesUnsignedToInt(message, 6);
        int interfaceCaptionByteCount = NetworkUtil::get2BytesUnsignedToInt(message, 8);
        int interfaceDescriptionByteCount =
                NetworkUtil::get2BytesUnsignedToInt(message, 10);
        int interfaceTitleByteCount = NetworkUtil::getByteUnsignedToInt(message, 12);
        auto languageEncoded = message.mid(13, 3);

        qDebug() << "Compatibility interface" << interfaceId
                 << "has encoded state" << int(interfaceStateEncoded)
                 << "and has" << actionCount << "actions";

        auto interfaceState =
                NetworkProtocol::decodeCompatibilityUiState(interfaceStateEncoded);
        qDebug() << "Compatibility interface" << interfaceId
                 << "has priority:" << interfaceState.priority();

        auto language = NetworkProtocol::decodeLanguage(languageEncoded);
        qDebug() << "Compatibility interface" << interfaceId
                 << "text is provided in language:" << language;

        int offset = 16;
        if (message.length() < offset + interfaceTitleByteCount
                                      + interfaceCaptionByteCount
                                      + interfaceDescriptionByteCount)
        {
            return; /* invalid message */
        }

        auto interfaceTitle =
                NetworkUtil::getUtf8String(message, offset, interfaceTitleByteCount);
        offset += interfaceTitleByteCount;
        qDebug() << "Compatibility interface" << interfaceId
                 << "has title:" << interfaceTitle;

        auto interfaceCaption =
                NetworkUtil::getUtf8String(message, offset, interfaceCaptionByteCount);
        offset += interfaceCaptionByteCount;
        qDebug() << "Compatibility interface" << interfaceId
                 << "has caption:" << interfaceCaption;

        auto interfaceDescription =
               NetworkUtil::getUtf8String(message, offset, interfaceDescriptionByteCount);
        offset += interfaceDescriptionByteCount;
        qDebug() << "Compatibility interface" << interfaceId
                 << "has description:" << interfaceDescription;

        QVector<int> actionIds;
        QVector<NetworkProtocol::CompatibilityUserInterfaceAction> actions;
        actionIds.reserve(actionCount);
        actions.reserve(actionCount);

        for (int i = 0; i < actionCount; ++i)
        {
            if (offset + 6 > message.size())
                return; /* invalid message */

            int actionId = NetworkUtil::get2BytesUnsignedToInt(message, offset);
            quint8 actionStateEncoded = NetworkUtil::getByte(message, offset + 3);
            int actionCaptionByteCount =
                    NetworkUtil::get2BytesUnsignedToInt(message, offset + 4);
            offset += 6;

            auto actionState =
                    NetworkProtocol::decodeCompatibilityUiActionState(actionStateEncoded);

            qDebug() << " Compatibility action with ID" << actionId << "has:"
                     << "visible:" << (actionState.visible() ? "Y" : "N")
                     << "enabled:" << (actionState.enabled() ? "Y" : "N")
                     << "disable-when-triggered:"
                     << (actionState.disableWhenTriggered() ? "Y" : "N");

            if (offset > message.size() - actionCaptionByteCount)
                return; /* invalid message */

            auto actionCaption =
                    NetworkUtil::getUtf8String(message, offset, actionCaptionByteCount);
            offset += actionCaptionByteCount;

            qDebug() << " Compatibility action with ID" << actionId
                     << "has caption:" << actionCaption;

            NetworkProtocol::CompatibilityUserInterfaceAction action(actionId,
                                                                     actionState,
                                                                     actionCaption);

            actionIds.append(actionId);
            actions.append(action);
        }

        if (offset != message.size())
            return; /* invalid message */

        NetworkProtocol::CompatibilityUserInterface interface(interfaceId,
                                                              language,
                                                              interfaceTitle,
                                                              interfaceCaption,
                                                              interfaceDescription,
                                                              actions);

        Q_EMIT compatibilityInterfaceDefinitionReceived(interfaceId, interfaceState,
                                                        language, interfaceTitle,
                                                        interfaceCaption,
                                                        interfaceDescription,
                                                        actionIds);

        for (auto const& action : qAsConst(actions))
        {
            Q_EMIT compatibilityInterfaceActionDefinitionReceived(interfaceId,
                                                                  action.id(),
                                                                  action.state(),
                                                                  language,
                                                                  action.caption());
        }
    }

    void ServerConnection::parseCompatibilityInterfaceStateUpdate(
                                                                const QByteArray& message)
    {
        qDebug() << "received compatibility interface state update";

        if (message.length() != 8)
            return; /* invalid message */

        int interfaceId = NetworkUtil::get2BytesUnsignedToInt(message, 4);
        quint16 encodedState = NetworkUtil::get2Bytes(message, 6);

        auto interfaceState = NetworkProtocol::decodeCompatibilityUiState(encodedState);
        qDebug() << "Compatibility interface" << interfaceId
                 << "now has priority:" << interfaceState.priority();

        Q_EMIT compatibilityInterfaceStateChanged(interfaceId, interfaceState);
    }

    void ServerConnection::parseCompatibilityInterfaceActionStateUpdate(
                                                                const QByteArray& message)
    {
        qDebug() << "received compatibility interface action state update";

        if (message.length() != 8)
            return; /* invalid message */

        int interfaceId = NetworkUtil::get2BytesUnsignedToInt(message, 2);
        int actionId = NetworkUtil::get2BytesUnsignedToInt(message, 4);
        quint8 actionStateEncoded = NetworkUtil::getByte(message, 7);

        auto actionState =
                NetworkProtocol::decodeCompatibilityUiActionState(actionStateEncoded);

        qDebug() << "Compatibility interface" << interfaceId << "action" << actionId
                 << "now has:"
                 << "visible:" << (actionState.visible() ? "Y" : "N")
                 << "enabled:" << (actionState.enabled() ? "Y" : "N")
                 << "disable-when-triggered:"
                 << (actionState.disableWhenTriggered() ? "Y" : "N");

        Q_EMIT compatibilityInterfaceActionStateChanged(interfaceId, actionId,
                                                        actionState);
    }

    void ServerConnection::parseCompatibilityInterfaceTextUpdate(
                                                                const QByteArray& message)
    {
        qDebug() << "received compatibility interface text update";

        if (message.length() < 12)
            return; /* invalid message */

        int interfaceId = NetworkUtil::get2BytesUnsignedToInt(message, 2);
        int interfaceCaptionByteCount = NetworkUtil::get2BytesUnsignedToInt(message, 4);
        int interfaceDescriptionByteCount =
                NetworkUtil::get2BytesUnsignedToInt(message, 6);
        auto languageEncoded = message.mid(9, 3);

        auto language = NetworkProtocol::decodeLanguage(languageEncoded);
        qDebug() << "Compatibility interface" << interfaceId
                 << "text update is provided in language:" << language;

        if (message.size() != 12 + interfaceCaptionByteCount
                                 + interfaceDescriptionByteCount)
        {
            return; /* invalid message */
        }

        int offset = 12;

        auto interfaceCaption =
                NetworkUtil::getUtf8String(message, offset, interfaceCaptionByteCount);
        offset += interfaceCaptionByteCount;
        qDebug() << "Compatibility interface" << interfaceId
                 << "now has caption:" << interfaceCaption;

        auto interfaceDescription =
               NetworkUtil::getUtf8String(message, offset, interfaceDescriptionByteCount);
        qDebug() << "Compatibility interface" << interfaceId
                 << "now has description:" << interfaceDescription;

        Q_EMIT compatibilityInterfaceTextChanged(interfaceId, language, interfaceCaption,
                                                 interfaceDescription);
    }

    void ServerConnection::parseCompatibilityInterfaceActionTextUpdate(
                                                                const QByteArray& message)
    {
        qDebug() << "received compatibility interface action text update";

        if (message.length() < 12)
            return; /* invalid message */

        int interfaceId = NetworkUtil::get2BytesUnsignedToInt(message, 2);
        int actionId = NetworkUtil::get2BytesUnsignedToInt(message, 4);
        int actionCaptionByteCount = NetworkUtil::get2BytesUnsignedToInt(message, 6);
        auto languageEncoded = message.mid(9, 3);

        auto language = NetworkProtocol::decodeLanguage(languageEncoded);
        qDebug() << "Compatibility interface" << interfaceId << "action" << actionId
                 << "text update is provided in language:" << language;

        if (message.size() != 12 + actionCaptionByteCount)
            return; /* invalid message */

        auto actionCaption =
                NetworkUtil::getUtf8String(message, 12, actionCaptionByteCount);

        qDebug() << "Compatibility interface" << interfaceId << "action" << actionId
                 << "now has caption:" << actionCaption;

        Q_EMIT compatibilityInterfaceActionTextChanged(interfaceId, actionId, language,
                                                       actionCaption);
    }

    void ServerConnection::sendCompatibilityInterfaceLanguageSelectionRequest(
                                                           UserInterfaceLanguage language)
    {
        if (!_binarySendingMode)
            return; /* only supported in binary mode */

        auto ref =
            registerResultHandler(
                          new CompatibilityInterfaceLanguageSelectionResultHandler(this));

        qDebug() << "sending compatibility interface language selection request:"
                 << "language:" << language << " reference:" << ref;

        auto primaryLanguage = NetworkProtocol::encodeLanguage(language);
        auto secondaryLanguage = QByteArray("eng");

        QByteArray message;
        message.reserve(2 + 2 + 4 + 4 + 4);
        NetworkProtocol::append2Bytes(message,
                       ClientMessageType::CompatibilityInterfaceLanguageSelectionRequest);
        NetworkUtil::append2Bytes(message, 0); /* unused */
        NetworkUtil::append4Bytes(message, ref);
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::appendByte(message, primaryLanguage[0]);
        NetworkUtil::appendByte(message, primaryLanguage[1]);
        NetworkUtil::appendByte(message, primaryLanguage[2]);
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::appendByte(message, secondaryLanguage[0]);
        NetworkUtil::appendByte(message, secondaryLanguage[1]);
        NetworkUtil::appendByte(message, secondaryLanguage[2]);

        sendBinaryMessage(message);
    }

    void ServerConnection::sendCompatibilityInterfaceDefinitionsRequest(
                                                                QVector<int> interfaceIds)
    {
        if (!_binarySendingMode)
            return; /* only supported in binary mode */

        if (interfaceIds.isEmpty())
            return;

        qDebug() << "sending compatibility interfaces definitions request for"
                 << interfaceIds.size() << "interface(s)";

        QByteArray message;
        message.reserve(2 + 2 + 2 * interfaceIds.size() + 2);
        NetworkProtocol::append2Bytes(message,
                             ClientMessageType::CompatibilityInterfaceDefinitionsRequest);
        NetworkUtil::append2BytesUnsigned(message, interfaceIds.size());

        for (auto interfaceId : qAsConst(interfaceIds))
        {
            NetworkUtil::append2BytesUnsigned(message, interfaceId);
        }

        if (interfaceIds.size() % 2 != 0) // padding
        {
            NetworkUtil::append2Bytes(message, 0);
        }

        sendBinaryMessage(message);
    }

    RequestID ServerConnection::sendCompatibilityInterfaceTriggerActionRequest(
                                                                          int interfaceId,
                                                                          int actionId)
    {
        if (!_binarySendingMode)
            return {}; /* only supported in binary mode */

        auto* resultHandler =
            new CompatibilityInterfaceActionTriggerResultHandler(this, interfaceId,
                                                                 actionId);
        auto ref = registerResultHandler(resultHandler);

        qDebug() << "sending compatibility interface action trigger request; ref:" << ref;

        QByteArray message;
        message.reserve(2 + 2 + 2 + 2 + 4);
        NetworkProtocol::append2Bytes(message,
                           ClientMessageType::CompatibilityInterfaceTriggerActionRequest);
        NetworkUtil::append2Bytes(message, 0); /* unused */
        NetworkUtil::append2BytesUnsigned(message, interfaceId);
        NetworkUtil::append2BytesUnsigned(message, actionId);
        NetworkUtil::append4Bytes(message, ref);

        sendBinaryMessage(message);

        return RequestID(ref);
    }

    void ServerConnection::readBinaryCommands()
    {
        char lengthBytes[4];

        while
            (_socket.isOpen()
            && _socket.peek(lengthBytes, sizeof(lengthBytes)) == sizeof(lengthBytes))
        {
            quint32 messageLength = NetworkUtil::get4Bytes(lengthBytes);

            if (_socket.bytesAvailable() - sizeof(lengthBytes) < messageLength) {
                qDebug() << "waiting for incoming message with length" << messageLength
                         << " --- only partially received";
                break; /* message not complete yet */
            }

            //qDebug() << "received complete binary message with length" << messageLength;

            _socket.read(lengthBytes, sizeof(lengthBytes)); /* consume the length */
            QByteArray message = _socket.read(messageLength);

            handleBinaryMessage(message);
        }
    }

    void ServerConnection::handleBinaryMessage(QByteArray const& message)
    {
        if (message.length() < 2)
        {
            qDebug() << "received invalid binary message (less than 2 bytes)";
            return; /* invalid message */
        }

        _timeSinceLastMessageReceived.start();
        _keepAliveTimer->stop();
        _keepAliveTimer->start(KeepAliveIntervalMs);

        quint16 messageType = NetworkUtil::get2Bytes(message, 0);
        if (messageType & (1u << 15))
        {
            quint8 extensionMessageType = messageType & 0x7Fu;
            quint8 extensionId = (messageType >> 7) & 0xFFu;

            handleExtensionMessage(extensionId, extensionMessageType, message);
        }
        else
        {
            auto serverMessageType = static_cast<ServerMessageType>(messageType);

            handleStandardBinaryMessage(serverMessageType, message);
        }
    }

    void ServerConnection::handleStandardBinaryMessage(ServerMessageType messageType,
                                                       QByteArray const& message)
    {
        switch (messageType)
        {
        case ServerMessageType::KeepAliveMessage:
            parseKeepAliveMessage(message);
            break;
        case ServerMessageType::ServerExtensionsMessage:
            parseServerProtocolExtensionsMessage(message);
            break;
        case ServerMessageType::ServerEventNotificationMessage:
            parseServerEventNotificationMessage(message);
            break;
        case ServerMessageType::PlayerStateMessage:
            parsePlayerStateMessage(message);
            break;
        case ServerMessageType::VolumeChangedMessage:
            parseVolumeChangedMessage(message);
            break;
        case ServerMessageType::TrackInfoMessage:
            parseTrackInfoMessage(message);
            break;
        case ServerMessageType::BulkTrackInfoMessage:
            parseBulkTrackInfoMessage(message);
            break;
        case ServerMessageType::BulkQueueEntryHashMessage:
            parseBulkQueueEntryHashMessage(message);
            break;
        case ServerMessageType::QueueContentsMessage:
            parseQueueContentsMessage(message);
            break;
        case ServerMessageType::QueueEntryRemovedMessage:
            parseQueueEntryRemovedMessage(message);
            break;
        case ServerMessageType::QueueEntryAddedMessage:
            parseQueueEntryAddedMessage(message);
            break;
        case ServerMessageType::DynamicModeStatusMessage:
            parseDynamicModeStatusMessage(message);
            break;
        case ServerMessageType::PossibleFilenamesForQueueEntryMessage:
            parsePossibleFilenamesForQueueEntryMessage(message);
            break;
        case ServerMessageType::ServerInstanceIdentifierMessage:
            parseServerInstanceIdentifierMessage(message);
            break;
        case ServerMessageType::QueueEntryMovedMessage:
            parseQueueEntryMovedMessage(message);
            break;
        case ServerMessageType::UsersListMessage:
            parseUsersListMessage(message);
            break;
        case ServerMessageType::NewUserAccountSaltMessage:
            parseNewUserAccountSaltMessage(message);
            break;
        case ServerMessageType::SimpleResultMessage:
            parseSimpleResultMessage(message);
            break;
        case ServerMessageType::UserLoginSaltMessage:
            parseUserLoginSaltMessage(message);
            break;
        case ServerMessageType::UserPlayingForModeMessage:
            parseUserPlayingForModeMessage(message);
            break;
        case ServerMessageType::CollectionFetchResponseMessage:
        case ServerMessageType::CollectionChangeNotificationMessage:
            parseTrackInfoBatchMessage(message, messageType);
            break;
        case ServerMessageType::ServerNameMessage:
            parseServerNameMessage(message);
            break;
        case ServerMessageType::HashUserDataMessage:
            parseHashUserDataMessage(message);
            break;
        case ServerMessageType::NewHistoryEntryMessage:
            parseNewHistoryEntryMessage(message);
            break;
        case ServerMessageType::PlayerHistoryMessage:
            parsePlayerHistoryMessage(message);
            break;
        case ServerMessageType::DatabaseIdentifierMessage:
            parseDatabaseIdentifierMessage(message);
            break;
        case ServerMessageType::DynamicModeWaveStatusMessage:
            parseDynamicModeWaveStatusMessage(message);
            break;
        case ServerMessageType::QueueEntryAdditionConfirmationMessage:
            parseQueueEntryAdditionConfirmationMessage(message);
            break;
        case ServerMessageType::ServerHealthMessage:
            parseServerHealthMessage(message);
            break;
        case ServerMessageType::CollectionAvailabilityChangeNotificationMessage:
            parseTrackAvailabilityChangeBatchMessage(message);
            break;
        case ServerMessageType::ServerClockMessage:
            parseServerClockMessage(message);
            break;
        case ServerMessageType::CompatibilityInterfaceAnnouncement:
            parseCompatibilityInterfaceAnnouncement(message);
            break;
        case ServerMessageType::CompatibilityInterfaceLanguageSelectionConfirmation:
            parseCompatibilityInterfaceLanguageSelectionConfirmation(message);
            break;
        case ServerMessageType::CompatibilityInterfaceDefinition:
            parseCompatibilityInterfaceDefinition(message);
            break;
        case ServerMessageType::CompatibilityInterfaceStateUpdate:
            parseCompatibilityInterfaceStateUpdate(message);
            break;
        case ServerMessageType::CompatibilityInterfaceActionStateUpdate:
            parseCompatibilityInterfaceActionStateUpdate(message);
            break;
        case ServerMessageType::CompatibilityInterfaceTextUpdate:
            parseCompatibilityInterfaceTextUpdate(message);
            break;
        case ServerMessageType::CompatibilityInterfaceActionTextUpdate:
            parseCompatibilityInterfaceActionTextUpdate(message);
            break;
        default:
            qDebug() << "received unknown binary message type"
                     << static_cast<int>(messageType)
                     << "with length" << message.length();
            break; /* unknown message type */
        }
    }

    void ServerConnection::handleExtensionMessage(quint8 extensionId, quint8 messageType,
                                                  QByteArray const& message)
    {
        /* parse extensions here */

        //if (extensionId == _knownExtensionOther.id) {
        //    switch (messageType) {
        //    case 1: parseExtensionMessage1(message); break;
        //    case 2: parseExtensionMessage2(message); break;
        //    case 3: parseExtensionMessage3(message); break;
        //    }
        //}

        qWarning() << "unhandled extension message" << messageType
                   << "for extension" << extensionId
                   << "with length" << message.length()
                   << "; extension name: "
                   << _serverExtensionNames.value(extensionId, "?");
    }

    void ServerConnection::parseKeepAliveMessage(QByteArray const& message)
    {
        if (message.length() != 4)
            return; /* invalid message */

        quint16 payload = NetworkUtil::get2Bytes(message, 2);

        qDebug() << "received keep-alive message from the server; payload="
                 << QString::number(payload, 16) << "(hex)";

        /* nothing needs to be done with this message, just receiving it was sufficient */
    }

    void ServerConnection::parseSimpleResultMessage(QByteArray const& message)
    {
        if (message.length() < 12) {
            return; /* invalid message */
        }

        quint16 errorCode = NetworkUtil::get2Bytes(message, 2);
        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
        quint32 intData = NetworkUtil::get4Bytes(message, 8);

        QByteArray blobData = message.mid(12);

        qDebug() << "received result/error message; errorCode:" << errorCode
                 << " client-ref:" << clientReference;

        handleResultMessage(errorCode, clientReference, intData, blobData);
    }

    void ServerConnection::parseServerProtocolExtensionsMessage(QByteArray const& message)
    {
        if (message.length() < 4) {
            return; /* invalid message */
        }

        /* be strict about reserved space */
        int filler = NetworkUtil::getByteUnsignedToInt(message, 2);
        if (filler != 0) {
            return; /* invalid message */
        }

        int extensionCount = NetworkUtil::getByteUnsignedToInt(message, 3);
        if (message.length() < 4 + extensionCount * 4) {
            return; /* invalid message */
        }

        QVector<NetworkProtocol::ProtocolExtension> extensions;
        QSet<quint8> ids;
        QSet<QString> names;
        extensions.reserve(extensionCount);
        ids.reserve(extensionCount);
        names.reserve(extensionCount);

        int offset = 4;
        for (int i = 0; i < extensionCount; ++i) {
            if (offset > message.length() - 3) {
                return; /* invalid message */
            }

            quint8 id = NetworkUtil::getByte(message, offset);
            quint8 version = NetworkUtil::getByte(message, offset + 1);
            quint8 byteCount = NetworkUtil::getByte(message, offset + 2);
            offset += 3;

            if (id == 0 || version == 0 || byteCount == 0
                    || offset > message.length() - byteCount)
            {
                return; /* invalid message */
            }

            QString name = NetworkUtil::getUtf8String(message, offset, byteCount);
            offset += byteCount;

            if (ids.contains(id) || names.contains(name)) {
                return; /* invalid message */
            }

            ids << id;
            names << name;
            extensions << NetworkProtocol::ProtocolExtension(id, name, version);
        }

        if (offset != message.length()) {
            return; /* invalid message */
        }

        registerServerProtocolExtensions(extensions);
    }

    void ServerConnection::parseServerEventNotificationMessage(QByteArray const& message)
    {
        if (message.length() != 4)
            return; /* invalid message */

        quint8 numericEventCode = NetworkUtil::getByte(message, 2);
        quint8 eventArg = NetworkUtil::getByte(message, 3);

        qDebug() << "received server event" << numericEventCode << "with arg" << eventArg;

        auto eventCode = static_cast<ServerEventCode>(numericEventCode);

        handleServerEvent(eventCode);
    }

    void ServerConnection::parseServerInstanceIdentifierMessage(QByteArray const& message)
    {
        if (message.length() != (2 + 16)) return; /* invalid message */

        QUuid uuid = QUuid::fromRfc4122(message.mid(2));
        qDebug() << "received server instance identifier:" << uuid;

        Q_EMIT receivedServerInstanceIdentifier(uuid);
    }

    void ServerConnection::parseServerNameMessage(QByteArray const& message)
    {
        if (message.length() < 4)
        {
            qWarning() << "invalid message; too short";
            return; /* invalid message */
        }

        quint8 nameType = NetworkUtil::getByte(message, 3);
        QString name = NetworkUtil::getUtf8String(message, 4, message.length() - 4);

        qDebug() << "received server name; type:" << nameType << " name:" << name;
        Q_EMIT receivedServerName(nameType, name);
    }

    void ServerConnection::parseDatabaseIdentifierMessage(QByteArray const& message)
    {
        if (message.length() != (2 + 16)) {
            qWarning() << "invalid message; length incorrect";
            return;
        }

        QUuid uuid = QUuid::fromRfc4122(message.mid(2));
        qDebug() << "received database identifier:" << uuid;

        Q_EMIT receivedDatabaseIdentifier(uuid);
    }

    void ServerConnection::parseServerHealthMessage(QByteArray const& message)
    {
        if (message.length() != 4)
        {
            qWarning() << "invalid message; length incorrect";
            return;
        }

        quint16 problems = NetworkUtil::get2Bytes(message, 2);

        qDebug() << "received server health message; problems=" << problems;

        bool databaseUnavailable = problems & 1u;

        ServerHealthStatus newServerHealthStatus(databaseUnavailable);

        /* server health messages cannot be re-requested by the client, so we need to
           store the information in the connection */
        _serverHealthStatus = newServerHealthStatus;

        Q_EMIT serverHealthReceived();
    }

    void ServerConnection::parseServerClockMessage(const QByteArray& message)
    {
        if (message.length() != 12)
        {
            qWarning() << "invalid message; length incorrect";
            return;
        }

        qint64 msSinceEpoch = NetworkUtil::get8BytesSigned(message, 4);

        auto serverClockTime = QDateTime::fromMSecsSinceEpoch(msSinceEpoch, Qt::UTC);

        auto clientClockTimeOffsetMs =
                serverClockTime.msecsTo(QDateTime::currentDateTimeUtc());

        qDebug() << "received server clock time message with value" << msSinceEpoch
                 << ";" << serverClockTime.toString(Qt::ISODateWithMs)
                 << "; client offset:" << clientClockTimeOffsetMs << "ms";

        const auto twoHoursMs = 2 * 60 * 60 * 1000;

        if (clientClockTimeOffsetMs > twoHoursMs || clientClockTimeOffsetMs < -twoHoursMs)
        {
            qWarning() << "client and server clock are more than two hours apart!";
        }

        Q_EMIT receivedClientClockTimeOffset(clientClockTimeOffsetMs);
    }

    void ServerConnection::parseUsersListMessage(QByteArray const& message)
    {
        if (message.length() < 4) {
            return; /* invalid message */
        }

        QList<QPair<uint, QString> > users;

        int userCount = NetworkUtil::get2BytesUnsignedToInt(message, 2);

        qDebug() << "received user account list; count:" << userCount;
        qDebug() << " message length=" << message.length();

        int offset = 4;
        for (int i = 0; i < userCount; ++i) {
            if (message.length() - offset < 5) {
                return; /* invalid message */
            }

            quint32 userId = NetworkUtil::get4Bytes(message, offset);
            offset += 4;
            int loginNameByteCount =
                    NetworkUtil::getByteUnsignedToInt(message, offset);
            offset += 1;
            if (message.length() - offset < loginNameByteCount) {
                return; /* invalid message */
            }

            QString login =
                NetworkUtil::getUtf8String(message, offset, loginNameByteCount);
            offset += loginNameByteCount;

            users.append(QPair<uint, QString>(userId, login));
        }

        if (offset != message.length())
            return; /* invalid message */

        Q_EMIT userAccountsReceived(users);
    }

    void ServerConnection::parseNewUserAccountSaltMessage(QByteArray const& message)
    {
        if (message.length() < 4) {
            return; /* invalid message */
        }

        int loginBytesSize = NetworkUtil::getByteUnsignedToInt(message, 2);
        int saltBytesSize = NetworkUtil::getByteUnsignedToInt(message, 3);

        if (message.length() != 4 + loginBytesSize + saltBytesSize) {
            return; /* invalid message */
        }

        qDebug() << "received salt for new user account";

        QString login = NetworkUtil::getUtf8String(message, 4, loginBytesSize);
        QByteArray salt = message.mid(4 + loginBytesSize, saltBytesSize);

        handleNewUserSalt(login, salt);
    }

    void ServerConnection::parseUserLoginSaltMessage(QByteArray const& message)
    {
        if (message.length() < 8) {
            return; /* invalid message */
        }

        int loginBytesSize = NetworkUtil::getByteUnsignedToInt(message, 4);
        int userSaltBytesSize = NetworkUtil::getByteUnsignedToInt(message, 5);
        int sessionSaltBytesSize = NetworkUtil::getByteUnsignedToInt(message, 6);

        if (message.length() !=
                8 + loginBytesSize + userSaltBytesSize + sessionSaltBytesSize)
        {
            return; /* invalid message */
        }

        QString login = NetworkUtil::getUtf8String(message, 8, loginBytesSize);
        QByteArray userSalt = message.mid(8 + loginBytesSize, userSaltBytesSize);
        QByteArray sessionSalt =
            message.mid(8 + loginBytesSize + userSaltBytesSize, sessionSaltBytesSize);

        handleLoginSalt(login, userSalt, sessionSalt);
    }

    void ServerConnection::parsePlayerStateMessage(QByteArray const& message)
    {
        if (message.length() != 20)
            return; /* invalid message */

        quint8 playerState = NetworkUtil::getByte(message, 2);
        quint8 volume = NetworkUtil::getByte(message, 3);
        qint32 queueLength = NetworkUtil::get4BytesSigned(message, 4);
        quint32 queueId = NetworkUtil::get4Bytes(message, 8);
        quint64 position = NetworkUtil::get8Bytes(message, 12);

        if (queueLength < 0)
            return; /* invalid message */

        //qDebug() << "received player state message";

        // TODO : rename volumeChanged signal or get rid of it
        if (volume <= 100) { Q_EMIT volumeChanged(volume); }

        bool delayedStartActive = false;
        if (_serverProtocolNo >= 20)
        {
            delayedStartActive = (playerState & 128) != 0;
            playerState &= 63;
        }

        auto state = PlayerState::Unknown;
        switch (playerState)
        {
            case 1:
                state = PlayerState::Stopped;
                break;
            case 2:
                state = PlayerState::Playing;
                break;
            case 3:
                state = PlayerState::Paused;
                break;
            default:
                qWarning() << "received unknown player state:" << playerState;
                break;
        }

        Q_EMIT receivedPlayerState(state, volume, queueLength, queueId, position,
                                   delayedStartActive);
    }

    void ServerConnection::parseVolumeChangedMessage(QByteArray const& message)
    {
        if (message.length() != 3) {
            return; /* invalid message */
        }

        quint8 volume = NetworkUtil::getByte(message, 2);

        qDebug() << "received volume changed event;  volume:" << volume;

        if (volume <= 100) { Q_EMIT volumeChanged(volume); }
    }

    void ServerConnection::parseUserPlayingForModeMessage(QByteArray const& message)
    {
        if (message.length() < 8) {
            return; /* invalid message */
        }

        int loginBytesSize = NetworkUtil::getByteUnsignedToInt(message, 2);
        quint32 userId = NetworkUtil::get4Bytes(message, 4);

        if (message.length() != 8 + loginBytesSize) {
            return; /* invalid message */
        }

        QString login = NetworkUtil::getUtf8String(message, 8, loginBytesSize);

        qDebug() << "received user playing for: id =" << userId
                 << "; login =" << login;

        Q_EMIT receivedUserPlayingFor(userId, login);
    }

    void ServerConnection::parseQueueContentsMessage(QByteArray const& message)
    {
        if (message.length() < 10)
            return; /* invalid message */

        qint32 queueLength = NetworkUtil::get4BytesSigned(message, 2);
        qint32 startOffset = NetworkUtil::get4BytesSigned(message, 6);

        if (queueLength < 0 || startOffset < 0)
            return; /* invalid message */

        QList<quint32> queueIDs;
        queueIDs.reserve((message.length() - 10) / 4);

        for (int offset = 10; offset <= message.length() - 4; offset += 4)
        {
            queueIDs.append(NetworkUtil::get4Bytes(message, offset));
        }

        if (queueLength - queueIDs.size() < startOffset)
            return; /* invalid message */

        qDebug() << "received queue contents;  Q-length:" << queueLength
                 << " offset:" << startOffset << " count:" << queueIDs.size();

        Q_EMIT receivedQueueContents(queueLength, startOffset, queueIDs);
    }

    void ServerConnection::parseTrackInfoMessage(QByteArray const& message)
    {
        bool preciseLength = _serverProtocolNo >= 13;

        if (message.length() < 12 + (preciseLength ? 8 : 4)) {
            return; /* invalid message */
        }

        quint16 status = NetworkUtil::get2Bytes(message, 2);
        quint32 queueId = NetworkUtil::get4Bytes(message, 4);

        qint64 lengthMilliseconds;
        int offset = 8;
        if (preciseLength) {
            lengthMilliseconds = NetworkUtil::get8BytesSigned(message, 8);
            offset += 8;
        }
        else {
            lengthMilliseconds = NetworkUtil::get4BytesSigned(message, 8);
            if (lengthMilliseconds > 0) lengthMilliseconds *= 1000;
            offset += 4;
        }

        int titleSize = NetworkUtil::get2BytesUnsignedToInt(message, offset);
        int artistSize = NetworkUtil::get2BytesUnsignedToInt(message, offset + 2);
        offset += 4;

        qDebug() << "received queue track info message; QID:" << queueId
                 << "; status:" << status
                 << "; length (ms):" << lengthMilliseconds;

        if (queueId == 0)
        {
            return; /* invalid message */
        }

        if (message.length() != offset + titleSize + artistSize)
        {
            return; /* invalid message */
        }

        QueueEntryType type = NetworkProtocol::trackStatusToQueueEntryType(status);

        QString title, artist;
        if (NetworkProtocol::isTrackStatusFromRealTrack(status))
        {
            title = NetworkUtil::getUtf8String(message, offset, titleSize);
            artist = NetworkUtil::getUtf8String(message, offset + titleSize, artistSize);
        }
        else
        {
            title = artist = NetworkProtocol::getPseudoTrackStatusText(status);
        }

        qDebug() << "received track info reply;  QID:" << queueId
                 << " type:" << type << " milliseconds:" << lengthMilliseconds
                 << " title:" << title << " artist:" << artist;

        Q_EMIT receivedTrackInfo(queueId, type, lengthMilliseconds, title, artist);
    }

    void ServerConnection::parseBulkTrackInfoMessage(QByteArray const& message)
    {
        if (message.length() < 4) {
            return; /* invalid message */
        }

        bool preciseLength = _serverProtocolNo >= 13;
        int trackInfoBlockByteCount = preciseLength ? 16 : 12;

        int trackCount = NetworkUtil::get2BytesUnsignedToInt(message, 2);
        int statusBlockCount = trackCount + trackCount % 2;
        if (trackCount == 0
            || message.length() <
                4 + statusBlockCount * 2 + trackCount * trackInfoBlockByteCount)
        {
            return; /* irrelevant or invalid message */
        }

        qDebug() << "received queue track info message; track count:" << trackCount;

        int offset = 4;
        QList<quint16> statuses;
        for (int i = 0; i < trackCount; ++i) {
            statuses.append(NetworkUtil::get2Bytes(message, offset));
            offset += 2;
        }
        if (trackCount % 2 != 0) {
            offset += 2; /* skip filler */
        }

        QList<int> offsets;
        while (true) {
            offsets.append(offset);

            quint32 queueID = NetworkUtil::get4Bytes(message, offset);
            offset += 4;
            offset += preciseLength ? 8 : 4; /* length field */
            int titleSize = NetworkUtil::get2BytesUnsignedToInt(message, offset);
            int artistSize = NetworkUtil::get2BytesUnsignedToInt(message, offset + 2);
            offset += 4;
            int titleArtistOffset = offset;

            if (queueID == 0) {
                return; /* invalid message */
            }

            if (titleSize > message.length() - titleArtistOffset
                || artistSize > message.length() - titleArtistOffset
                || (titleSize + artistSize) > message.length() - titleArtistOffset)
            {
                return; /* invalid message */
            }

            offset += titleSize + artistSize;

            if (offset == message.length()) {
                break; /* end of message */
            }

            /* at least one more track info follows */

            if (offset + trackInfoBlockByteCount > message.length()) {
                return;  /* invalid message */
            }
        }

        qDebug() << "received bulk track info reply;  count:" << trackCount;

        if (trackCount != offsets.size()) {
            return;  /* invalid message */
        }

        /* now read all track info's */
        for (int i = 0; i < trackCount; ++i) {
            offset = offsets[i];
            quint32 queueID = NetworkUtil::get4Bytes(message, offset);
            offset += 4;
            quint16 status = statuses[i];

            qint64 lengthMilliseconds;
            if (preciseLength) {
                lengthMilliseconds = NetworkUtil::get8BytesSigned(message, offset);
                offset += 8;
            }
            else {
                lengthMilliseconds = NetworkUtil::get4BytesSigned(message, offset);
                if (lengthMilliseconds > 0) lengthMilliseconds *= 1000;
                offset += 4;
            }

            int titleSize = NetworkUtil::get2BytesUnsignedToInt(message, offset);
            int artistSize = NetworkUtil::get2BytesUnsignedToInt(message, offset + 2);
            offset += 4;

            auto type = NetworkProtocol::trackStatusToQueueEntryType(status);

            QString title, artist;
            if (NetworkProtocol::isTrackStatusFromRealTrack(status)) {
                title = NetworkUtil::getUtf8String(message, offset, titleSize);
                artist =
                    NetworkUtil::getUtf8String(message, offset + titleSize, artistSize);
            }
            else {
                title = artist = NetworkProtocol::getPseudoTrackStatusText(status);
            }

            Q_EMIT receivedTrackInfo(queueID, type, lengthMilliseconds, title, artist);
        }
    }

    void ServerConnection::parsePossibleFilenamesForQueueEntryMessage(
                                                                QByteArray const& message)
    {
        if (message.length() < 6) return; /* invalid message */

        quint32 queueID = NetworkUtil::get4Bytes(message, 2);

        QList<QString> names;
        int offset = 6;
        while (offset < message.length()) {
            if (offset > message.length() - 4) return; /* invalid message */
            int nameLength = NetworkUtil::get4BytesSigned(message, offset);
            if (nameLength <= 0) return; /* invalid message */
            offset += 4;
            if (nameLength + offset > message.length()) return; /* invalid message */
            QString name = NetworkUtil::getUtf8String(message, offset, nameLength);
            offset += nameLength;
            names.append(name);
        }

        qDebug() << "received a list of" << names.size()
                 << "possible filenames for QID" << queueID;

        if (names.size() == 1) {
            qDebug() << " received name" << names[0];
        }

        Q_EMIT receivedPossibleFilenames(queueID, names);
    }

    void ServerConnection::parseBulkQueueEntryHashMessage(const QByteArray& message)
    {
        qint32 messageLength = message.length();
        if (messageLength < 4) {
            invalidMessageReceived(message, "bulk-queue-entry-hashes");
            return; /* invalid message */
        }

        int trackCount = NetworkUtil::get2BytesUnsignedToInt(message, 2);
        if (trackCount == 0
            || messageLength
                != 4 + trackCount * (8 + NetworkProtocol::FILEHASH_BYTECOUNT))
        {
            invalidMessageReceived(
                message, "bulk-queue-entry-hashes",
                "track count=" + QString::number(trackCount)
            );
            return; /* irrelevant or invalid message */
        }

        qDebug() << "received bulk queue entry hash message; count:" << trackCount;

        int offset = 4;
        for (int i = 0; i < trackCount; ++i) {
            quint32 queueID = NetworkUtil::get4Bytes(message, offset);
            quint16 status = NetworkUtil::get2Bytes(message, offset + 4);
            offset += 8;

            bool ok;
            FileHash hash = NetworkProtocol::getHash(message, offset, &ok);
            offset += NetworkProtocol::FILEHASH_BYTECOUNT;
            if (!ok) {
                qWarning() << "could not extract hash for QID" << queueID
                           << "; track status=" << status;
                continue;
            }

            auto type = NetworkProtocol::trackStatusToQueueEntryType(status);

            Q_EMIT receivedQueueEntryHash(queueID, type, hash);
        }
    }

    void ServerConnection::parseQueueEntryAddedMessage(QByteArray const& message)
    {
        if (message.length() != 10)
        {
            return; /* invalid message */
        }

        qint32 offset = NetworkUtil::get4BytesSigned(message, 2);
        quint32 queueID = NetworkUtil::get4Bytes(message, 6);

        qDebug() << "received queue track insertion event;  QID:" << queueID
                 << " offset:" << offset;

        if (offset < 0)
        {
            return; /* invalid message */
        }

        Q_EMIT queueEntryAdded(offset, queueID, RequestID());
    }

    void ServerConnection::parseQueueEntryAdditionConfirmationMessage(
                                                                QByteArray const& message)
    {
        if (message.length() != 16)
        {
            qWarning() << "invalid message; length incorrect";
            return;
        }

        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
        qint32 index = NetworkUtil::get4BytesSigned(message, 8);
        quint32 queueID = NetworkUtil::get4Bytes(message, 12);

        if (index < 0)
        {
            qWarning() << "invalid queue addition confirmation message: index < 0";
            return;
        }

        auto resultHandler = _resultHandlers.take(clientReference);
        if (resultHandler)
        {
            resultHandler->handleQueueEntryAdditionConfirmation(
                                                         clientReference, index, queueID);
            delete resultHandler;
        }
        else
        {
            qWarning() << "no result handler found for reference" << clientReference;
            Q_EMIT queueEntryAdded(index, queueID, RequestID(clientReference));
        }
    }

    void ServerConnection::parseQueueEntryRemovedMessage(QByteArray const& message)
    {
        if (message.length() != 10)
        {
            return; /* invalid message */
        }

        qint32 offset = NetworkUtil::get4BytesSigned(message, 2);
        quint32 queueId = NetworkUtil::get4Bytes(message, 6);

        qDebug() << "received queue track removal event;  QID:" << queueId
                 << " offset:" << offset;

        if (offset < 0)
        {
            return; /* invalid message */
        }

        Q_EMIT queueEntryRemoved(offset, queueId);
    }

    void ServerConnection::parseQueueEntryMovedMessage(QByteArray const& message)
    {
        if (message.length() != 14)
        {
            return; /* invalid message */
        }

        qint32 fromOffset = NetworkUtil::get4BytesSigned(message, 2);
        qint32 toOffset = NetworkUtil::get4BytesSigned(message, 6);
        quint32 queueId = NetworkUtil::get4Bytes(message, 10);

        qDebug() << "received queue track moved event;  QID:" << queueId
                 << " from-offset:" << fromOffset << " to-offset:" << toOffset;

        if (fromOffset < 0 || toOffset < 0)
        {
            return; /* invalid message */
        }

        Q_EMIT queueEntryMoved(fromOffset, toOffset, queueId);
    }

    void ServerConnection::parseDynamicModeStatusMessage(QByteArray const& message)
    {
        if (message.length() != 7) return; /* invalid message */

        quint8 isEnabled = NetworkUtil::getByte(message, 2);
        int noRepetitionSpanSeconds = NetworkUtil::get4BytesSigned(message, 3);

        if (noRepetitionSpanSeconds < 0) return; /* invalid message */

        qDebug() << "received dynamic mode status:" << (isEnabled > 0 ? "ON" : "OFF");

        Q_EMIT dynamicModeStatusReceived(isEnabled > 0, noRepetitionSpanSeconds);
    }

    void ServerConnection::parseDynamicModeWaveStatusMessage(QByteArray const& message)
    {
        qDebug() << "parsing dynamic mode wave status message";

        auto expectedMessageLength = _serverProtocolNo >= 14 ? 12 : 8;

        if (message.length() != expectedMessageLength)
        {
            invalidMessageReceived(message, "dynamic-mode-wave-status",
                                   "wrong message length");
            return;
        }

        auto statusByte = NetworkUtil::getByte(message, 3);
        if (!Common::isValidStartStopEventStatus(statusByte))
        {
            invalidMessageReceived(
                message, "dynamic-mode-wave-status",
                "invalid status value: " + QString::number(statusByte)
            );
            return;
        }

        /* we have no use for the user yet */
        //auto user = NetworkUtil::get4Bytes(message, 4);

        auto status = StartStopEventStatus(statusByte);
        bool statusActive = Common::isActive(status);
        bool statusChanged = Common::isChange(status);

        int progress = -1;
        int progressTotal = -1;
        if (_serverProtocolNo >= 14)
        {
            progress = NetworkUtil::get2BytesSigned(message, 8);
            progressTotal = NetworkUtil::get2BytesSigned(message, 10);
        }

        Q_EMIT dynamicModeHighScoreWaveStatusReceived(statusActive, statusChanged,
                                                      progress, progressTotal);
    }

    void ServerConnection::parseTrackAvailabilityChangeBatchMessage(
                                                                QByteArray const& message)
    {
        qint32 messageLength = message.length();
        if (messageLength < 8) {
            qDebug() << "invalid message detected: length is too short";
            return; /* invalid message */
        }

        int availableCount = int(uint(NetworkUtil::get2Bytes(message, 4)));
        int unavailableCount = int(uint(NetworkUtil::get2Bytes(message, 6)));

        int expectedMessageLength =
            8 + (availableCount + unavailableCount) * NetworkProtocol::FILEHASH_BYTECOUNT;

        if (messageLength != expectedMessageLength) {
            qDebug() << "invalid message detected: length does not match expected length";
            return; /* invalid message */
        }

        int offset = 8;

        QVector<FileHash> available;
        available.reserve(availableCount);
        for (int i = 0; i < availableCount; ++i) {
            bool ok;
            FileHash hash = NetworkProtocol::getHash(message, offset, &ok);
            if (!ok) {
                qDebug() << "invalid message detected: did not read hash correctly;"
                         << "  ok=" << (ok ? "true" : "false");
                return; /* invalid message */
            }

            offset += NetworkProtocol::FILEHASH_BYTECOUNT;

            /* workaround for server bug; we shouldn't be receiving these */
            if (hash.length() == 0) continue;

            available.append(hash);
        }

        QVector<FileHash> unavailable;
        unavailable.reserve(unavailableCount);
        for (int i = 0; i < unavailableCount; ++i) {
            bool ok;
            FileHash hash = NetworkProtocol::getHash(message, offset, &ok);
            if (!ok) {
                qDebug() << "invalid message detected: did not read hash correctly;"
                         << "  ok=" << (ok ? "true" : "false");
                return; /* invalid message */
            }

            offset += NetworkProtocol::FILEHASH_BYTECOUNT;

            /* workaround for server bug; we shouldn't be receiving these */
            if (hash.length() == 0) continue;

            unavailable.append(hash);
        }

        qDebug() << "got track availability changes: " << available.size() << "available,"
                 << unavailable.size() << "unavailable";

        if (available.empty() && unavailable.empty())
            return;

        if (available.size() <= 3) {
            for (auto const& hash : available) {
                qDebug() << " available:" << hash;
            }
        }
        if (unavailable.size() <= 3) {
            for (auto const& hash : unavailable) {
                qDebug() << " unavailable:" << hash;
            }
        }

        Q_EMIT collectionTracksAvailabilityChanged(available, unavailable);
    }

    void ServerConnection::parseTrackInfoBatchMessage(QByteArray const& message,
                                                      ServerMessageType messageType)
    {
        qint32 messageLength = message.length();
        if (messageLength < 4) {
            return; /* invalid message */
        }

        bool isNotification =
            messageType == ServerMessageType::CollectionChangeNotificationMessage;

        int offset = isNotification ? 4 : 8;

        bool withAlbumAndTrackLength = _serverProtocolNo >= 7;
        const int fixedInfoLengthPerTrack =
            NetworkProtocol::FILEHASH_BYTECOUNT + 1 + 2 + 2
                + (withAlbumAndTrackLength ? 2 + 4 : 0);

        int trackCount = NetworkUtil::get2BytesUnsignedToInt(message, 2);
        if (trackCount == 0 || messageLength < offset + fixedInfoLengthPerTrack)
        {
            return; /* irrelevant or invalid message */
        }

        CollectionFetcher* collectionFetcher = nullptr;
        if (!isNotification) {
            quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
            collectionFetcher = _collectionFetchers.value(clientReference, nullptr);
            if (!collectionFetcher) {
                return; /* irrelevant or invalid message */
            }
        }

        QList<int> offsets;
        offsets.append(offset);

        while (true) {
            /* set pointer past hash and availability */
            int current = offset + NetworkProtocol::FILEHASH_BYTECOUNT + 1;
            int titleSize = NetworkUtil::get2BytesUnsignedToInt(message, current);
            current += 2;
            int artistSize = NetworkUtil::get2BytesUnsignedToInt(message, current);
            current += 2;
            int albumSize = 0;
            //qint32 trackLength = 0;
            if (withAlbumAndTrackLength) {
                albumSize = NetworkUtil::get2BytesUnsignedToInt(message, current);
                current += 2;
                //trackLength = (qint32)NetworkUtil::get4Bytes(message, current);
                current += 4;
            }

            if (titleSize > messageLength - current
                || artistSize > messageLength - current
                || albumSize > messageLength - current
                || (titleSize + artistSize + albumSize) > messageLength - current)
            {
                return; /* invalid message */
            }

            if (current + titleSize + artistSize + albumSize == messageLength) {
                break; /* end of message */
            }

            /* at least one more track info follows */

            /* offset for next track */
            offset = current + titleSize + artistSize + albumSize;

            if (offset + fixedInfoLengthPerTrack > messageLength)
            {
                return;  /* invalid message */
            }

            offsets.append(offset);
        }

        qDebug() << "received collection track info message;  track count:" << trackCount
                 << "; notification?" << (isNotification ? "Y" : "N")
                 << "; with album & length?" << (withAlbumAndTrackLength ? "Y" : "N");

        if (trackCount != offsets.size()) {
            qDebug() << " invalid message detected: offsets size:" << offsets.size();
            return; /* invalid message */
        }

        /* now read all track info's */
        QVector<CollectionTrackInfo> infos;
        infos.reserve(trackCount);
        for (int i = 0; i < trackCount; ++i) {
            offset = offsets[i];

            bool ok;
            FileHash hash = NetworkProtocol::getHash(message, offset, &ok);
            if (!ok) {
                qDebug() << " invalid message detected: did not read hash correctly;"
                         << "  ok=" << (ok ? "true" : "false");
                return; /* invalid message */
            }
            offset += NetworkProtocol::FILEHASH_BYTECOUNT;

            quint8 availabilityByte = NetworkUtil::getByte(message, offset);
            int titleSize = NetworkUtil::get2BytesUnsignedToInt(message, offset + 1);
            int artistSize = NetworkUtil::get2BytesUnsignedToInt(message, offset + 3);
            offset += 5;
            int albumSize = 0;
            qint32 trackLengthInMs = -1;
            if (withAlbumAndTrackLength) {
                albumSize = NetworkUtil::get2BytesUnsignedToInt(message, offset);
                trackLengthInMs = NetworkUtil::get4BytesSigned(message, offset + 2);
                offset += 6;
            }

            QString title = NetworkUtil::getUtf8String(message, offset, titleSize);
            offset += titleSize;
            QString artist = NetworkUtil::getUtf8String(message, offset, artistSize);
            offset += artistSize;
            QString album = "";
            if (withAlbumAndTrackLength) {
                album = NetworkUtil::getUtf8String(message, offset, albumSize);
            }

            /* workaround for server bug; we shouldn't be receiving these */
            if (hash.length() == 0) continue;

            CollectionTrackInfo info(hash, availabilityByte & 1, title, artist, album,
                                     trackLengthInMs);
            infos.append(info);
        }

        if (infos.empty()) return;

        if (infos.size() <= 3) {
            for (auto const& info : infos) {
                auto length = info.lengthInMilliseconds();
                qDebug() << " track: hash:" << info.hash() << "; title:" << info.title()
                         << "; artist:" << info.artist()
                         << "; length:"
                         << Util::millisecondsToShortDisplayTimeText(length)
                         << "; available:" << info.isAvailable();
            }
        }

        if (isNotification) {
            Q_EMIT collectionTracksChanged(infos);
        }
        else {
            Q_EMIT collectionFetcher->receivedData(infos);
        }
    }

    void ServerConnection::parseHashUserDataMessage(const QByteArray& message)
    {
        int messageLength = message.length();
        if (messageLength < 12) {
            qWarning() << "ServerConnection::parseHashUserDataMessage : invalid msg (1)";
            return; /* invalid message */
        }

        int hashCount = NetworkUtil::get2BytesUnsignedToInt(message, 2);
        quint16 fields = NetworkUtil::get2Bytes(message, 6);
        quint32 userId = NetworkUtil::get4Bytes(message, 8);
        int offset = 12;

        if ((fields & 3) != fields || fields == 0) {
            return; /* has unsupported fields or none */
        }

        bool havePreviouslyHeard = (fields & 1) == 1;
        bool haveScore = (fields & 2) == 2;

        int bytesPerHash =
            NetworkProtocol::FILEHASH_BYTECOUNT
                + (havePreviouslyHeard ? 8 : 0)
                + (haveScore ? 2 : 0);

        if ((messageLength - offset) != hashCount * bytesPerHash) {
            qWarning() << "ServerConnection::parseHashUserDataMessage: invalid msg (2)";
            return; /* invalid message */
        }

        qDebug() << "received hash user data message; count:" << hashCount
                 << "; user:" << userId << "; fields:" << fields;

        bool ok;
        for (int i = 0; i < hashCount; ++i) {
            FileHash hash = NetworkProtocol::getHash(message, offset, &ok);
            if (!ok) return; /* invalid message */
            offset += NetworkProtocol::FILEHASH_BYTECOUNT;

            QDateTime previouslyHeard;
            qint16 score = -1;

            if (havePreviouslyHeard) {
                previouslyHeard =
                    NetworkUtil::getMaybeEmptyQDateTimeFrom8ByteMsSinceEpoch(
                        message, offset
                    );
                offset += 8;
            }

            if (haveScore) {
                score = NetworkUtil::get2BytesSigned(message, offset);
                offset += 2;
            }

            if (hash.isNull()) {
                qWarning() << "received user data for null hash; ignoring";
                continue;
            }

            // TODO: fixme: receiver cannot know which fields were not received now
            Q_EMIT receivedHashUserData(hash, userId, previouslyHeard, score);
        }
    }

    void ServerConnection::parseNewHistoryEntryMessage(const QByteArray& message)
    {
        qDebug() << "parsing player history entry message";

        if (message.length() != 4 + 28) {
            return; /* invalid message */
        }

        quint32 queueID = NetworkUtil::get4Bytes(message, 4);
        quint32 user = NetworkUtil::get4Bytes(message, 8);
        QDateTime started = NetworkUtil::getQDateTimeFrom8ByteMsSinceEpoch(message, 12);
        QDateTime ended = NetworkUtil::getQDateTimeFrom8ByteMsSinceEpoch(message, 20);
        int permillagePlayed = NetworkUtil::get2BytesSigned(message, 28);
        quint16 status = NetworkUtil::get2Bytes(message, 30);

        bool hadError = status & 1;
        bool hadSeek = status & 2;

        qDebug() << "received player history entry:" << " QID:" << queueID
                 << " started:" << started << " ended:" << ended;

        PlayerHistoryTrackInfo info(queueID, user, started, ended, hadError, hadSeek,
                                    permillagePlayed);

        Q_EMIT receivedPlayerHistoryEntry(info);
    }

    void ServerConnection::parsePlayerHistoryMessage(const QByteArray& message)
    {
        qDebug() << "parsing player history list message";

        if (message.length() < 4) {
            return; /* invalid message */
        }

        int entryCount = NetworkUtil::getByteUnsignedToInt(message, 3);

        if (message.length() != 4 + entryCount * 28) {
            return; /* invalid message */
        }

        int offset = 4;
        QVector<PlayerHistoryTrackInfo> entries;
        entries.reserve(entryCount);
        for(int i = 0; i < entryCount; ++i) {
            auto queueID = NetworkUtil::get4Bytes(message, offset);
            auto user = NetworkUtil::get4Bytes(message, offset + 4);
            auto started =
                NetworkUtil::getQDateTimeFrom8ByteMsSinceEpoch(message, offset + 8);
            auto ended =
                NetworkUtil::getQDateTimeFrom8ByteMsSinceEpoch(message, offset + 16);
            int permillagePlayed = NetworkUtil::get2BytesSigned(message, offset + 24);
            quint16 status = NetworkUtil::get2Bytes(message, offset + 26);

            bool hadError = status & 1;
            bool hadSeek = status & 2;

            entries.append(
                PlayerHistoryTrackInfo(
                    queueID, user, started, ended, hadError, hadSeek, permillagePlayed
                )
            );

            offset += 28;
        }

        Q_EMIT receivedPlayerHistory(entries);
    }

    void ServerConnection::handleResultMessage(quint16 errorCode, quint32 clientReference,
                                               quint32 intData,
                                               QByteArray const& blobData)
    {
        auto errorCodeEnum = static_cast<ResultMessageErrorCode>(errorCode);

        if (errorCodeEnum == ResultMessageErrorCode::InvalidMessageStructure)
        {
            qWarning() << "errortype = InvalidMessageStructure !!";
        }

        if (clientReference == _userLoginRef)
        {
            handleUserLoginResult(errorCodeEnum, intData, blobData);
            return;
        }

        if (clientReference == _userAccountRegistrationRef)
        {
            handleUserRegistrationResult(errorCodeEnum, intData, blobData);
            return;
        }

        auto resultHandler = _resultHandlers.take(clientReference);
        if (resultHandler)
        {
            ResultMessageData data(errorCodeEnum, clientReference, intData, blobData);
            resultHandler->handleResult(data);
            delete resultHandler;
            return;
        }

        qWarning() << "error/result message cannot be handled; ref:" << clientReference
                   << " intData:" << intData << "; blobdata-length:" << blobData.size();
    }

    quint32 ServerConnection::registerResultHandler(ResultHandler* handler)
    {
        auto ref = getNewReference();
        _resultHandlers[ref] = handler;
        return ref;
    }

    void ServerConnection::discardResultHandler(quint32 clientReference)
    {
        auto resultHandler = _resultHandlers.take(clientReference);
        if (!resultHandler)
            return;

        delete resultHandler;
    }

    void ServerConnection::invalidMessageReceived(QByteArray const& message,
                                                  QString messageType, QString extraInfo)
    {
        qWarning() << "received invalid message; length=" << message.size()
                   << " type=" << messageType << " extra info=" << extraInfo;
    }

    void ServerConnection::registerServerProtocolExtensions(
                            const QVector<NetworkProtocol::ProtocolExtension>& extensions)
    {
        /* handle extensions here */
        for (auto const& extension : extensions)
        {
            qDebug() << "server will use ID" << extension.id
                     << "and version" << extension.version
                     << "for protocol extension" << extension.name;

            _serverExtensionNames[extension.id] = extension.name;

            //if (extension.name == "known_extension_name") {
            //    _knownExtensionOther = extension;
            //}
        }
    }

    void ServerConnection::handleServerEvent(ServerEventCode eventCode)
    {
        switch (eventCode)
        {
        case ServerEventCode::Reserved:
            break; /* not to be used, treat as invalid */

        case ServerEventCode::FullIndexationRunning:
            onFullIndexationRunningStatusReceived(true);
            return;

        case ServerEventCode::FullIndexationNotRunning:
            onFullIndexationRunningStatusReceived(false);
            return;
        }

        qDebug() << "received unknown server event:" << static_cast<int>(eventCode);
    }

}

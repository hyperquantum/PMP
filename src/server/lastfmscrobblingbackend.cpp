/*
    Copyright (C) 2018-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "lastfmscrobblingbackend.h"

#include "common/async.h"
#include "common/version.h"

#include <algorithm>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDebug>
#include <QDomDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace PMP::Server
{
    LastFmRequestHandler::LastFmRequestHandler(LastFmScrobblingBackend* parent,
                                               QNetworkReply* pendingReply,
                                               QString xmlTagName)
     : QObject(parent), _parent(parent), _reply(pendingReply), _xmlTagName(xmlTagName)
    {
        connect(
            _reply, &QNetworkReply::finished,
            this, &LastFmRequestHandler::requestFinished
        );
    }

    LastFmRequestHandler::~LastFmRequestHandler()
    {
        _reply->deleteLater();
    }

    void LastFmRequestHandler::requestFinished()
    {
        auto error = _reply->error();
        auto replyData = _reply->readAll();
        auto replyText = QString::fromUtf8(replyData);

        if (error != QNetworkReply::NoError)
        {
            qWarning() << "Last.Fm reply has network error " << error
                       << "with error text:" << _reply->errorString();
        }

        qDebug() << "Last.Fm reply consists of" << replyData.size() << "bytes,"
                 << replyText.size() << "characters:\n"
                 << replyText;

        if (error != QNetworkReply::NoError && replyText.size() == 0)
        {
            onNetworkError(error, _reply->errorString());
            return;
        }

        parseReply(replyData);

        /* delete the handler after the reply has been handled */
        deleteLater();
    }

    void LastFmRequestHandler::onNetworkError(QNetworkReply::NetworkError error,
                                              QString errorText)
    {
        /* workaround for "network access is disabled" problem */
        if (error == QNetworkReply::UnknownNetworkError
                && errorText.contains("Network access is disabled"))
        {
            qDebug() << "detected 'Network access is disabled' problem;"
                     << "applying workaround";

            Q_EMIT mustRecreateNetworkManager();
        }

        onGenericError();
    }

    void LastFmRequestHandler::onParseError()
    {
        onGenericError();
    }

    void LastFmRequestHandler::parseReply(QByteArray bytes)
    {
        QDomDocument dom;
        QString xmlParseError;
        int xmlErrorLine;
        if (!dom.setContent(bytes, &xmlParseError, &xmlErrorLine))
        {
            qWarning() << "Could not parse the Last.Fm reply as valid XML;"
                       << "error at line" << xmlErrorLine << ":" << xmlParseError;
            onParseError();
            return;
        }

        auto lfmElement = dom.documentElement();
        if (lfmElement.isNull() || lfmElement.tagName() != "lfm")
        {
            qWarning() << "Last.Fm reply XML does not have <lfm> root element";
            onParseError();
            return;
        }

        auto status = lfmElement.attribute("status");
        if (status != "ok")
        {
            qDebug() << "Last.Fm reply indicates that the request failed";
            auto errorElement = lfmElement.firstChildElement("error");
            if (errorElement.isNull())
            {
                qWarning() << "Last.Fm failure reply has no <error> element";
                onParseError();
            }
            else
            {
                handleErrorReply(errorElement);
            }
            return;
        }

        auto childNode = lfmElement.firstChildElement(_xmlTagName);
        if (childNode.isNull())
        {
            qWarning() << "Last.Fm reply does not have " << _xmlTagName << "element";
            onParseError();
        }
        else
        {
            handleOkReply(childNode);
        }
    }

    void LastFmRequestHandler::handleErrorReply(const QDomElement& errorElement)
    {
        auto errorCodeText = errorElement.attribute("code");
        qDebug() << "received LFM error status;"
                 << "code:" << errorCodeText << ";"
                 << "message:" << errorElement.text();

        bool ok;
        int errorCode = errorCodeText.toInt(&ok);
        if (!ok)
        {
            qWarning() << "could not convert Last.Fm error code to a number";
            onParseError();
            return;
        }

        handleErrorCode(errorCode);
    }

    void LastFmRequestHandler::handleErrorCode(int lastFmErrorCode)
    {
        /* we only handle generic error codes here */

        switch (lastFmErrorCode)
        {
            case 4: /* not an official (documented) error code; we'll just try again */
                qWarning() << "LFM error code" << lastFmErrorCode
                           << ": should try again later";
                Q_EMIT shouldTryAgainLater();
                break;

            case 9: /* invalid session key, need to re-authenticate */
                qWarning() << "LFM reports session key not valid (or not anymore)";
                Q_EMIT mustInvalidateSessionKey();
                break;

            case 8: case 11: case 16:
                /* retry request later */
                qWarning() << "LFM error code" << lastFmErrorCode
                           << ": should try again later";
                Q_EMIT shouldTryAgainLater();
                break;

            case 2: case 3: case 5: case 6: case 7: case 13: case 27:
                qWarning() << "LFM error code" << lastFmErrorCode
                           << ": probably a bug in the request";
                Q_EMIT fatalError();
                break;

            case 10: case 26: /* invalid/suspended API key */
                qWarning() << "LFM error code" << lastFmErrorCode
                           << ": problem with our API key";
                Q_EMIT fatalError();
                break;

            case 29: /* rate limit exceeded */
                qWarning() << "LFM reports rate limit exceeded";
                Q_EMIT shouldTryAgainLater();
                break;

            default:
                qWarning() << "unknown/unhandled LFM error code" << lastFmErrorCode;
                Q_EMIT fatalError();
                break;
        }

        onGenericError();
    }

    /* ============================================================================ */

    LastFmAuthenticationRequestHandler::LastFmAuthenticationRequestHandler(
                                                          LastFmScrobblingBackend* parent,
                                                              QNetworkReply* pendingReply)
     : LastFmRequestHandler(parent, pendingReply, "session"),
        _promise(Async::createPromise<LastFmAuthenticationResult, Result>())
    {
        //
    }

    NewFuture<LastFmAuthenticationResult, Result>
        LastFmAuthenticationRequestHandler::future() const
    {
        return _promise.future();
    }

    void LastFmAuthenticationRequestHandler::handleOkReply(
                                                          const QDomElement& childElement)
    {
        auto nameNode = childElement.firstChildElement("name");
        auto keyNode = childElement.firstChildElement("key");

        if (nameNode.isNull() || keyNode.isNull())
        {
            qWarning() << "Last.Fm session node is missing name or key";
            onParseError();
            return;
        }

        qDebug() << "session.name:" << nameNode.text();
        qDebug() << "session.key:" << keyNode.text();

        LastFmAuthenticationResult result;
        result.username = nameNode.text();
        result.sessionKey = keyNode.text();

        _promise.setResult(result);
    }

    void LastFmAuthenticationRequestHandler::handleErrorCode(int lastFmErrorCode)
    {
        if (lastFmErrorCode == 4) /* authentication failed */
        {
            qDebug() << "LFM authentication failed";
            _promise.setError(Error::scrobblingAuthenticationFailed());
        }
        else
        {
            /* generic error, let the base class handle it */
            LastFmRequestHandler::handleErrorCode(lastFmErrorCode);
        }
    }

    void LastFmAuthenticationRequestHandler::onGenericError()
    {
        _promise.setError(Error::unspecifiedScrobblingBackendError());
    }

    /* ============================================================================ */

    LastFmNowPlayingRequestHandler::LastFmNowPlayingRequestHandler(
                                                          LastFmScrobblingBackend* parent,
                                                              QNetworkReply* pendingReply)
     : LastFmRequestHandler(parent, pendingReply, "nowplaying")
    {
        //
    }

    void LastFmNowPlayingRequestHandler::handleOkReply(const QDomElement& childElement)
    {
        Q_UNUSED(childElement)

        /* don't parse the reply, just assume that it was successful */
        Q_EMIT nowPlayingUpdateSuccessful();
    }

    void LastFmNowPlayingRequestHandler::onGenericError()
    {
        Q_EMIT nowPlayingUpdateFailed();
    }

    /* ============================================================================ */

    LastFmScrobbleRequestHandler::LastFmScrobbleRequestHandler(
                                                          LastFmScrobblingBackend* parent,
                                                              QNetworkReply* pendingReply)
     : LastFmRequestHandler(parent, pendingReply, "scrobbles")
    {
        //
    }

    void LastFmScrobbleRequestHandler::handleOkReply(const QDomElement& childElement)
    {
        /* it should have exactly one "scrobble" element */
        auto scrobbleElement = childElement.firstChildElement("scrobble");
        if (scrobbleElement.isNull()
                || !scrobbleElement.nextSiblingElement("scrobble").isNull())
        {
            Q_EMIT scrobbleError();
            return;
        }

        auto timestampElement = scrobbleElement.firstChildElement("timestamp");
        if (timestampElement.isNull())
        {
            Q_EMIT scrobbleError();
            return;
        }

        bool ok;
        auto timestampNumber = timestampElement.text().toLongLong(&ok);
        if (!ok)
        {
            Q_EMIT scrobbleError();
            return;
        }

        auto ignoredMessageElement =
            scrobbleElement.firstChildElement("ignoredMessage");
        if (ignoredMessageElement.isNull())
        {
            Q_EMIT scrobbleError();
            return;
        }

        auto ignoredReasonText = ignoredMessageElement.text();
        auto ignoredReason = ignoredMessageElement.attribute("code").toInt(&ok);
        if (!ok)
        {
            Q_EMIT scrobbleError();
            return;
        }

        bool scrobbleAccepted = (ignoredReason == 0);
        if (scrobbleAccepted)
            qDebug() << "scrobble was accepted";
        else
            qDebug() << "scrobble NOT accepted for reason code" << ignoredReason
                     << ":" << ignoredReasonText;

        auto titleText = scrobbleElement.firstChildElement("track").text();
        auto artistText = scrobbleElement.firstChildElement("artist").text();
        auto albumText = scrobbleElement.firstChildElement("album").text();
        auto albumArtistText = scrobbleElement.firstChildElement("albumArtist").text();

        qDebug() << "scrobble feedback received:\n"
                 << " timestamp: " << QString::number(timestampNumber) << "\n"
                 << " title:" << titleText << "\n"
                 << " artist:" << artistText << "\n"
                 << " album:" << albumText << "\n"
                 << " album artist:" << albumArtistText;

        if (scrobbleAccepted)
            Q_EMIT scrobbleSuccessful();
        else
            Q_EMIT scrobbleIgnored();
    }

    void LastFmScrobbleRequestHandler::onGenericError()
    {
        Q_EMIT scrobbleError();
    }

    /* ============================================================================ */

    LastFmScrobblingBackend::LastFmScrobblingBackend()
     : ScrobblingBackend(), _networkAccessManager(nullptr)
    {
        qDebug() << "Creating LastFmScrobblingProvider;  user-agent:" << userAgent;
    }

    const char* LastFmScrobblingBackend::apiUrl = "https://ws.audioscrobbler.com/2.0/";
    const char* LastFmScrobblingBackend::apiKey = "fc44ba796d201052f53f92818834f907";
    const char* LastFmScrobblingBackend::apiSecret = "3e58b46e070c34718686e0dfbd02d22f";
    const char* LastFmScrobblingBackend::userAgent =
                    PMP_PRODUCT_NAME " " PMP_VERSION_DISPLAY " (LFM scrobbler v0.4)";
    const char* LastFmScrobblingBackend::contentTypeForPostRequest =
                                                      "application/x-www-form-urlencoded";

    bool LastFmScrobblingBackend::needsSsl() const
    {
        return true;
    }

    void LastFmScrobblingBackend::initialize()
    {
        ScrobblingBackend::initialize();

        leaveState(ScrobblingBackendState::NotInitialized);
    }

    NewSimpleFuture<Result> LastFmScrobblingBackend::authenticateWithCredentials(
                                                                QString usernameOrEmail,
                                                                QString password)
    {
        if (_username != usernameOrEmail)
        {
            _username.clear();
        }

        auto handler = doGetMobileTokenCall(usernameOrEmail, password);

        return
            handler->future()
                .thenOnEventLoop<SuccessType, Result>(
                    this,
                    [this, usernameOrEmail](
                        ResultOrError<LastFmAuthenticationResult, Result> outcome)
                            -> ResultOrError<SuccessType, Result>
                    {
                        if (outcome.failed())
                        {
                            return outcome.error();
                        }

                        auto username = outcome.result().username;
                        auto sessionKey = outcome.result().sessionKey;
                        _username = username;
                        _sessionKey = sessionKey;
                        updateState();
                        Q_EMIT authenticatedSuccessfully(username, sessionKey);
                        return success;
                    }
                )
                .convertToSimpleFuture<Result>(
                    [](SuccessType const&) -> Result { return Success(); },
                    [](Result const& result) { return result; }
                );
    }

    void LastFmScrobblingBackend::setUsername(const QString& username)
    {
        if (_username == username) return; /* no change */

        _username = username;
    }

    void LastFmScrobblingBackend::setSessionKey(const QString& sessionKey)
    {
        if (_sessionKey == sessionKey) return; /* no change */
        _sessionKey = sessionKey;

        updateState();
    }

    QString LastFmScrobblingBackend::username() const
    {
        return _username;
    }

    QString LastFmScrobblingBackend::sessionKey() const
    {
        return _sessionKey;
    }

    LastFmAuthenticationRequestHandler* LastFmScrobblingBackend::doGetMobileTokenCall(
                                                                  QString usernameOrEmail,
                                                                  QString password)
    {
        QVector<QPair<QString, QString>> parameters;
        parameters << QPair<QString, QString>("method", "auth.getMobileSession");
        parameters << QPair<QString, QString>("api_key", apiKey);
        parameters << QPair<QString, QString>("password", password);
        parameters << QPair<QString, QString>("username", usernameOrEmail);

        auto authenticationReply = signAndSendPost(parameters);

        auto handler = new LastFmAuthenticationRequestHandler(this, authenticationReply);
        connectStateHandlingSignals(handler);
        return handler;
    }

    void LastFmScrobblingBackend::updateNowPlaying(ScrobblingTrack track)
    {
        if (state() != ScrobblingBackendState::ReadyForScrobbling)
            return;

        auto handler = doUpdateNowPlayingCall(_sessionKey, track);

        connect(
            handler, &LastFmNowPlayingRequestHandler::nowPlayingUpdateSuccessful,
            this, [this]() { Q_EMIT this->gotNowPlayingResult(true); }
        );
        connect(
            handler, &LastFmNowPlayingRequestHandler::nowPlayingUpdateFailed,
            this, [this]() { Q_EMIT this->gotNowPlayingResult(false); }
        );
    }

    void LastFmScrobblingBackend::scrobbleTrack(QDateTime timestamp,
                                                ScrobblingTrack track)
    {
        if (state() != ScrobblingBackendState::ReadyForScrobbling)
            return;

        auto handler = doScrobbleCall(_sessionKey, timestamp, track);

        connect(
            handler, &LastFmScrobbleRequestHandler::scrobbleSuccessful,
            this, [this]() { Q_EMIT this->gotScrobbleResult(ScrobbleResult::Success); }
        );
        connect(
            handler, &LastFmScrobbleRequestHandler::scrobbleIgnored,
            this, [this]() { Q_EMIT this->gotScrobbleResult(ScrobbleResult::Ignored); }
        );
        connect(
            handler, &LastFmScrobbleRequestHandler::scrobbleError,
            this, [this]() { Q_EMIT this->gotScrobbleResult(ScrobbleResult::Error); }
        );
    }

    LastFmNowPlayingRequestHandler* LastFmScrobblingBackend::doUpdateNowPlayingCall(
                                                                    QString sessionKey,
                                                                    ScrobblingTrack track)
    {
        QVector<QPair<QString, QString>> parameters;
        parameters << QPair<QString, QString>("method", "track.updateNowPlaying");
        if (!track.album.isEmpty())
        {
            parameters << QPair<QString, QString>("album", track.album);
        }
        if (!track.albumArtist.isEmpty() && track.artist != track.albumArtist)
        {
            parameters << QPair<QString, QString>("albumArtist", track.albumArtist);
        }
        parameters << QPair<QString, QString>("api_key", apiKey);
        parameters << QPair<QString, QString>("artist", track.artist);
        if (track.durationInSeconds > 0)
        {
            auto durationText = QString::number(track.durationInSeconds);
            parameters << QPair<QString, QString>("duration", durationText);
        }
        parameters << QPair<QString, QString>("sk", sessionKey);
        parameters << QPair<QString, QString>("track", track.title);

        auto nowPlayingReply = signAndSendPost(parameters);

        auto handler = new LastFmNowPlayingRequestHandler(this, nowPlayingReply);
        connectStateHandlingSignals(handler);
        return handler;
    }

    LastFmScrobbleRequestHandler* LastFmScrobblingBackend::doScrobbleCall(
                                                                    QString sessionKey,
                                                                    QDateTime timestamp,
                                                                    ScrobblingTrack track)
    {
        auto timestampAsUnixTime = timestamp.toSecsSinceEpoch();
        auto timestampText = QString::number(timestampAsUnixTime);

        QVector<QPair<QString, QString>> parameters;
        parameters << QPair<QString, QString>("method", "track.scrobble");
        if (!track.album.isEmpty())
        {
            parameters << QPair<QString, QString>("album", track.album);
        }
        if (!track.albumArtist.isEmpty() && track.artist != track.albumArtist)
        {
            parameters << QPair<QString, QString>("albumArtist", track.albumArtist);
        }
        parameters << QPair<QString, QString>("api_key", apiKey);
        parameters << QPair<QString, QString>("artist", track.artist);
        if (track.durationInSeconds > 0)
        {
            auto durationText = QString::number(track.durationInSeconds);
            parameters << QPair<QString, QString>("duration", durationText);
        }
        parameters << QPair<QString, QString>("sk", sessionKey);
        parameters << QPair<QString, QString>("timestamp", timestampText);
        parameters << QPair<QString, QString>("track", track.title);

        auto scrobbleReply = signAndSendPost(parameters);

        auto handler = new LastFmScrobbleRequestHandler(this, scrobbleReply);
        connectStateHandlingSignals(handler);
        return handler;
    }

    QNetworkReply* LastFmScrobblingBackend::signAndSendPost(
                                              QVector<QPair<QString, QString>> parameters)
    {
        signCall(parameters);

        if (!_networkAccessManager)
        {
            _networkAccessManager = new QNetworkAccessManager(this);
        }

        QUrl url(apiUrl);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, contentTypeForPostRequest);
        request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);

        QUrlQuery query;
        query.setQueryItems(parameters.toList());

        QUrl urlParameters;
        urlParameters.setQuery(query);
        auto parametersString = urlParameters.toEncoded();
        if (parametersString.startsWith('?'))
            parametersString = parametersString.mid(1);
        parametersString.replace('+', "%2B"); /* '+' was not encoded yet */

        qDebug() << "parameters:" << parametersString;
        return _networkAccessManager->post(request, parametersString);
    }

    void LastFmScrobblingBackend::disposeOfNetworkAccessManager()
    {
        qDebug() << "forcing NetworkAccessManager to be recreated next time";
        auto networkAccessManager = _networkAccessManager;
        _networkAccessManager = nullptr;
        if (networkAccessManager) { networkAccessManager->deleteLater(); }
    }

    void LastFmScrobblingBackend::connectStateHandlingSignals(
                                                            LastFmRequestHandler* handler)
    {
        connect(
            handler, &LastFmRequestHandler::mustRecreateNetworkManager,
            this, &LastFmScrobblingBackend::disposeOfNetworkAccessManager
        );
        connect(
            handler, &LastFmRequestHandler::fatalError,
            this,
            [this]() { setState(ScrobblingBackendState::PermanentFatalError); }
        );
        connect(
            handler, &LastFmRequestHandler::shouldTryAgainLater,
            this, &LastFmScrobblingBackend::serviceTemporarilyUnavailable
        );
        connect(
            handler, &LastFmRequestHandler::mustInvalidateSessionKey,
            this,
            [this]()
            {
                _sessionKey = "";
                updateState();
            }
        );
    }

    void LastFmScrobblingBackend::signCall(QVector<QPair<QString, QString>>& parameters)
    {
        std::sort(parameters.begin(), parameters.end());

        QByteArray signData;
        for (auto const& parameter : qAsConst(parameters))
        {
            signData += parameter.first.toUtf8();
            signData += parameter.second.toUtf8();
        }

        signData += apiSecret;

        auto hash = QCryptographicHash::hash(signData, QCryptographicHash::Md5);

        qDebug() << "Last.Fm signature data:" << QString::fromUtf8(signData);
        qDebug() << "Last.Fm signature generated:" << hash.toHex();
        parameters << QPair<QString, QString>("api_sig", hash.toHex());
    }

    void LastFmScrobblingBackend::updateState()
    {
        auto oldState = state();

        switch (oldState)
        {
            case ScrobblingBackendState::NotInitialized:
            case ScrobblingBackendState::PermanentFatalError:
                /* these states need to be switched away from explicitly */
                return;

            default:
                break;
        }

        leaveState(oldState);
    }

    void LastFmScrobblingBackend::leaveState(ScrobblingBackendState oldState)
    {
        if (state() != oldState)
            return;

        if (!_sessionKey.isEmpty())
        {
            setState(ScrobblingBackendState::ReadyForScrobbling);
            return;
        }

        if (state() != ScrobblingBackendState::WaitingForUserCredentials)
        {
            setState(ScrobblingBackendState::WaitingForUserCredentials);
            return;
        }
    }
}

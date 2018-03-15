/*
    Copyright (C) 2018, Kevin Andre <hyperquantum@gmail.com>

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

#include "lastfmscrobblingprovider.h"

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

namespace PMP {

    LastFmScrobblingProvider::LastFmScrobblingProvider()
     : ScrobblingProvider(), _networkAccessManager(nullptr)
    {
        qDebug() << "Creating LastFmScrobblingProvider;  user-agent:" << userAgent;
    }

    const char* LastFmScrobblingProvider::apiUrl = "https://ws.audioscrobbler.com/2.0/";
    const char* LastFmScrobblingProvider::apiKey = "fc44ba796d201052f53f92818834f907";
    const char* LastFmScrobblingProvider::apiSecret = "3e58b46e070c34718686e0dfbd02d22f";
    const char* LastFmScrobblingProvider::userAgent =
                    "Party Music Player " PMP_VERSION_DISPLAY " (LFM scrobbler v0.0.1)";
    const char* LastFmScrobblingProvider::contentTypeForPostRequest =
                                                      "application/x-www-form-urlencoded";

    void LastFmScrobblingProvider::initialize() {
        if (_sessionKey.isNull()) {
            setState(ScrobblingProviderState::NeedAuthentication);
        }
        else {
            setState(ScrobblingProviderState::ReadyForScrobbling);
        }
    }

    void LastFmScrobblingProvider::setSessionKey(const QString& sessionKey) {
        if (_sessionKey == sessionKey) return; /* no change */
        _sessionKey = sessionKey;

        if (_sessionKey.isNull()) {
            setState(ScrobblingProviderState::NeedAuthentication);
        }
        else {
            if (state() == ScrobblingProviderState::NeedAuthentication) {
                setState(ScrobblingProviderState::ReadyForScrobbling);
            }
        }
    }

    void LastFmScrobblingProvider::doGetMobileTokenCall(const QString& username,
                                                        const QString& password)
    {
        QVector<QPair<QString, QString>> parameters;
        parameters << QPair<QString, QString>("method", "auth.getMobileSession");
        parameters << QPair<QString, QString>("api_key", apiKey);
        parameters << QPair<QString, QString>("password", password);
        parameters << QPair<QString, QString>("username", username);

        signAndSendPost(parameters);
    }

    void LastFmScrobblingProvider::doScrobbleCall(QDateTime timestamp,
                                                  const QString& title,
                                                  const QString& artist,
                                                  const QString& album,
                                                  int trackDurationSeconds)
    {
        if (_sessionKey.isEmpty()) return; /* cannot do it */

        /* 'toSecsSinceEpoch()' requires Qt 5.8, and toTime_t() is deprecated */
        auto timestampAsUnixTime = timestamp.toMSecsSinceEpoch() / 1000;
        auto timestampText = QString::number(timestampAsUnixTime);

        QVector<QPair<QString, QString>> parameters;
        parameters << QPair<QString, QString>("method", "track.scrobble");
        parameters << QPair<QString, QString>("album", album);
        parameters << QPair<QString, QString>("api_key", apiKey);
        parameters << QPair<QString, QString>("artist", artist);
        if (trackDurationSeconds > 0) {
            auto durationText = QString::number(trackDurationSeconds);
            parameters << QPair<QString, QString>("duration", durationText);
        }
        parameters << QPair<QString, QString>("sk", _sessionKey);
        parameters << QPair<QString, QString>("timestamp", timestampText);
        parameters << QPair<QString, QString>("track", title);

        signAndSendPost(parameters);
    }

    QNetworkReply* LastFmScrobblingProvider::signAndSendPost(
                                              QVector<QPair<QString, QString>> parameters)
    {
        signCall(parameters);

        if (!_networkAccessManager) {
            _networkAccessManager = new QNetworkAccessManager(this);

            connect(
                _networkAccessManager, &QNetworkAccessManager::finished,
                this, &LastFmScrobblingProvider::requestFinished
            );
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

        qDebug() << "parameters:" << parametersString;
        return _networkAccessManager->post(request, parametersString);
    }

    void LastFmScrobblingProvider::requestFinished(QNetworkReply* reply) {        
        auto replyData = reply->readAll();
        qDebug() << "Last.Fm reply received. Byte count:" << replyData.size();

        reply->deleteLater();

        auto replyText = QString::fromUtf8(replyData);
        qDebug() << "Last.Fm reply:\n" << replyText;

        auto error = reply->error();
        if (error != QNetworkReply::NoError) {
            qDebug() << "Last.Fm reply has error code" << error
                     << "with error text:" << reply->errorString();

            //return;
        }

        QDomDocument dom;
        QString xmlParseError;
        int xmlErrorLine;
        if (!dom.setContent(replyData, &xmlParseError, &xmlErrorLine)) {
            qDebug() << "Could not parse the Last.Fm reply as valid XML;"
                     << "error at line" << xmlErrorLine << ":" << xmlParseError;

            return;
        }

        auto lfmElement = dom.documentElement();
        if (lfmElement.isNull() || lfmElement.tagName() != "lfm") {
            qDebug() << "Last.Fm reply XML does not have <lfm> root element";
            return;
        }

        auto status = lfmElement.attribute("status");
        if (status != "ok") {
            qDebug() << "Last.Fm reply indicates that the request failed";

            auto errorElement = lfmElement.firstChildElement("error");
            parseError(errorElement);

        }
        else {
            auto sessionNode = lfmElement.firstChildElement("session");
            if (!sessionNode.isNull()) {
                qDebug() << "have session node";
                auto nameNode = sessionNode.firstChildElement("name");
                auto keyNode = sessionNode.firstChildElement("key");
                qDebug() << " name:" << nameNode.text();
                qDebug() << " key:" << keyNode.text();

                _username = nameNode.text();
                setSessionKey(keyNode.text());
            }

            auto scrobblesNode = lfmElement.firstChildElement("scrobbles");
            if (!scrobblesNode.isNull()) {
                qDebug() << "have scrobbles node";
                parseScrobbles(scrobblesNode);
            }
        }



        emit receivedAuthenticationReply();
    }

    void LastFmScrobblingProvider::parseError(const QDomElement& errorElement) {
        if (errorElement.isNull()) return; /* gibberish */

        auto errorCodeText = errorElement.attribute("code");
        qDebug() << "error code:" << errorCodeText;
        qDebug() << "error message:" << errorElement.text();

        bool ok;
        int errorCode = errorCodeText.toInt(&ok);

        if (!ok) return; /* gibberish */

        switch (errorCode) {
            case 4: /* authentication failed */
                setState(ScrobblingProviderState::InvalidUserCredentials);
                break;

            case 9: /* invalid session key, need to re-authenticate */
                setSessionKey("");
                break;

            case 11: case 16:
                /* TODO: retry request later */
                setState(ScrobblingProviderState::TemporarilyUnavailable);
                break;

            case 2: case 3: case 5: case 6: case 7: case 13: case 27:
                /* these indicate a bug in creating the request */
                setState(ScrobblingProviderState::PermanentFatalError);
                break;

            case 10: case 26: /* invalid/suspended API key */
                setState(ScrobblingProviderState::PermanentFatalError);
                break;

            case 29: /* rate limit exceeded */
                setState(ScrobblingProviderState::TemporarilyUnavailable);
                break;

            default:
                /* unknown error code */
                setState(ScrobblingProviderState::PermanentFatalError);
                break;
        }
    }

    void LastFmScrobblingProvider::parseScrobbles(const QDomElement& scrobblesElement) {
        auto ignoredText = scrobblesElement.attribute("ignored");
        auto acceptedText = scrobblesElement.attribute("accepted");



        for(auto node = scrobblesElement.firstChild();
            !node.isNull();
            node = node.nextSibling())
        {
            auto scrobbleElement = node.toElement();

            if (scrobbleElement.isNull()) continue;
            if (scrobbleElement.tagName() != "scrobble") continue;

            auto timestampElement = scrobbleElement.firstChildElement("timestamp");
            if (timestampElement.isNull()) continue;

            bool ok;
            auto timestampNumber = timestampElement.text().toLongLong(&ok);
            if (!ok) continue;

            bool scrobbleAccepted = false;
            auto ignoredMessageElement =
                scrobbleElement.firstChildElement("ignoredMessage");
            if (ignoredMessageElement.isNull()) continue;

            auto ignoredReasonText = ignoredMessageElement.text();
            auto ignoredReason = ignoredMessageElement.attribute("code").toInt(&ok);
            if (!ok) continue;

            if (ignoredReason == 0) scrobbleAccepted = true;

            if (scrobbleAccepted)
                qDebug() << "scrobble was accepted";
            else
                qDebug() << "scrobble NOT accepted for reason code" << ignoredReason
                         << ":" << ignoredReasonText;

            auto titleText = scrobbleElement.firstChildElement("track").text();
            auto artistText = scrobbleElement.firstChildElement("artist").text();
            auto albumText = scrobbleElement.firstChildElement("album").text();

            qDebug() << "received:\n"
                     << " title:" << titleText << "\n"
                     << " artist:" << artistText << "\n"
                     << " album:" << albumText;



        }


    }

    void LastFmScrobblingProvider::signCall(QVector<QPair<QString, QString>>& parameters)
    {
        std::sort(parameters.begin(), parameters.end());

        QByteArray signData;
        Q_FOREACH(auto parameter, parameters) {
            signData += parameter.first.toUtf8();
            signData += parameter.second.toUtf8();
        }

        signData += apiSecret;

        auto hash = QCryptographicHash::hash(signData, QCryptographicHash::Md5);

        qDebug() << "Last.Fm signature data:" << QString::fromUtf8(signData);
        qDebug() << "Last.Fm signature generated:" << hash.toHex();
        parameters << QPair<QString, QString>("api_sig", hash.toHex());
    }

}
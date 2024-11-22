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

#ifndef PMP_LASTFMSCROBBLINGBACKEND_H
#define PMP_LASTFMSCROBBLINGBACKEND_H

#include "scrobblingbackend.h"

#include "common/promise.h"

#include "result.h"

#include <QDateTime>
#include <QNetworkReply>
#include <QPair>
#include <QString>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QDomElement)
QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)

namespace PMP::Server
{
    class LastFmScrobblingBackend;

    class LastFmRequestHandler : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void mustRecreateNetworkManager();
        void fatalError();
        void shouldTryAgainLater();
        void mustInvalidateSessionKey();

    protected:
        LastFmRequestHandler(LastFmScrobblingBackend* parent,
                             QNetworkReply* pendingReply, QString xmlTagName);

        ~LastFmRequestHandler();

        virtual void onNetworkError(QNetworkReply::NetworkError error, QString errorText);
        virtual void onParseError();
        virtual void handleOkReply(const QDomElement& childElement) = 0;
        virtual void handleErrorCode(int lastFmErrorCode);
        virtual void onGenericError() = 0;

    private slots:
        void requestFinished();

    private:
        void parseReply(QByteArray bytes);

        void handleErrorReply(const QDomElement& errorElement);

        LastFmScrobblingBackend* _parent;
        QNetworkReply* _reply;
        QString _xmlTagName;
    };

    struct LastFmAuthenticationResult
    {
        QString username;
        QString sessionKey;
    };

    class LastFmAuthenticationRequestHandler : public LastFmRequestHandler
    {
        Q_OBJECT
    public:
        LastFmAuthenticationRequestHandler(LastFmScrobblingBackend* parent,
                                           QNetworkReply* pendingReply);

        Future<LastFmAuthenticationResult, Result> future() const;

    protected:
        void handleOkReply(const QDomElement& childElement) override;
        void handleErrorCode(int lastFmErrorCode) override;
        void onGenericError() override;

    private:
        Promise<LastFmAuthenticationResult, Result> _promise;
    };

    class LastFmNowPlayingRequestHandler : public LastFmRequestHandler
    {
        Q_OBJECT
    public:
        LastFmNowPlayingRequestHandler(LastFmScrobblingBackend* parent,
                                       QNetworkReply* pendingReply);

    Q_SIGNALS:
        void nowPlayingUpdateSuccessful();
        void nowPlayingUpdateFailed();

    protected:
        void handleOkReply(const QDomElement& childElement) override;
        void onGenericError() override;
    };

    class LastFmScrobbleRequestHandler : public LastFmRequestHandler
    {
        Q_OBJECT
    public:
        LastFmScrobbleRequestHandler(LastFmScrobblingBackend* parent,
                                     QNetworkReply* pendingReply);

    Q_SIGNALS:
        void scrobbleSuccessful();
        void scrobbleIgnored();
        void scrobbleError();

    protected:
        void handleOkReply(const QDomElement& childElement) override;
        void onGenericError() override;
    };

    class LastFmScrobblingBackend : public ScrobblingBackend
    {
        Q_OBJECT
    public:
        LastFmScrobblingBackend();

        void setUsername(const QString& username);
        void setSessionKey(const QString& sessionKey);

        QString username() const;
        QString sessionKey() const;

        void updateNowPlaying(ScrobblingTrack track) override;
        void scrobbleTrack(QDateTime timestamp, ScrobblingTrack track) override;

        SimpleFuture<Result> authenticateWithCredentials(QString usernameOrEmail,
                                                         QString password) override;

    public slots:
        void initialize() override;

    Q_SIGNALS:
        void authenticatedSuccessfully(QString username, QString sessionKey);

    protected:
        bool needsSsl() const override;

    private slots:
        void disposeOfNetworkAccessManager();

    private:
        void updateState();
        void leaveState(ScrobblingBackendState oldState);

        LastFmAuthenticationRequestHandler* doGetMobileTokenCall(QString usernameOrEmail,
                                                                 QString password);

        LastFmNowPlayingRequestHandler* doUpdateNowPlayingCall(QString sessionKey,
                                                               ScrobblingTrack track);

        LastFmScrobbleRequestHandler* doScrobbleCall(QString sessionKey,
                                                     QDateTime timestamp,
                                                     ScrobblingTrack track);

        static void signCall(QVector<QPair<QString, QString>>& parameters);
        QNetworkReply* signAndSendPost(QVector<QPair<QString, QString>> parameters);

        void connectStateHandlingSignals(LastFmRequestHandler* handler);

        static const char* apiUrl;
        static const char* apiKey;
        static const char* apiSecret;
        static const char* contentTypeForPostRequest;
        static const char* userAgent;

        QNetworkAccessManager* _networkAccessManager;
        QString _username;
        QString _sessionKey;
    };
}
#endif

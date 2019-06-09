/*
    Copyright (C) 2018-2019, Kevin Andre <hyperquantum@gmail.com>

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

#include <QDateTime>
#include <QPair>
#include <QString>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QDomElement)
QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace PMP {

    class LastFmScrobblingBackend : public ScrobblingBackend
    {
        Q_OBJECT
    public:
        LastFmScrobblingBackend();

        void doGetMobileTokenCall(QString const& usernameOrEmail,
                                  QString const& password);

        void doUpdateNowPlayingCall(QString const& title, QString const& artist,
                                    QString const& album, int trackDurationSeconds = -1);

        void doScrobbleCall(QDateTime timestamp, QString const& title,
                            QString const& artist, QString const& album,
                            int trackDurationSeconds = -1);

        void setUsername(const QString& username);
        void setSessionKey(const QString& sessionKey);

        QString username() const;
        QString sessionKey() const;

        void updateNowPlaying(QString const& title, QString const& artist,
                              QString const& album,
                              int trackDurationSeconds = -1) override;

        void scrobbleTrack(QDateTime timestamp, QString const& title,
                           QString const& artist, QString const& album,
                           int trackDurationSeconds = -1) override;

    public slots:
        void initialize() override;
        void authenticateWithCredentials(QString usernameOrEmail, QString password);

    Q_SIGNALS:
        void needUserCredentials(QString suggestedUsername, bool authenticationFailed);
        //void receivedAuthenticationReply();

    private slots:
        void requestFinished(QNetworkReply* reply);

    private:
        void updateState();
        void leaveState(ScrobblingBackendState oldState);

        static void signCall(QVector<QPair<QString, QString>>& parameters);
        QNetworkReply* signAndSendPost(QVector<QPair<QString, QString>> parameters);

        void parseReply(QDomElement const& lfmElement, bool isScrobbleReply);
        void parseError(QDomElement const& errorElement);
        ScrobbleResult parseScrobbles(QDomElement const& scrobblesElement);

        static const char* apiUrl;
        static const char* apiKey;
        static const char* apiSecret;
        static const char* contentTypeForPostRequest;
        static const char* userAgent;

        QNetworkAccessManager* _networkAccessManager;
        QNetworkReply* _authenticationReply;
        QNetworkReply* _nowPlayingReply;
        QNetworkReply* _scrobbleReply;
        QString _username;
        QString _sessionKey;
    };
}
#endif

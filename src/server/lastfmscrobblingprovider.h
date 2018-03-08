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

#ifndef PMP_LASTFMSCROBBLINGPROVIDER_H
#define PMP_LASTFMSCROBBLINGPROVIDER_H

#include "scrobblingprovider.h"

#include <QDateTime>
#include <QPair>
#include <QString>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QDomElement)
QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace PMP {

    class LastFmScrobblingProvider : public ScrobblingProvider
    {
        Q_OBJECT
    public:
        LastFmScrobblingProvider();

        void doGetMobileTokenCall(QString const& username, QString const& password);

        void doScrobbleCall(QDateTime timestamp, QString const& title,
                            QString const& artist, QString const& album,
                            int trackDurationSeconds = -1);

        void setSessionKey(const QString& sessionKey);

    Q_SIGNALS:
        void receivedAuthenticationReply();

    public slots:


    private slots:
        void requestFinished(QNetworkReply* reply);

    private:
        QNetworkReply* signAndSendPost(QVector<QPair<QString, QString>> parameters);

        void parseScrobbles(QDomElement const& scrobblesElement);

        static void signCall(QVector<QPair<QString, QString>>& parameters);

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

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

#ifndef PMP_SERVERSETTINGS_H
#define PMP_SERVERSETTINGS_H

#include <QObject>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QSettings)

namespace PMP::Server
{
    struct DatabaseConnectionSettings
    {
        QString hostname;
        int port;
        QString username;
        QString password;

        DatabaseConnectionSettings()
         : port(0)
        {
            //
        }

        bool isComplete() const
        {
            return !hostname.isEmpty() && !username.isEmpty() && !password.isEmpty();
        }

        bool operator==(DatabaseConnectionSettings const& other) const
        {
            return hostname == other.hostname
                && port == other.port
                && username == other.username
                && password == other.password;
        }

        bool operator!=(DatabaseConnectionSettings const& other) const
        {
            return !(*this == other);
        }
    };

    class ServerSettings : public QObject
    {
        Q_OBJECT
    public:
        ServerSettings();

        QString serverCaption() const { return _serverCaption; }
        int defaultVolume() const { return _defaultVolume; }
        QStringList musicPaths() const { return _musicPaths; }
        QString fixedServerPassword() const { return _fixedServerPassword; }

        DatabaseConnectionSettings databaseConnectionSettings() const
        {
            return _databaseConnectionSettings;
        }

    public Q_SLOTS:
        void load();

    Q_SIGNALS:
        void serverCaptionChanged();
        void defaultVolumeChanged();
        void musicPathsChanged();
        void fixedServerPasswordChanged();
        void databaseConnectionSettingsChanged();

    private:
        void loadServerCaption(QSettings& settings);
        void loadDefaultVolume(QSettings& settings);
        void loadMusicPaths(QSettings& settings);
        void loadFixedServerPassword(QSettings& settings);
        void loadDatabaseConnectionSettings(QSettings& settings);

        void setServerCaption(QString serverCaption);
        void setDefaultVolume(int volume);
        void setMusicPaths(QStringList const& paths);
        void setFixedServerPassword(QString password);
        void setDatabaseConnectionSettings(DatabaseConnectionSettings const& settings);

        static QStringList generateDefaultScanPaths();

        QString _serverCaption;
        int _defaultVolume;
        QStringList _musicPaths;
        QString _fixedServerPassword;
        DatabaseConnectionSettings _databaseConnectionSettings;
    };
}
#endif

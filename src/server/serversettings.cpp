/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "serversettings.h"

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QtDebug>

namespace PMP
{
    ServerSettings::ServerSettings()
    {
        //
    }

    void ServerSettings::load()
    {
        qDebug() << "loading server settings file";

        QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                           QCoreApplication::organizationName(),
                           QCoreApplication::applicationName());

        settings.setIniCodec("UTF-8");

        loadServerCaption(settings);
        loadDefaultVolume(settings);
        loadMusicPaths(settings);
        loadFixedServerPassword(settings);
        loadDatabaseConnectionSettings(settings);
    }

    void ServerSettings::loadServerCaption(QSettings& settings)
    {
        /* in Qt .ini files, the group "General" is used when not specifying a group */

        QVariant captionSetting = settings.value("server_caption");
        if (!captionSetting.isValid() || captionSetting.toString().isEmpty())
        {
            settings.setValue("server_caption", "");
            setServerCaption({});
            return;
        }

        auto caption = captionSetting.toString();

        if (caption.length() > 63)
        {
            qWarning() << "Server caption as defined in the settings file is too long;"
                       << "maximum length is 63 characters";
        }

        caption.truncate(63);

        setServerCaption(caption);
    }

    void ServerSettings::loadDefaultVolume(QSettings& settings)
    {
        int volume = -1;
        QVariant defaultVolumeSetting = settings.value("player/default_volume");
        if (defaultVolumeSetting.isValid() && defaultVolumeSetting.toString() != "")
        {
            bool ok;
            volume = defaultVolumeSetting.toString().toInt(&ok);
            if (!ok || volume < 0 || volume > 100)
            {
                qWarning() << "server settings: ignoring invalid default volume; must be a number from 0 to 100";
                volume = -1;
            }
        }
        if (volume < 0)
        {
            settings.setValue("player/default_volume", "");
        }

        setDefaultVolume(volume);
    }

    void ServerSettings::loadMusicPaths(QSettings& settings)
    {
        QStringList paths;
        QVariant musicPathsSetting = settings.value("media/scan_directories");
        if (!musicPathsSetting.isValid() || musicPathsSetting.toStringList().empty())
        {
            qInfo() << "server settings: no music paths set. Setting default paths";
            paths = generateDefaultScanPaths();
            settings.setValue("media/scan_directories", paths);
        }
        else
        {
            paths = musicPathsSetting.toStringList();
        }

        setMusicPaths(paths);
    }

    void ServerSettings::loadFixedServerPassword(QSettings& settings)
    {
        /* get rid of old 'serverpassword' setting if it still exists */
        settings.remove("security/serverpassword");

        QString fixedServerPassword;

        QVariant fixedServerPasswordSetting =
                settings.value("security/fixedserverpassword");

        if (!fixedServerPasswordSetting.isValid()
                || fixedServerPasswordSetting.toString().isEmpty())
        {
            // not set or unreadable
        }
        else
        {
            fixedServerPassword = fixedServerPasswordSetting.toString();

            // TODO : make this more strict
            if (fixedServerPassword.length() < 6)
            {
                qWarning() << "server settings: ignoring 'fixedserverpassword' setting"
                           << " because its value is unsafe (too short)";
                fixedServerPassword.clear();
            }
        }

        if (fixedServerPassword.isEmpty())
        {
            settings.setValue("security/fixedserverpassword", "");
        }

        setFixedServerPassword(fixedServerPassword);
    }

    void ServerSettings::loadDatabaseConnectionSettings(QSettings& settings)
    {
        DatabaseConnectionSettings newConnectionSettings;

        QVariant hostnameSetting = settings.value("database/hostname");
        if (!hostnameSetting.isValid() || hostnameSetting.toString().isEmpty())
        {
            settings.setValue("database/hostname", "");
        }
        else
        {
            newConnectionSettings.hostname = hostnameSetting.toString();
        }

        QVariant usernameSetting = settings.value("database/username");
        if (!usernameSetting.isValid() || usernameSetting.toString().isEmpty())
        {
            settings.setValue("database/username", "");
        }
        else
        {
            newConnectionSettings.username = usernameSetting.toString();
        }

        QVariant passwordSetting = settings.value("database/password");
        if (!passwordSetting.isValid() || passwordSetting.toString().isEmpty())
        {
            settings.setValue("database/password", "");
        }
        else
        {
            newConnectionSettings.password = passwordSetting.toString();
        }

        /*
        QVariant schemaSetting = settings.value("database/schema");
        if (!schemaSetting.isValid() || schemaSetting.toString().isEmpty())
        {
            settings.setValue("database/schema", "");
        }
        else
        {
            newConnectionSettings.schema = schemaSetting.toString();
        }
        */

        setDatabaseConnectionSettings(newConnectionSettings);
    }

    void ServerSettings::setServerCaption(QString serverCaption)
    {
        if (serverCaption == _serverCaption)
            return; /* no change */

        _serverCaption = serverCaption;
        Q_EMIT serverCaptionChanged();
    }

    void ServerSettings::setDefaultVolume(int volume)
    {
        if (volume == _defaultVolume)
            return; /* no change */

        _defaultVolume = volume;
        Q_EMIT defaultVolumeChanged();
    }

    void ServerSettings::setMusicPaths(const QStringList& paths)
    {
        QStringList newPaths = paths;
        newPaths.sort();
        newPaths.removeDuplicates();

        if (newPaths == _musicPaths)
            return; /* no change */

        _musicPaths = newPaths;
        Q_EMIT musicPathsChanged();
    }

    void ServerSettings::setFixedServerPassword(QString password)
    {
        if (password == _fixedServerPassword)
            return; /* no change */

        _fixedServerPassword = password;
        Q_EMIT fixedServerPasswordChanged();
    }

    void ServerSettings::setDatabaseConnectionSettings(
                                               DatabaseConnectionSettings const& settings)
    {
        if (settings == _databaseConnectionSettings)
            return; /* no change */

        _databaseConnectionSettings = settings;
        Q_EMIT databaseConnectionSettingsChanged();
    }

    QStringList ServerSettings::generateDefaultScanPaths()
    {
        QStringList paths;

        paths.append(QStandardPaths::standardLocations(QStandardPaths::MusicLocation));

        paths.append(
                    QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation));

        paths.append(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation));

        return paths;
    }

}

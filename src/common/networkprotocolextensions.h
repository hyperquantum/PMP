/*
    Copyright (C) 2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_NETWORKPROTOCOLEXTENSIONS_H
#define PMP_NETWORKPROTOCOLEXTENSIONS_H

#include "networkprotocol.h"
#include "nullable.h"

#include <QByteArray>
#include <QHash>
#include <QHashFunctions>
#include <QList>
#include <QString>
#include <QtDebug>

namespace PMP
{
    enum class NetworkProtocolExtension
    {
        NoneOrInvalid = 0,
        Scrobbling,
        // ExtensionName1,
        // ExtensionName2,
    };

    inline QString toString(NetworkProtocolExtension extension)
    {
        switch (extension)
        {
        case NetworkProtocolExtension::NoneOrInvalid:
            return "NoneOrInvalid";

        case NetworkProtocolExtension::Scrobbling:
            return "Scrobbling";

        //case NetworkProtocolExtension::ExtensionName1:
        //    return "ExtensionName1";

        //case NetworkProtocolExtension::ExtensionName2:
        //    return "ExtensionName2";
        }

        qWarning() << "NetworkProtocolExtension value not handled:"
                   << static_cast<int>(extension);
        return QString::number(static_cast<int>(extension));
    }

    inline QDebug operator<<(QDebug debug, NetworkProtocolExtension extension)
    {
        switch (extension)
        {
        case NetworkProtocolExtension::NoneOrInvalid:
            debug << "NetworkProtocolExtension::Unknown";
            return debug;

        case NetworkProtocolExtension::Scrobbling:
            debug << "NetworkProtocolExtension::Stopped";
            return debug;

        //case NetworkProtocolExtension::ExtensionName1:
        //    debug << "NetworkProtocolExtension::ExtensionName1";
        //    return debug;

        //case NetworkProtocolExtension::ExtensionName2:
        //    debug << "NetworkProtocolExtension::ExtensionName2";
        //    return debug;
        }

        debug << static_cast<int>(extension);
        return debug;
    }

    inline uint qHash(NetworkProtocolExtension extension, uint seed)
    {
        return ::qHash(static_cast<int>(extension), seed);
    }

    class NetworkProtocolExtensionTags
    {
    public:
        static NetworkProtocolExtensionTags const& instance();

        NetworkProtocolExtension getExtensionForTag(QString tag) const;
        QString getTagForExtension(NetworkProtocolExtension extension) const;

    private:
        NetworkProtocolExtensionTags();

        void registerTag(NetworkProtocolExtension extension, QString tag);

        QHash<QString, NetworkProtocolExtension> _tagToEnum;
        QHash<NetworkProtocolExtension, QString> _enumToTag;
    };

    class NetworkProtocolExtensionInfo
    {
    public:
        NetworkProtocolExtensionInfo();

        NetworkProtocolExtensionInfo(NetworkProtocolExtension extension, quint8 id,
                                     quint8 version);

        NetworkProtocolExtension extension;
        quint8 id;
        quint8 version;
    };

    class NetworkProtocolExtensionSupportMap
    {
    public:
        void registerExtensionSupport(NetworkProtocolExtensionInfo const& extension);

        Nullable<NetworkProtocolExtensionInfo> getExtensionInfo(
                                                NetworkProtocolExtension extension) const;
        Nullable<NetworkProtocolExtensionInfo> getExtensionInfo(quint8 id) const;

        quint8 getExtensionId(NetworkProtocolExtension extension) const;
        NetworkProtocolExtension getExtensionById(quint8 id) const;

        bool isSupported(NetworkProtocolExtension extension, quint8 version) const;

        bool isNotSupported(NetworkProtocolExtension extension, quint8 version) const
        {
            return !isSupported(extension, version);
        }

        QList<NetworkProtocolExtensionInfo> getAllExtensions() const;

    private:
        QHash<NetworkProtocolExtension, NetworkProtocolExtensionInfo> _byExtension;
        QHash<quint8, NetworkProtocolExtensionInfo> _byId;
    };

    class NetworkProtocolExtensionMessages
    {
    public:
        static Nullable<NetworkProtocolExtensionSupportMap> parseExtensionSupportMessage(
                                                               QByteArray const& message);
        static QByteArray generateExtensionSupportMessage(ClientOrServer typeOfMessage,
                              NetworkProtocolExtensionSupportMap const& extensionSupport);

        static QByteArray generateExtensionMessageStart(
                               NetworkProtocolExtension extension,
                               NetworkProtocolExtensionSupportMap const& extensionSupport,
                               quint8 messageType);

        static QByteArray generateExtensionResultMessage(quint8 extensionId,
                                                         quint8 resultCode,
                                                         quint32 clientReference);

    private:
        static quint16 encodeMessageTypeForExtension(quint8 extensionId,
                                                     quint8 messageType);
    };
}

Q_DECLARE_METATYPE(PMP::NetworkProtocolExtension)

#endif

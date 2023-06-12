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

#include "networkprotocolextensions.h"

#include "networkprotocol.h"
#include "networkutil.h"

#include <QtDebug>
#include <QVector>

namespace PMP
{
    NetworkProtocolExtensionTags const& NetworkProtocolExtensionTags::instance()
    {
        static NetworkProtocolExtensionTags staticInstance;
        return staticInstance;
    }

    NetworkProtocolExtension NetworkProtocolExtensionTags::getExtensionForTag(
                                                                        QString tag) const
    {
        return _tagToEnum.value(tag, NetworkProtocolExtension::NoneOrInvalid);
    }

    QString NetworkProtocolExtensionTags::getTagForExtension(
                                                 NetworkProtocolExtension extension) const
    {
        return _enumToTag.value(extension, {});
    }

    NetworkProtocolExtensionTags::NetworkProtocolExtensionTags()
    {
        registerTag(NetworkProtocolExtension::Scrobbling, "scrobbling");
    }

    void NetworkProtocolExtensionTags::registerTag(NetworkProtocolExtension extension,
                                                   QString tag)
    {
        Q_ASSERT_X(!_enumToTag.contains(extension),
                   "NetworkProtocolExtensionTags::registerTag",
                   "enum registered already!");
        Q_ASSERT_X(!_tagToEnum.contains(tag),
                   "NetworkProtocolExtensionTags::registerTag",
                   "tag registered already!");

        _enumToTag.insert(extension, tag);
        _tagToEnum.insert(tag, extension);
    }

    NetworkProtocolExtensionInfo::NetworkProtocolExtensionInfo()
        : extension(NetworkProtocolExtension::NoneOrInvalid), id(0), version(0)
    {
        //
    }

    NetworkProtocolExtensionInfo::NetworkProtocolExtensionInfo(
                                                    NetworkProtocolExtension extension,
                                                    quint8 id, quint8 version)
        : extension(extension), id(id), version(version)
    {
        //
    }

    void NetworkProtocolExtensionSupportMap::registerExtensionSupport(
                                            const NetworkProtocolExtensionInfo& extension)
    {
        Q_ASSERT_X(extension.id > 0,
                   "NetworkProtocolExtensionSupportMap::registerExtensionSupport",
                   "extension id must be non-zero");
        Q_ASSERT_X(extension.version > 0,
                   "NetworkProtocolExtensionSupportMap::registerExtensionSupport",
                   "extension version must be non-zero");
        Q_ASSERT_X(!_byExtension.contains(extension.extension),
                   "NetworkProtocolExtensionSupportMap::registerExtensionSupport",
                   "extension registered already!");
        Q_ASSERT_X(!_byId.contains(extension.id),
                   "NetworkProtocolExtensionSupportMap::registerExtensionSupport",
                   "id registered already!");

        _byExtension.insert(extension.extension, extension);
        _byId.insert(extension.id, extension);
    }

    Nullable<NetworkProtocolExtensionInfo>
    NetworkProtocolExtensionSupportMap::getExtensionInfo(
                                                 NetworkProtocolExtension extension) const
    {
        auto it = _byExtension.constFind(extension);
        if (it == _byExtension.constEnd())
            return null;

        return it.value();
    }

    Nullable<NetworkProtocolExtensionInfo>
        NetworkProtocolExtensionSupportMap::getExtensionInfo(quint8 id) const
    {
        auto it = _byId.constFind(id);
        if (it == _byId.constEnd())
            return null;

        return it.value();
    }

    quint8 NetworkProtocolExtensionSupportMap::getExtensionId(
                                                 NetworkProtocolExtension extension) const
    {
        auto it = _byExtension.constFind(extension);
        if (it == _byExtension.constEnd())
            return 0;

        return it.value().id;
    }

    NetworkProtocolExtension NetworkProtocolExtensionSupportMap::getExtensionById(
                                                                          quint8 id) const
    {
        auto it = _byId.constFind(id);
        if (it == _byId.constEnd())
            return NetworkProtocolExtension::NoneOrInvalid;

        return it.value().extension;
    }

    bool NetworkProtocolExtensionSupportMap::isSupported(
                                                      NetworkProtocolExtension extension,
                                                      quint8 version) const
    {
        auto it = _byExtension.constFind(extension);
        if (it == _byExtension.constEnd())
            return false;

        return it.value().version >= version;
    }

    QList<NetworkProtocolExtensionInfo>
        NetworkProtocolExtensionSupportMap::getAllExtensions() const
    {
        return _byExtension.values();
    }

    Nullable<NetworkProtocolExtensionSupportMap>
        NetworkProtocolExtensionMessages::parseExtensionSupportMessage(
                                                                const QByteArray& message)
    {
        if (message.length() < 4)
            return null; /* invalid message */

        /* be strict about reserved space */
        int filler = NetworkUtil::getByteUnsignedToInt(message, 2);
        if (filler != 0)
            return null; /* invalid message */

        int extensionCount = NetworkUtil::getByteUnsignedToInt(message, 3);
        if (message.length() < 4 + extensionCount * 4)
            return null; /* invalid message */

        auto& tagsLookup = NetworkProtocolExtensionTags::instance();

        NetworkProtocolExtensionSupportMap supportMap;
        QSet<quint8> idsEncountered;
        QSet<QString> tagsEncountered;
        idsEncountered.reserve(extensionCount);
        tagsEncountered.reserve(extensionCount);

        int offset = 4;
        for (int i = 0; i < extensionCount; ++i)
        {
            if (offset > message.length() - 3)
                return null; /* invalid message */

            quint8 id = NetworkUtil::getByte(message, offset);
            quint8 version = NetworkUtil::getByte(message, offset + 1);
            quint8 byteCount = NetworkUtil::getByte(message, offset + 2);
            offset += 3;

            if (id == 0 || version == 0 || byteCount == 0
                || offset > message.length() - byteCount)
            {
                return null; /* invalid message */
            }

            QString tag = NetworkUtil::getUtf8String(message, offset, byteCount);
            offset += byteCount;

            if (idsEncountered.contains(id) || tagsEncountered.contains(tag))
                return null; /* invalid message */

            idsEncountered << id;
            tagsEncountered << tag;

            auto extension = tagsLookup.getExtensionForTag(tag);
            if (extension == NetworkProtocolExtension::NoneOrInvalid)
            {
                qDebug() << "network protocol extension with tag" << ("\"" + tag + "\"")
                         << "not recognized; id:" << id << " version:" << version;
            }
            else
            {
                qDebug() << "network protocol extension" << toString(extension)
                         << "will be identified with ID" << id << "by the other party";
            }

            if (extension != NetworkProtocolExtension::NoneOrInvalid)
            {
                NetworkProtocolExtensionInfo extensionInfo(extension, id, version);

                supportMap.registerExtensionSupport(extensionInfo);
            }
        }

        if (offset != message.length())
            return null; /* invalid message */

        return supportMap;
    }

    QByteArray NetworkProtocolExtensionMessages::generateExtensionSupportMessage(
                               ClientOrServer typeOfMessage,
                               const NetworkProtocolExtensionSupportMap& extensionSupport)
    {
        const auto extensions = extensionSupport.getAllExtensions();

        quint8 extensionCount = static_cast<quint8>(extensions.size());

        QByteArray message;
        message.reserve(4 + extensionCount * 16); /* estimate */

        if (typeOfMessage == ClientOrServer::Client)
        {
            NetworkProtocol::append2Bytes(message,
                                          ClientMessageType::ClientExtensionsMessage);
        }
        else
        {
            NetworkProtocol::append2Bytes(message,
                                          ServerMessageType::ServerExtensionsMessage);
        }

        NetworkUtil::appendByte(message, 0); /* filler */
        NetworkUtil::appendByte(message, extensionCount);

        auto& tagsLookup = NetworkProtocolExtensionTags::instance();

        for (auto extensionInfo : extensions)
        {
            auto tag = tagsLookup.getTagForExtension(extensionInfo.extension);

            QByteArray tagBytes = tag.toUtf8();
            quint8 tagBytesCount = static_cast<quint8>(tagBytes.size());

            NetworkUtil::appendByte(message, extensionInfo.id);
            NetworkUtil::appendByte(message, extensionInfo.version);
            NetworkUtil::appendByte(message, tagBytesCount);
            message += tagBytes;
        }

        return message;
    }

    QByteArray NetworkProtocolExtensionMessages::generateExtensionMessageStart(
                               NetworkProtocolExtension extension,
                               const NetworkProtocolExtensionSupportMap& extensionSupport,
                               quint8 messageType)
    {
        auto infoOrNull = extensionSupport.getExtensionInfo(extension);
        Q_ASSERT_X(infoOrNull != null,
                   "NetworkProtocolExtensionMessages::generateExtensionMessageStart",
                   "extension not found in map");

        auto extensionInfo = infoOrNull.value();

        quint16 encodedMessageType =
            encodeMessageTypeForExtension(extensionInfo.id, messageType);

        QByteArray header;
        header.reserve(2);
        NetworkUtil::append2Bytes(header, encodedMessageType);
        return header;
    }

    QByteArray NetworkProtocolExtensionMessages::generateExtensionResultMessage(
                                                                  quint8 extensionId,
                                                                  quint8 resultCode,
                                                                  quint32 clientReference)
    {
        QByteArray message;
        message.reserve(2 + 2 + 4);
        NetworkProtocol::append2Bytes(message, ServerMessageType::ExtensionResultMessage);
        NetworkUtil::appendByte(message, extensionId);
        NetworkUtil::appendByte(message, resultCode);
        NetworkUtil::append4Bytes(message, clientReference);

        return message;
    }

    quint16 NetworkProtocolExtensionMessages::encodeMessageTypeForExtension(
                                                                       quint8 extensionId,
                                                                       quint8 messageType)
    {
        return (1u << 15) + (extensionId << 7) + (messageType & 0x7Fu);
    }
}

/*
    Copyright (C) 2015-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "networkprotocol.h"

#include "common/util.h"

#include "filehash.h"
#include "networkutil.h"

#include <limits>

#include <QCryptographicHash>
#include <QtDebug>

namespace PMP
{
    void NetworkProtocol::append2Bytes(QByteArray& buffer, ServerMessageType messageType)
    {
        NetworkUtil::append2Bytes(buffer, static_cast<quint16>(messageType));
    }

    void NetworkProtocol::append2Bytes(QByteArray& buffer, ClientMessageType messageType)
    {
        NetworkUtil::append2Bytes(buffer, static_cast<quint16>(messageType));
    }

    void NetworkProtocol::append2Bytes(QByteArray& buffer,
                                       ResultMessageErrorCode errorCode)
    {
        NetworkUtil::append2Bytes(buffer, static_cast<quint16>(errorCode));
    }

    quint16 NetworkProtocol::encodeMessageTypeForExtension(quint8 extensionId,
                                                           quint8 messageType)
    {
        return (1u << 15) + (extensionId << 7) + (messageType & 0x7Fu);
    }

    void NetworkProtocol::appendExtensionMessageStart(QByteArray& buffer,
                                                      quint8 extensionId,
                                                      quint8 messageType)
    {
        quint16 encodedMessageType =
                encodeMessageTypeForExtension(extensionId, messageType);

        NetworkUtil::append2Bytes(buffer, encodedMessageType);
    }

    int NetworkProtocol::ratePassword(QString password)
    {
        int rating = 0;
        for (int i = 0; i < password.size(); ++i)
        {
            rating += 3;
            QChar c = password[i];

            if (c.isDigit())
            {
                rating += 1; /* digits are slightly better than lowercase letters */
            }
            else if (c.isLetter())
            {
                if (c.isLower())
                {
                    /* lowercase letters worth the least */
                }
                else
                {
                    rating += 2; /* uppercase letters are better than digits */
                }
            }
            else
            {
                rating += 7;
            }
        }

        int lastDiff = 0;
        int diffConstantCount = 0;
        for (int i = 1; i < password.size(); ++i)
        {
            QChar prevC = password[i - 1];
            int prevN = prevC.unicode();
            QChar curC = password[i];
            int curN = curC.unicode();

            /* punish patterns such as "eeeee", "123456", "98765", "ghijklm" etc... */
            int diff = curN - prevN;
            if (diff <= 1 && diff >= -1)
            {
                rating -= 1;
            }
            if (diff == lastDiff)
            {
                diffConstantCount += 1;
                rating -= diffConstantCount;
            }
            else
            {
                diffConstantCount = 0;
            }

            lastDiff = diff;
        }

        return rating;
    }

    QByteArray NetworkProtocol::hashPassword(QByteArray const& salt, QString password)
    {
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        hasher.addData(salt);
        hasher.addData(password.toUtf8());
        return hasher.result();
    }

    QByteArray NetworkProtocol::hashPasswordForSession(const QByteArray& userSalt,
                                                       const QByteArray& sessionSalt,
                                                       QString password)
    {
        QByteArray hashedSaltedUserPassword = hashPassword(userSalt, password);
        return hashPasswordForSession(sessionSalt, hashedSaltedUserPassword);
    }

    QByteArray NetworkProtocol::hashPasswordForSession(const QByteArray& sessionSalt,
                                               const QByteArray& hashedSaltedUserPassword)
    {
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        hasher.addData(sessionSalt);
        hasher.addData(hashedSaltedUserPassword);
        return hasher.result();
    }

    quint16 NetworkProtocol::createTrackStatusForTrack()
    {
        return 0; /* TODO: make this depend on track load/error/... status */
    }

    quint16 NetworkProtocol::createTrackStatusFor(SpecialQueueItemType itemType)
    {
        switch (itemType)
        {
        case PMP::SpecialQueueItemType::Break:
            return (1u << 15) + 1;

        case PMP::SpecialQueueItemType::Barrier:
            return (1u << 15) + 2;
        }

        qWarning() << "Unhandled SpecialQueueItemType" << itemType;
        return createTrackStatusForUnknownThing();
    }

    quint16 NetworkProtocol::createTrackStatusUnknownId()
    {
        return 0xFFFFu;
    }

    quint16 NetworkProtocol::createTrackStatusForUnknownThing()
    {
        return 0xFFFEu;
    }

    bool NetworkProtocol::isTrackStatusFromRealTrack(quint16 status)
    {
        return (status & (1u << 15)) == 0;
    }

    QString NetworkProtocol::getPseudoTrackStatusText(quint16 status)
    {
        if (isTrackStatusFromRealTrack(status)) return "<< Real track >>";

        auto type = trackStatusToQueueEntryType(status);
        switch (type)
        {
        case PMP::QueueEntryType::Unknown:
        case PMP::QueueEntryType::Track:
            break; /* these are not expected */

        case PMP::QueueEntryType::BreakPoint:
            return "<<<<< BREAK >>>>>";

        case PMP::QueueEntryType::Barrier:
            return "<<<< BARRIER >>>>";

        case PMP::QueueEntryType::UnknownSpecialType:
            return "<<<<< ????? >>>>>";
        }

        qWarning() << "Unhandled/unexpected QueueEntryType" << type;
        return "<<<< ??????? >>>>";
    }

    QueueEntryType NetworkProtocol::trackStatusToQueueEntryType(quint16 status)
    {
        if (isTrackStatusFromRealTrack(status))
            return QueueEntryType::Track;

        auto type = status - (1u << 15);
        if (type == 1) return QueueEntryType::BreakPoint;
        if (type == 2) return QueueEntryType::Barrier;

        return QueueEntryType::UnknownSpecialType;
    }

    const QByteArray NetworkProtocol::_fileHashAllZeroes =
            Util::generateZeroedMemory(FILEHASH_BYTECOUNT);

    void NetworkProtocol::appendHash(QByteArray& buffer, Nullable<FileHash> hash)
    {
        if (hash.isNull())
        {
            buffer += _fileHashAllZeroes;
            return;
        }

        auto hashValue = hash.value();
        if (hashValue.isNull())
        {
            buffer += _fileHashAllZeroes;
            return;
        }

        NetworkUtil::append8Bytes(buffer, hashValue.length());
        buffer += hashValue.SHA1();
        buffer += hashValue.MD5();
        // TODO: check if the length of what we added matches FILEHASH_BYTECOUNT
    }

    FileHash NetworkProtocol::getHash(const QByteArray& buffer, int position, bool* ok)
    {
        if (position > buffer.size() - FILEHASH_BYTECOUNT)
        {
            /* not enough bytes to read */
            qWarning() << "NetworkProtocol::getHash: ERROR: not enough bytes to read";
            if (ok) *ok = false;
            return FileHash();
        }

        quint64 lengthPart = NetworkUtil::get8Bytes(buffer, position);
        if (lengthPart == 0
                && buffer.mid(position, FILEHASH_BYTECOUNT) == _fileHashAllZeroes)
        { /* this is how an empty hash is transmitted */
            if (ok) *ok = true;
            return FileHash();
        }
        position += 8;

        if (lengthPart > std::numeric_limits<uint>::max())
        {
            /* conversion of 64-bit number to platform-specific uint would truncate */
            qWarning() << "NetworkProtocol::getHash: ERROR: length overflow";
            if (ok) *ok = false;
            return FileHash();
        }

        QByteArray sha1Data = buffer.mid(position, 20);
        position += 20;
        QByteArray md5Data = buffer.mid(position, 16);

        if (ok) *ok = true;
        return FileHash(lengthPart, sha1Data, md5Data);
    }

    qint16 NetworkProtocol::getHashUserDataFieldsMaskForProtocolVersion(int version)
    {
        if (version < 3) return 1; /* only previously heard */

        return 3; /* previously heard & score */
    }
}

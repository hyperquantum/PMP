/*
    Copyright (C) 2015-2024, Kevin Andre <hyperquantum@gmail.com>

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
#include "obfuscator.h"
#include "scrobblingprovider.h"

#include <limits>

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QtDebug>

namespace PMP
{
    bool NetworkProtocol::isSupported(ParameterlessActionCode action, int protocolVersion)
    {
        auto minimumProtocolVersion = getMinimumProtocolVersionThatSupports(action);
        if (minimumProtocolVersion < 0)
        {
            qWarning() << "ParameterlessActionCode" << int(action) << "is invalid";
            return false;
        }

        return protocolVersion >= minimumProtocolVersion;
    }

    bool PMP::NetworkProtocol::isSupported(ServerEventCode eventCode, int protocolVersion)
    {
        auto minimumProtocolVersion = getMinimumProtocolVersionThatSupports(eventCode);
        if (minimumProtocolVersion < 0)
        {
            qWarning() << "ServerEventCode" << int(eventCode) << "is invalid";
            return false;
        }

        return protocolVersion >= minimumProtocolVersion;
    }

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

    quint8 NetworkProtocol::encode(StartStopEventStatus status)
    {
        switch (status)
        {
        case StartStopEventStatus::Undefined:
            return 0;
        case StartStopEventStatus::StatusUnchangedNotActive:
            return 1;
        case StartStopEventStatus::StatusUnchangedActive:
            return 2;
        case StartStopEventStatus::StatusChangedToActive:
            return 3;
        case StartStopEventStatus::StatusChangedToNotActive:
            return 4;
        }

        return 0;
    }

    StartStopEventStatus NetworkProtocol::decodeStartStopEventStatus(quint8 status)
    {
        switch (status)
        {
        case 0:
            return StartStopEventStatus::Undefined;
        case 1:
            return StartStopEventStatus::StatusUnchangedNotActive;
        case 2:
            return StartStopEventStatus::StatusUnchangedActive;
        case 3:
            return StartStopEventStatus::StatusChangedToActive;
        case 4:
            return StartStopEventStatus::StatusChangedToNotActive;
        default:
            return StartStopEventStatus::Undefined;
        }
    }

    quint8 NetworkProtocol::encode(ScrobblingProvider provider)
    {
        switch (provider)
        {
            case ScrobblingProvider::Unknown:
                return 0;
            case ScrobblingProvider::LastFm:
                return 1;
        }

        return 0;
    }

    ScrobblingProvider NetworkProtocol::decodeScrobblingProvider(quint8 provider)
    {
        switch (provider)
        {
            case 1:
                return ScrobblingProvider::LastFm;
            case 0:
            default:
                return ScrobblingProvider::Unknown;
        }
    }

    quint8 NetworkProtocol::encode(ScrobblerStatus status)
    {
        switch (status)
        {
            case ScrobblerStatus::Green:
                return 1;
            case ScrobblerStatus::Yellow:
                return 2;
            case ScrobblerStatus::Red:
                return 3;
            case ScrobblerStatus::WaitingForUserCredentials:
                return 4;
            case ScrobblerStatus::Unknown:
                return 0;
        }

        return 0;
    }

    ScrobblerStatus NetworkProtocol::decodeScrobblerStatus(quint8 status)
    {
        switch (status)
        {
            case 1:
                return ScrobblerStatus::Green;
            case 2:
                return ScrobblerStatus::Yellow;
            case 3:
                return ScrobblerStatus::Red;
            case 4:
                return ScrobblerStatus::WaitingForUserCredentials;
            case 0:
            default:
                return ScrobblerStatus::Unknown;
        }
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

        auto oldBufferLength = buffer.length();

        NetworkUtil::append8Bytes(buffer, hashValue.length());
        buffer += hashValue.SHA1();
        buffer += hashValue.MD5();

        auto newBufferLength = buffer.length();
        Q_ASSERT_X(newBufferLength - oldBufferLength == FILEHASH_BYTECOUNT,
                   "NetworkProtocol::appendHash",
                   "wrong number of bytes for a file hash");
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

        return 1 | 2; /* previously heard & score */
    }

    ObfuscatedScrobblingUsernameAndPassword
    NetworkProtocol::obfuscateScrobblingCredentials(UsernameAndPassword credentials)
    {
        auto* randomGenerator = QRandomGenerator::global();

        auto keyId = randomGenerator->bounded(14);
        auto key = getScrobblingAuthenticationObfuscationKey(keyId);

        auto usernameBytes = credentials.username.toUtf8();
        auto passwordBytes = credentials.password.toUtf8();

        qint32 usernameByteCount = usernameBytes.size();
        qint32 passwordByteCount = passwordBytes.size();

        QByteArray bytes = usernameBytes;
        bytes += passwordBytes;
        NetworkUtil::append4Bytes(bytes, randomGenerator->generate());
        NetworkUtil::append4BytesSigned(bytes, usernameByteCount);
        NetworkUtil::append4BytesSigned(bytes, passwordByteCount);

        Obfuscator obfuscator(key);
        auto obfuscated = obfuscator.encrypt(bytes);

        ObfuscatedScrobblingUsernameAndPassword result;
        result.keyId = keyId;
        result.bytes = obfuscated;

        return result;
    }

    Nullable<UsernameAndPassword> NetworkProtocol::deobfuscateScrobblingCredentials(
                            ObfuscatedScrobblingUsernameAndPassword obfuscatedCredentials)
    {
        auto key = getScrobblingAuthenticationObfuscationKey(obfuscatedCredentials.keyId);
        if (key == 0)
            return null;

        Obfuscator obfuscator(key);
        auto deobfuscatedBytes = obfuscator.decrypt(obfuscatedCredentials.bytes);
        if (deobfuscatedBytes.size() < 12)
            return null;

        qint32 usernameByteCount =
            NetworkUtil::get4BytesSigned(deobfuscatedBytes, deobfuscatedBytes.size() - 8);
        qint32 passwordByteCount =
            NetworkUtil::get4BytesSigned(deobfuscatedBytes, deobfuscatedBytes.size() - 4);

        auto usernameBytes = deobfuscatedBytes.mid(0, usernameByteCount);
        auto passwordBytes = deobfuscatedBytes.mid(usernameByteCount, passwordByteCount);

        UsernameAndPassword credentials;
        credentials.username = QString::fromUtf8(usernameBytes);
        credentials.password = QString::fromUtf8(passwordBytes);

        return credentials;
    }

    int NetworkProtocol::getMinimumProtocolVersionThatSupports(
                                                        ParameterlessActionCode action)
    {
        switch (action)
        {
        case ParameterlessActionCode::Reserved:
            return -1; /* invalid value, not supported */

        case ParameterlessActionCode::ReloadServerSettings:
            return 15;

        case ParameterlessActionCode::DeactivateDelayedStart:
            return 20;

        case ParameterlessActionCode::StartFullIndexation:
        case ParameterlessActionCode::StartQuickScanForNewFiles:
            return 26;
        }

        return -1; /* invalid value, not supported */
    }

    int NetworkProtocol::getMinimumProtocolVersionThatSupports(ServerEventCode eventCode)
    {
        switch (eventCode)
        {
        case ServerEventCode::Reserved:
            return -1; /* invalid value, not supported */

        case ServerEventCode::FullIndexationRunning:
        case ServerEventCode::FullIndexationNotRunning:
            return 1; /* was supported before we started incrementing protocol version */
        }

        return -1; /* invalid value, not supported */
    }

    quint64 NetworkProtocol::getScrobblingAuthenticationObfuscationKey(quint8 keyId)
    {
        switch (keyId)
        {
        case 0: return 0x4313523b3fe841adULL;
        case 1: return 0xae7e6d9609a57bd1ULL;
        case 2: return 0x412c721f46eb4bdbULL;
        case 3: return 0x2e618d6e4c77ee06ULL;
        case 4: return 0x2e9e0ab8ecc52b83ULL;
        case 5: return 0xf54cbce59bdea005ULL;
        case 6: return 0xb8425247aeb773c3ULL;
        case 7: return 0xe5f480813d6dfa58ULL;
        case 8: return 0xd33163532fd46022ULL;
        case 9: return 0xbda9d7d719e84220ULL;
        case 10: return 0xeb8b46d138fc19f9ULL;
        case 11: return 0x6a7a0b00bc3d051cULL;
        case 12: return 0x63c70b5efceb8b53ULL;
        case 13: return 0xebf1effcacb3b7caULL;
        }

        qWarning() << "unknown scrobbling authentication key ID:" << keyId;
        return 0;
    }

}

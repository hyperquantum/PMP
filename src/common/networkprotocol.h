/*
    Copyright (C) 2015-2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_NETWORKPROTOCOL_H
#define PMP_NETWORKPROTOCOL_H

#include "common/queueentrytype.h"
#include "common/scrobblerstatus.h"
#include "common/scrobblingprovider.h"
#include "common/startstopeventstatus.h"

#include <QByteArray>
#include <QString>

namespace PMP
{
    /*
        Network protocol versions
        =========================

        Changes for each version:

          1: first version, no version increments for a long time
          2: first increment for test purposes, no strict incrementing yet after this
          3: client msg 18, server msg 22: added support for retrieving scores
          4: client msg 19: added support for inserting a track at a specific index
          5: client msg 20, server msg 23 & 24: added player history fetching
          6: single byte request 17 & server msg 25: retrieving the database identifier
          7: server msgs 18 & 19 extended with album and track length
          8: server msg 26 and single byte request 24 for dynamic mode waves
          9: client msg 21, server msg 27: queue entry duplication
         10: single byte request 51 & server msg 28: server health messages
         11: server msg 29: track availability change notifications
         12: clienst msg 22, server msg 30, single byte request 18: protocol extensions
         13: server msgs 3 & 4: change track length to milliseconds
         14: single byte request 25 & server msg 26: wave termination & progress

    */

    class FileHash;

    enum class ServerMessageType
    {
        None = 0,
        PlayerStateMessage = 1,
        VolumeChangedMessage = 2,
        TrackInfoMessage = 3,
        BulkTrackInfoMessage = 4,
        QueueContentsMessage = 5,
        QueueEntryRemovedMessage = 6,
        QueueEntryAddedMessage = 7,
        DynamicModeStatusMessage = 8,
        PossibleFilenamesForQueueEntryMessage = 9,
        ServerInstanceIdentifierMessage = 10,
        QueueEntryMovedMessage = 11,
        UsersListMessage = 12,
        NewUserAccountSaltMessage = 13,
        SimpleResultMessage = 14,
        UserLoginSaltMessage = 15,
        UserPlayingForModeMessage = 16,
        ServerEventNotificationMessage = 17,
        CollectionFetchResponseMessage = 18,
        CollectionChangeNotificationMessage = 19,
        ServerNameMessage = 20,
        BulkQueueEntryHashMessage = 21,
        HashUserDataMessage = 22,
        NewHistoryEntryMessage = 23,
        PlayerHistoryMessage = 24,
        DatabaseIdentifierMessage = 25,
        DynamicModeWaveStatusMessage = 26,
        QueueEntryAdditionConfirmationMessage = 27,
        ServerHealthMessage = 28,
        CollectionAvailabilityChangeNotificationMessage = 29,
        ServerExtensionsMessage = 30,
    };

    enum class ClientMessageType
    {
        None = 0,
        SingleByteActionMessage = 1,
        TrackInfoRequestMessage = 2,
        BulkTrackInfoRequestMessage = 3,
        QueueFetchRequestMessage = 4,
        QueueEntryRemovalRequestMessage = 5,
        GeneratorNonRepetitionChangeMessage = 6,
        PossibleFilenamesForQueueEntryRequestMessage = 7,
        PlayerSeekRequestMessage = 8,
        QueueEntryMoveRequestMessage = 9,
        InitiateNewUserAccountMessage = 10,
        FinishNewUserAccountMessage = 11,
        InitiateLoginMessage = 12,
        FinishLoginMessage = 13,
        CollectionFetchRequestMessage = 14,
        AddHashToEndOfQueueRequestMessage = 15,
        AddHashToFrontOfQueueRequestMessage = 16,
        BulkQueueEntryHashRequestMessage = 17,
        HashUserDataRequestMessage = 18,
        InsertHashIntoQueueRequestMessage = 19,
        PlayerHistoryRequestMessage = 20,
        QueueEntryDuplicationRequestMessage = 21,
        ClientExtensionsMessage = 22,
    };

    enum class ResultMessageErrorCode
    {
        NoError = 0,
        InvalidMessageStructure = 1,
        NotLoggedIn = 10,

        InvalidUserAccountName = 11,
        UserAccountAlreadyExists = 12,
        UserAccountRegistrationMismatch = 13,
        UserAccountLoginMismatch = 14,
        UserLoginAuthenticationFailed = 15,
        AlreadyLoggedIn = 16,

        QueueIdNotFound = 20,

        DatabaseProblem = 90,
        NonFatalInternalServerError = 254,
        UnknownError = 255
    };

    class NetworkProtocol
    {
    public:
        struct ProtocolExtensionSupport
        {
            quint8 id;
            quint8 version;

            ProtocolExtensionSupport()
             : id(0), version(0)
            {
                //
            }

            explicit ProtocolExtensionSupport(quint8 id, quint8 version = 1)
             : id(id), version(version)
            {
                //
            }
        };

        struct ProtocolExtension : ProtocolExtensionSupport
        {
            QString name;

            ProtocolExtension()
             : ProtocolExtensionSupport()
            {
                //
            }

            ProtocolExtension(quint8 id, QString name, quint8 version = 1)
             : ProtocolExtensionSupport(id, version), name(name)
            {
                //
            }
        };

        enum class ScrobblingServerMessage : quint8 {
            ProviderInfoMessage = 1,
            StatusChangeMessage = 2,
            ProviderEnabledChangeMessage = 3,
        };

        enum class ScrobblingClientMessage : quint8 {
            ProviderInfoRequestMessage = 1,
            EnableDisableRequestMessage = 2,
        };

        static void append2Bytes(QByteArray& buffer, ServerMessageType messageType);
        static void append2Bytes(QByteArray& buffer, ClientMessageType messageType);
        static void append2Bytes(QByteArray& buffer, ResultMessageErrorCode errorCode);

        static quint16 encodeMessageTypeForExtension(quint8 extensionId,
                                                     quint8 messageType);
        static void appendExtensionMessageStart(QByteArray& buffer, quint8 extensionId,
                                                quint8 messageType);

        static quint8 encode(ScrobblingProvider provider);
        static ScrobblingProvider decodeScrobblingProvider(quint8 provider);

        static quint8 encode(ScrobblerStatus status);
        static ScrobblerStatus decodeScrobblerStatus(quint8 status);

        static int ratePassword(QString password);

        static QByteArray hashPassword(QByteArray const& salt, QString password);
        static QByteArray hashPasswordForSession(QByteArray const& userSalt,
                                                 QByteArray const& sessionSalt,
                                                 QString password);
        static QByteArray hashPasswordForSession(QByteArray const& sessionSalt,
                                              QByteArray const& hashedSaltedUserPassword);

        static quint16 createTrackStatusForTrack();
        static quint16 createTrackStatusUnknownId();
        static quint16 createTrackStatusForBreakPoint();

        static bool isTrackStatusFromRealTrack(quint16 status);
        static QString getPseudoTrackStatusText(quint16 status);
        static QueueEntryType trackStatusToQueueEntryType(quint16 status);

        static const int FILEHASH_BYTECOUNT = 8 /*length*/ + 20 /*SHA-1*/ + 16 /*MD5*/;
        static void appendHash(QByteArray& buffer, const FileHash& hash);
        static FileHash getHash(const QByteArray& buffer, int position, bool* ok);

        static qint16 getHashUserDataFieldsMaskForProtocolVersion(int version);

    private:
        static const QByteArray _fileHashAllZeroes;
    };
}
#endif

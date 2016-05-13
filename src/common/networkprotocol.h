/*
    Copyright (C) 2015-2016, Kevin Andre <hyperquantum@gmail.com>

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

#include <QByteArray>
#include <QString>

namespace PMP {

    class FileHash;

    enum class QueueEntryType : quint8 {
        Unknown = 0,
        Track = 1,
        BreakPoint = 10,
    };

    class NetworkProtocol {
    public:
        enum ServerMessageType {
            ServerMessageTypeNone = 0,
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
        };

        enum ClientMessageType {
            ClientMessageTypeNone = 0,
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
        };

        enum ErrorType {
            NoError = 0,
            InvalidMessageStructure = 1,
            NotLoggedIn = 10,

            InvalidUserAccountName = 11,
            UserAccountAlreadyExists = 12,
            UserAccountRegistrationMismatch = 13,
            UserAccountLoginMismatch = 14,
            UserLoginAuthenticationFailed = 15,
            AlreadyLoggedIn = 16,

            DatabaseProblem = 90,
            UnknownError = 255
        };

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
        static FileHash getHash(const QByteArray& buffer, uint position, bool* ok);
    };
}
#endif

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

#include "compatibilityui.h"
#include "queueentrytype.h"
#include "startstopeventstatus.h"

#include <QByteArray>
#include <QString>

namespace PMP {

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
         15: single byte request 52, server msgs 31-36, client msgs 23-24: compatibility interfaces

    */

    class CompatibilityUiState;
    class FileHash;

    class NetworkProtocol
    {
    public:
        enum ServerMessageType
        {
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
            NewHistoryEntryMessage = 23,
            PlayerHistoryMessage = 24,
            DatabaseIdentifierMessage = 25,
            DynamicModeWaveStatusMessage = 26,
            QueueEntryAdditionConfirmationMessage = 27,
            ServerHealthMessage = 28,
            CollectionAvailabilityChangeNotificationMessage = 29,
            ServerExtensionsMessage = 30,
            CompatibilityInterfaceAnnouncement = 31,
            CompatibilityInterfaceDefinition = 32,
            CompatibilityInterfaceStateUpdate = 33,
            CompatibilityInterfaceActionStateUpdate = 34,
            CompatibilityInterfaceTextUpdate = 35,
            CompatibilityInterfaceActionTextUpdate = 36,
        };

        enum ClientMessageType
        {
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
            InsertHashIntoQueueRequestMessage = 19,
            PlayerHistoryRequestMessage = 20,
            QueueEntryDuplicationRequestMessage = 21,
            ClientExtensionsMessage = 22,
            CompatibilityInterfaceDefinitionsRequest = 23,
            CompatibilityInterfaceTriggerActionRequest = 24,
        };

        enum ErrorType
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

        struct ProtocolExtensionSupport {
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

        struct ProtocolExtension : ProtocolExtensionSupport {
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

        class CompatibilityUserInterfaceAction
        {
        public:
            CompatibilityUserInterfaceAction()
             : _id(-1),
               _visible(false),
               _enabled(false),
               _disableWhenTriggered(false)
            {
                //
            }

            CompatibilityUserInterfaceAction(int id, QString caption, bool visible,
                                             bool enabled, bool disableWhenTriggered)
             : _id(id),
               _caption(caption),
               _visible(visible),
               _enabled(enabled),
               _disableWhenTriggered(disableWhenTriggered)
            {
                //
            }

            int id() const { return _id; }
            QString caption() const { return _caption; }
            bool visible() const { return _visible; }
            bool enabled() const { return _enabled; }
            bool disableWhenTriggered() const { return _disableWhenTriggered; }

        private:
            int _id;
            QString _caption;
            bool _visible;
            bool _enabled;
            bool _disableWhenTriggered;
        };

        class CompatibilityUserInterface
        {
        public:
            CompatibilityUserInterface(int id, UserInterfaceLanguage language,
                                       QString title, QString caption,
                                       QString description,
                                       QVector<CompatibilityUserInterfaceAction> actions)
             : _id(id),
               _language(language),
               _title(title),
               _caption(caption),
               _description(description),
               _actions(actions)
            {
                //
            }

            int id() const { return _id; }
            UserInterfaceLanguage language() const { return _language; }
            QString title() const { return _title; }
            QString caption() const { return _caption; }
            QString description() const { return _description; }
            QVector<CompatibilityUserInterfaceAction> const& actions() const {
                                                                        return _actions; }

        private:
            int _id;
            UserInterfaceLanguage _language;
            QString _title;
            QString _caption;
            QString _description;
            QVector<CompatibilityUserInterfaceAction> _actions;
        };

        static quint16 encodeMessageTypeForExtension(quint8 extensionId,
                                                     quint8 messageType);
        static void appendExtensionMessageStart(QByteArray& buffer, quint8 extensionId,
                                                quint8 messageType);

        static QByteArray encodeLanguage(UserInterfaceLanguage language);
        static UserInterfaceLanguage decodeLanguage(QByteArray isoCode);

        static quint16 encodeCompatibilityUiState(CompatibilityUiState const& state);
        static CompatibilityUiState decodeCompatibilityUiState(quint16 state);

        static quint8 encodeCompatibilityUiActionState(
                                                 CompatibilityUiActionState const& state);
        static CompatibilityUiActionState decodeCompatibilityUiActionState(quint8 state);

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

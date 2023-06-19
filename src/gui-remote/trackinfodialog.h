/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_TRACKINFODIALOG_H
#define PMP_TRACKINFODIALOG_H

#include "client/authenticationcontroller.h"
#include "client/collectiontrackinfo.h"

#include <QDateTime>
#include <QDialog>
#include <QList>
#include <QTimer>

namespace Ui
{
    class TrackInfoDialog;
}

namespace PMP::Client
{
    class ServerInterface;
}

namespace PMP
{
    class TrackInfoDialog : public QDialog
    {
        Q_OBJECT
    public:
        TrackInfoDialog(QWidget* parent,
                        Client::ServerInterface* serverInterface,
                        Client::LocalHashId hashId, quint32 queueId = 0);

        TrackInfoDialog(QWidget* parent,
                        Client::ServerInterface* serverInterface,
                        Client::CollectionTrackInfo const& track);

        ~TrackInfoDialog();

    private Q_SLOTS:
        void newTrackReceived(Client::CollectionTrackInfo track);
        void trackDataChanged(Client::CollectionTrackInfo track);
        void dataReceivedForUser(quint32 userId);
        void updateLastHeard();

    private:
        void init();
        void fillUserComboBox(QList<Client::UserAccount> accounts);

        void enableDisableButtons();

        void fillQueueId();
        void fillHash();
        void fillTrackDetails(Client::CollectionTrackInfo const& trackInfo);
        void fillUserData(Client::LocalHashId hashId, quint32 userId);
        void clearTrackDetails();
        void clearUserData();

        Ui::TrackInfoDialog* _ui;
        Client::ServerInterface* _serverInterface;
        QTimer* _lastHeardUpdateTimer;
        Client::LocalHashId _trackHashId;
        QDateTime _lastHeard;
        quint32 _queueId { 0 };
        quint32 _userId { 0 };
        bool _updatingUsersList { false };
    };
}
#endif

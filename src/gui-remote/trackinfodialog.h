/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/collectiontrackinfo.h"
#include "common/filehash.h"

#include <QDialog>

namespace Ui {
    class TrackInfoDialog;
}

namespace PMP {

    class ClientServerInterface;
    class FileHash;

    class TrackInfoDialog : public QDialog {
        Q_OBJECT
    public:
        TrackInfoDialog(QWidget* parent, FileHash const& hash,
                        ClientServerInterface* clientServerInterface);

        TrackInfoDialog(QWidget* parent, CollectionTrackInfo const& track,
                        ClientServerInterface* clientServerInterface);

        ~TrackInfoDialog();

    private Q_SLOTS:
        void newTrackReceived(CollectionTrackInfo track);
        void trackDataChanged(CollectionTrackInfo track);
        void dataReceivedForUser(quint32 userId);

    private:
        void init();

        void fillHash(const FileHash& hash);
        void fillTrackDetails(CollectionTrackInfo const& trackInfo);
        void fillUserData(const FileHash& hash);
        void clearTrackDetails();
        void clearUserData();

        Ui::TrackInfoDialog* _ui;
        ClientServerInterface* _clientServerInterface;
        FileHash _trackHash;
    };
}
#endif

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

#include "trackinfodialog.h"
#include "ui_trackinfodialog.h"

#include "common/clientserverinterface.h"
#include "common/collectiontrackinfo.h"
#include "common/collectionwatcher.h"
#include "common/userdatafetcher.h"
#include "common/util.h"

#include <QLocale>
#include <QtDebug>

namespace PMP {

    TrackInfoDialog::TrackInfoDialog(QWidget* parent, const FileHash& hash,
                                     ClientServerInterface* clientServerInterface)
     : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
        _ui(new Ui::TrackInfoDialog),
        _clientServerInterface(clientServerInterface),
        _trackHash(hash)
    {
        init();

        auto trackInfo = clientServerInterface->collectionWatcher().getTrack(hash);

        if (trackInfo.hash().isNull()) { /* not found? */
            fillHash(hash);
            clearTrackDetails();
        }
        else {
            fillHash(trackInfo.hash());
            fillTrackDetails(trackInfo);
        }

        fillUserData(_trackHash);
    }

    TrackInfoDialog::TrackInfoDialog(QWidget* parent, const CollectionTrackInfo& track,
                                     ClientServerInterface* clientServerInterface)
     : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
        _ui(new Ui::TrackInfoDialog),
        _clientServerInterface(clientServerInterface),
        _trackHash(track.hash())
    {
        init();

        fillHash(track.hash());
        fillTrackDetails(track);
        fillUserData(_trackHash);
    }

    TrackInfoDialog::~TrackInfoDialog()
    {
        qDebug() << "TrackInfoDialog being destructed";
        delete _ui;
    }

    void TrackInfoDialog::newTrackReceived(CollectionTrackInfo track)
    {
        if (track.hash() != _trackHash)
            return;

        fillTrackDetails(track);
    }

    void TrackInfoDialog::trackDataChanged(CollectionTrackInfo track)
    {
        if (track.hash() != _trackHash)
            return;

        fillTrackDetails(track);
    }

    void TrackInfoDialog::dataReceivedForUser(quint32 userId)
    {
        if (userId != _clientServerInterface->userLoggedInId())
            return;

        fillUserData(_trackHash);
    }

    void TrackInfoDialog::init()
    {
        _ui->setupUi(this);

        connect(
            &_clientServerInterface->collectionWatcher(),
            &CollectionWatcher::newTrackReceived,
            this, &TrackInfoDialog::newTrackReceived
        );

        connect(
            &_clientServerInterface->collectionWatcher(),
            &CollectionWatcher::trackDataChanged,
            this, &TrackInfoDialog::trackDataChanged
        );

        connect(
            &_clientServerInterface->userDataFetcher(),
            &UserDataFetcher::dataReceivedForUser,
            this, &TrackInfoDialog::dataReceivedForUser
        );
    }

    void TrackInfoDialog::fillHash(const FileHash& hash)
    {
        QString hashText =
                QString::number(hash.length())
                    + Util::FigureDash + hash.SHA1().toHex()
                    + Util::FigureDash + hash.MD5().toHex();

        _ui->hashValueLabel->setText(hashText);
    }

    void TrackInfoDialog::fillTrackDetails(const CollectionTrackInfo& trackInfo)
    {
        _ui->titleValueLabel->setText(trackInfo.title());
        _ui->artistValueLabel->setText(trackInfo.artist());
        _ui->albumValueLabel->setText(trackInfo.album());

        QString lengthText;
        if (trackInfo.lengthIsKnown()) {
            auto length = trackInfo.lengthInMilliseconds();
            lengthText = Util::millisecondsToLongDisplayTimeText(length);
        }
        else {
            lengthText = tr("unknown");
        }

        _ui->lengthValueLabel->setText(lengthText);
    }

    void TrackInfoDialog::fillUserData(const FileHash& hash)
    {
        if (!_clientServerInterface->isLoggedIn()) {
            _ui->usernameValueLabel->clear();
            clearUserData();
            return;
        }

        _ui->usernameValueLabel->setText(_clientServerInterface->userLoggedInName());

        auto userId = _clientServerInterface->userLoggedInId();
        auto userData =
               _clientServerInterface->userDataFetcher().getHashDataForUser(userId, hash);

        if (userData == nullptr) {
            clearUserData();
            return;
        }

        QLocale locale;

        QString lastHeardText;
        if (!userData->previouslyHeardReceived) {
            lastHeardText = tr("unknown");
        }
        else if (userData->previouslyHeard.isNull()) {
            lastHeardText = tr("never");
        }
        else {
            lastHeardText = locale.toString(userData->previouslyHeard.toLocalTime());
        }

        _ui->lastHeardValueLabel->setText(lastHeardText);

        QString scoreText;
        if (!userData->scoreReceived) {
            scoreText = tr("unknown");
        }
        else if (userData->scorePermillage < 0) {
            scoreText = tr("no score yet");
        }
        else {
            scoreText = locale.toString(userData->scorePermillage / 10.0f, 'f', 1);
        }

        _ui->scoreValueLabel->setText(scoreText);
    }

    void TrackInfoDialog::clearTrackDetails()
    {
        _ui->titleValueLabel->clear();
        _ui->artistValueLabel->clear();
        _ui->albumValueLabel->clear();
        _ui->lengthValueLabel->clear();
    }

    void TrackInfoDialog::clearUserData()
    {
        _ui->lastHeardValueLabel->clear();
        _ui->scoreValueLabel->clear();
    }
}

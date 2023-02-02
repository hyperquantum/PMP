/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/collectiontrackinfo.h"
#include "common/unicodechars.h"
#include "common/util.h"

#include "client/collectionwatcher.h"
#include "client/generalcontroller.h"
#include "client/queuecontroller.h"
#include "client/serverinterface.h"
#include "client/userdatafetcher.h"

#include <QApplication>
#include <QClipboard>
#include <QLocale>
#include <QtDebug>

using namespace PMP::Client;

namespace PMP
{
    TrackInfoDialog::TrackInfoDialog(QWidget* parent,
                                     ServerInterface* serverInterface,
                                     const FileHash& hash,
                                     quint32 queueId)
     : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
        _ui(new Ui::TrackInfoDialog),
        _serverInterface(serverInterface),
        _lastHeardUpdateTimer(new QTimer(this)),
        _trackHash(hash),
        _queueId(queueId)
    {
        init();

        fillQueueId();
        fillHash();

        auto trackInfo = serverInterface->collectionWatcher().getTrack(hash);

        if (trackInfo.hash().isNull())
        { /* not found? */
            clearTrackDetails();
        }
        else
        {
            fillTrackDetails(trackInfo);
        }

        fillUserData(_trackHash);
    }

    TrackInfoDialog::TrackInfoDialog(QWidget* parent,
                                     ServerInterface* serverInterface,
                                     const CollectionTrackInfo& track)
     : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
        _ui(new Ui::TrackInfoDialog),
        _serverInterface(serverInterface),
        _lastHeardUpdateTimer(new QTimer(this)),
        _trackHash(track.hash()),
        _queueId(0)
    {
        init();

        fillHash();
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
        if (userId != _serverInterface->userLoggedInId())
            return;

        fillUserData(_trackHash);
    }

    void TrackInfoDialog::updateLastHeard()
    {
        if (_lastHeard.isNull())
        {
            _lastHeardUpdateTimer->stop();
            return;
        }

        auto clientClockTimeOffsetMs =
                _serverInterface->generalController().clientClockTimeOffsetMs();

        auto adjustedLastHeard = _lastHeard.addMSecs(clientClockTimeOffsetMs);

        auto howLongAgo = Util::getHowLongAgoInfo(adjustedLastHeard);

        QLocale locale;

        QString lastHeardText =
            QString("%1 - %2")
                .replace('-', UnicodeChars::emDash)
                .arg(howLongAgo.text(),
                     locale.toString(adjustedLastHeard.toLocalTime()));

        _ui->lastHeardValueLabel->setText(lastHeardText);

        if (_lastHeardUpdateTimer->isActive())
            _lastHeardUpdateTimer->setInterval(howLongAgo.intervalMs());
        else
            _lastHeardUpdateTimer->start(howLongAgo.intervalMs());
    }

    void TrackInfoDialog::init()
    {
        _ui->setupUi(this);

        if (_queueId == 0)
        {
            _ui->queueIdLabel->setVisible(false);
            _ui->queueIdValueLabel->setVisible(false);
            _ui->fileInfoFormLayout->removeWidget(_ui->queueIdLabel);
            _ui->fileInfoFormLayout->removeWidget(_ui->queueIdValueLabel);
        }

        connect(
            _lastHeardUpdateTimer, &QTimer::timeout,
            this, &TrackInfoDialog::updateLastHeard
        );

        connect(
            &_serverInterface->collectionWatcher(),
            &CollectionWatcher::newTrackReceived,
            this, &TrackInfoDialog::newTrackReceived
        );

        connect(
            &_serverInterface->collectionWatcher(),
            &CollectionWatcher::trackDataChanged,
            this, &TrackInfoDialog::trackDataChanged
        );

        connect(
            &_serverInterface->userDataFetcher(),
            &UserDataFetcher::dataReceivedForUser,
            this, &TrackInfoDialog::dataReceivedForUser
        );

        connect(
            _serverInterface, &ServerInterface::connectedChanged,
            this, &TrackInfoDialog::enableDisableButtons
        );

        auto queueController = &_serverInterface->queueController();
        connect(
            _ui->addToQueueFrontButton, &QPushButton::clicked,
            this,
            [this, queueController]()
            {
                queueController->insertQueueEntryAtFront(_trackHash);
            }
        );
        connect(
            _ui->addToQueueEndButton, &QPushButton::clicked,
            this,
            [this, queueController]()
            {
                queueController->insertQueueEntryAtEnd(_trackHash);
            }
        );

        connect(
            _ui->copyHashButton, &QPushButton::clicked,
            this,
            [this]()
            {
                QApplication::clipboard()->setText(_trackHash.toString());
            }
        );

        connect(
            _ui->closeButton, &QPushButton::clicked,
            this, &TrackInfoDialog::close
        );

        enableDisableButtons();

        _ui->closeButton->setFocus();
    }

    void TrackInfoDialog::enableDisableButtons()
    {
        auto connected = _serverInterface->connected();
        auto haveHash = !_trackHash.isNull();

        _ui->addToQueueFrontButton->setEnabled(connected && haveHash);
        _ui->addToQueueEndButton->setEnabled(connected && haveHash);

        _ui->copyHashButton->setEnabled(haveHash);
    }

    void TrackInfoDialog::fillQueueId()
    {
        if (_queueId != 0)
        {
            _ui->queueIdValueLabel->setText(QString::number(_queueId));
        }
    }

    void TrackInfoDialog::fillHash()
    {
        _ui->hashValueLabel->setText(_trackHash.toFancyString());
    }

    void TrackInfoDialog::fillTrackDetails(const CollectionTrackInfo& trackInfo)
    {
        _ui->titleValueLabel->setText(trackInfo.title());
        _ui->artistValueLabel->setText(trackInfo.artist());
        _ui->albumValueLabel->setText(trackInfo.album());

        QString lengthText;
        if (trackInfo.lengthIsKnown())
        {
            auto length = trackInfo.lengthInMilliseconds();
            lengthText = Util::millisecondsToLongDisplayTimeText(length);
        }
        else
        {
            lengthText = tr("unknown");
        }

        _ui->lengthValueLabel->setText(lengthText);
    }

    void TrackInfoDialog::fillUserData(const FileHash& hash)
    {
        if (!_serverInterface->isLoggedIn())
        {
            _ui->usernameValueLabel->clear();
            clearUserData();
            return;
        }

        _ui->usernameValueLabel->setText(_serverInterface->userLoggedInName());

        auto userId = _serverInterface->userLoggedInId();
        auto userData =
               _serverInterface->userDataFetcher().getHashDataForUser(userId, hash);

        if (userData == nullptr)
        {
            clearUserData();
            return;
        }

        QLocale locale;

        _lastHeard = QDateTime();
        if (!userData->previouslyHeardReceived)
        {
            _ui->lastHeardValueLabel->setText(tr("unknown"));
        }
        else if (userData->previouslyHeard.isNull())
        {
            _ui->lastHeardValueLabel->setText(tr("never"));
        }
        else
        {
            _lastHeard = userData->previouslyHeard;
            updateLastHeard();
        }

        QString scoreText;
        if (!userData->scoreReceived)
        {
            scoreText = tr("unknown");
        }
        else if (userData->scorePermillage < 0)
        {
            scoreText = tr("no score yet");
        }
        else
        {
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
        _lastHeard = QDateTime();

        _ui->lastHeardValueLabel->clear();
        _ui->scoreValueLabel->clear();
    }
}

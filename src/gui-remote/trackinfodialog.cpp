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

#include "trackinfodialog.h"
#include "ui_trackinfodialog.h"

#include "common/unicodechars.h"
#include "common/util.h"

#include "client/authenticationcontroller.h"
#include "client/collectionwatcher.h"
#include "client/generalcontroller.h"
#include "client/localhashidrepository.h"
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
                                     LocalHashId hashId,
                                     quint32 queueId)
     : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
        _ui(new Ui::TrackInfoDialog),
        _serverInterface(serverInterface),
        _lastHeardUpdateTimer(new QTimer(this)),
        _trackHashId(hashId),
        _queueId(queueId)
    {
        init();

        fillQueueId();
        fillHash();

        auto trackInfo = serverInterface->collectionWatcher().getTrack(hashId);

        if (trackInfo.hashId().isZero())
        { /* not found? */
            clearTrackDetails();
        }
        else
        {
            fillTrackDetails(trackInfo);
        }

        fillUserData(_trackHashId, _userId);
    }

    TrackInfoDialog::TrackInfoDialog(QWidget* parent,
                                     ServerInterface* serverInterface,
                                     const CollectionTrackInfo& track)
     : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
        _ui(new Ui::TrackInfoDialog),
        _serverInterface(serverInterface),
        _lastHeardUpdateTimer(new QTimer(this)),
        _trackHashId(track.hashId())
    {
        init();

        fillHash();
        fillTrackDetails(track);
        fillUserData(_trackHashId, _userId);
    }

    TrackInfoDialog::~TrackInfoDialog()
    {
        delete _ui;
    }

    void TrackInfoDialog::newTrackReceived(CollectionTrackInfo track)
    {
        if (track.hashId() != _trackHashId)
            return;

        fillTrackDetails(track);
    }

    void TrackInfoDialog::trackDataChanged(CollectionTrackInfo track)
    {
        if (track.hashId() != _trackHashId)
            return;

        fillTrackDetails(track);
    }

    void TrackInfoDialog::dataReceivedForUser(quint32 userId)
    {
        if (userId != _userId)
            return;

        fillUserData(_trackHashId, _userId);
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

        connect(
            _ui->userComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this]()
            {
                if (_updatingUsersList)
                    return;

                auto userId = _ui->userComboBox->currentData().value<int>();
                _userId = userId;

                fillUserData(_trackHashId, _userId);
            }
        );

        _userId = _serverInterface->userLoggedInId();

        _serverInterface->authenticationController()
            .getUserAccounts()
            .addResultListener(
                this, [this](QList<UserAccount> accounts) { fillUserComboBox(accounts); }
            );

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
                queueController->insertQueueEntryAtFront(_trackHashId);
            }
        );
        connect(
            _ui->addToQueueEndButton, &QPushButton::clicked,
            this,
            [this, queueController]()
            {
                queueController->insertQueueEntryAtEnd(_trackHashId);
            }
        );

        connect(
            _ui->copyHashButton, &QPushButton::clicked,
            this,
            [this]()
            {
                auto hash = _serverInterface->hashIdRepository()->getHash(_trackHashId);
                QApplication::clipboard()->setText(hash.toString());
            }
        );

        connect(
            _ui->closeButton, &QPushButton::clicked,
            this, &TrackInfoDialog::close
        );

        enableDisableButtons();

        _ui->closeButton->setFocus();
    }

    void TrackInfoDialog::fillUserComboBox(QList<UserAccount> accounts)
    {
        _updatingUsersList = true;

        auto* combo = _ui->userComboBox;
        combo->clear();

        int indexToSelect = -1;

        combo->addItem(tr("Public"), QVariant::fromValue(int(0)));
        if (_userId == 0) { indexToSelect = 0; }

        auto myUserId = _serverInterface->authenticationController().userLoggedInId();
        auto myUsername = _serverInterface->authenticationController().userLoggedInName();
        combo->addItem(myUsername, QVariant::fromValue(myUserId));
        if (_userId == myUserId) { indexToSelect = 1; }

        for (auto const& account : accounts)
        {
            if (account.userId == myUserId) continue; /* already added before the loop */

            if (account.userId == _userId && indexToSelect < 0)
                indexToSelect = combo->count();

            combo->addItem(account.username, QVariant::fromValue(account.userId));
        }

        if (indexToSelect >= 0)
            combo->setCurrentIndex(indexToSelect);

        _updatingUsersList = false;
    }

    void TrackInfoDialog::enableDisableButtons()
    {
        auto connected = _serverInterface->connected();
        auto haveHash = !_trackHashId.isZero();

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
        auto hash = _serverInterface->hashIdRepository()->getHash(_trackHashId);
        _ui->hashValueLabel->setText(hash.toFancyString());
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

    void TrackInfoDialog::fillUserData(LocalHashId hashId, quint32 userId)
    {
        if (!_serverInterface->isLoggedIn())
        {
            clearUserData();
            return;
        }

        auto userData =
               _serverInterface->userDataFetcher().getHashDataForUser(userId, hashId);

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

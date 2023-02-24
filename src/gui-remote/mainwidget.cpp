/*
    Copyright (C) 2014-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "mainwidget.h"
#include "ui_mainwidget.h"

#include "common/unicodechars.h"
#include "common/util.h"

#include "client/currenttrackmonitor.h"
#include "client/dynamicmodecontroller.h"
#include "client/playercontroller.h"
#include "client/queuecontroller.h"
#include "client/serverinterface.h"

#include "autopersonalmodeaction.h"
#include "clickablelabel.h"
#include "dynamicmodeparametersdialog.h"
#include "playerhistorymodel.h"
#include "precisetrackprogressmonitor.h"
#include "queuemediator.h"
#include "queuemodel.h"
#include "scoreformatdelegate.h"
#include "trackinfodialog.h"

#include <algorithm>

#include <QtDebug>
#include <QKeyEvent>
#include <QMenu>
#include <QSettings>

using namespace PMP::Client;
using namespace PMP::UnicodeChars;

namespace PMP
{
    MainWidget::MainWidget(QWidget* parent)
     : QWidget(parent),
        _ui(new Ui::MainWidget),
        _serverInterface(nullptr),
        _trackProgressMonitor(nullptr),
        _queueMediator(nullptr),
        _queueModel(nullptr), _queueContextMenu(nullptr),
        _historyModel(nullptr),
        _historyContextMenu(nullptr),
        _showingTimeRemaining(false)
    {
        _ui->setupUi(this);

        _ui->splitter->setStretchFactor(0, 4);
        _ui->splitter->setStretchFactor(1, 8);

        auto trackTimeLabel = ClickableLabel::replace(_ui->positionLabel);
        auto trackTimeValueLabel = ClickableLabel::replace(_ui->positionValueLabel);

        connect(trackTimeLabel, &ClickableLabel::clicked,
                this, &MainWidget::switchTrackTimeDisplayMode);
        connect(trackTimeValueLabel, &ClickableLabel::clicked,
                this, &MainWidget::switchTrackTimeDisplayMode);
    }

    MainWidget::~MainWidget()
    {
        QSettings settings(QCoreApplication::organizationName(),
                           QCoreApplication::applicationName());

        settings.beginGroup("queue");
        settings.setValue(
            "columnsstate", _ui->queueTableView->horizontalHeader()->saveState()
        );
        settings.endGroup();

        settings.beginGroup("history");
        settings.setValue(
            "columnsstate", _ui->historyTableView->horizontalHeader()->saveState()
        );
        settings.endGroup();

        settings.beginGroup("historysplitter");
        settings.setValue("state", _ui->splitter->saveState());
        settings.endGroup();

        delete _ui;
    }

    void MainWidget::setConnection(ServerInterface* serverInterface)
    {
        _serverInterface = serverInterface;
        new AutoPersonalModeAction(serverInterface);
        _queueMediator = new QueueMediator(serverInterface,
                                           &serverInterface->queueMonitor(),
                                           serverInterface);
        auto* queueEntryInfoStorage = &serverInterface->queueEntryInfoStorage();
        _queueModel =
            new QueueModel(
                serverInterface, serverInterface, _queueMediator,
                queueEntryInfoStorage
            );
        _historyModel = new PlayerHistoryModel(this, _serverInterface);

        _ui->trackInfoButton->setEnabled(false);
        _ui->userPlayingForLabel->setText("");
        _ui->toPersonalModeButton->setText(_serverInterface->userLoggedInName());
        _ui->toPublicModeButton->setEnabled(false);
        _ui->toPersonalModeButton->setEnabled(false);
        _ui->playButton->setEnabled(false);
        _ui->pauseButton->setEnabled(false);
        _ui->skipButton->setEnabled(false);
        _ui->queueTableView->setModel(_queueModel);
        _ui->queueTableView->installEventFilter(this);
        _ui->queueTableView->setDragEnabled(true);
        _ui->queueTableView->setAcceptDrops(true);
        _ui->queueTableView->setDropIndicatorShown(true);
        _ui->queueTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _ui->queueTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        _ui->queueTableView->setItemDelegateForColumn(4, new ScoreFormatDelegate(this));
        _ui->historyTableView->setModel(_historyModel);
        _ui->historyTableView->setDragEnabled(true);
        _ui->historyTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _ui->historyTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

        auto* playerController = &serverInterface->playerController();
        auto* currentTrackMonitor = &serverInterface->currentTrackMonitor();
        _trackProgressMonitor = new PreciseTrackProgressMonitor(currentTrackMonitor);
        auto* queueController = &serverInterface->queueController();
        auto* dynamicModeController = &serverInterface->dynamicModeController();

        connect(
            _historyModel, &PlayerHistoryModel::rowsInserted,
            _ui->historyTableView, &QTableView::scrollToBottom,
            Qt::QueuedConnection /* queued because it must be the last slot to run */
        );

        connect(
            _ui->historyTableView, &QTableView::customContextMenuRequested,
            this, &MainWidget::historyContextMenuRequested
        );

        connect(
            _ui->trackProgress, &TrackProgressWidget::seekRequested,
            currentTrackMonitor, &CurrentTrackMonitor::seekTo
        );

        connect(
            _ui->trackInfoButton, &QPushButton::clicked,
            this, &MainWidget::trackInfoButtonClicked
        );

        connect(
            _ui->toPublicModeButton, &QPushButton::clicked,
            playerController, &PlayerController::switchToPublicMode
        );
        connect(
            _ui->toPersonalModeButton, &QPushButton::clicked,
            playerController, &PlayerController::switchToPersonalMode
        );

        connect(
            _ui->playButton, &QPushButton::clicked,
            playerController, &PlayerController::play
        );
        connect(
            _ui->pauseButton, &QPushButton::clicked,
            playerController, &PlayerController::pause
        );
        connect(
            _ui->skipButton, &QPushButton::clicked,
            playerController, &PlayerController::skip
        );

        connect(
            _ui->insertBreakButton, &QPushButton::clicked,
            queueController, &QueueController::insertBreakAtFrontIfNotExists
        );

        connect(
            _ui->queueTableView, &QTableView::customContextMenuRequested,
            this, &MainWidget::queueContextMenuRequested
        );

        connect(
            _ui->dynamicModeCheckBox, &QCheckBox::stateChanged,
            this, &MainWidget::changeDynamicMode
        );
        connect(
            dynamicModeController, &DynamicModeController::dynamicModeEnabledChanged,
            this, &MainWidget::dynamicModeEnabledChanged
        );

        connect(
            _ui->dynamicModeParametersButton, &QPushButton::clicked,
            this, &MainWidget::dynamicModeParametersButtonClicked
        );
        connect(
            _ui->expandButton, &QPushButton::clicked,
            dynamicModeController, &DynamicModeController::expandQueue
        );
        connect(
            _ui->trimButton, &QPushButton::clicked,
            dynamicModeController, &DynamicModeController::trimQueue
        );

        connect(
            playerController, &PlayerController::volumeChanged,
            this, &MainWidget::volumeChanged
        );
        connect(
            _ui->volumeIncreaseButton, &QToolButton::clicked,
            this, &MainWidget::increaseVolume
        );
        connect(
            _ui->volumeDecreaseButton, &QToolButton::clicked,
            this, &MainWidget::decreaseVolume
        );

        connect(
            currentTrackMonitor, &CurrentTrackMonitor::currentTrackChanged,
            this, &MainWidget::currentTrackChanged
        );
        connect(
            currentTrackMonitor, &CurrentTrackMonitor::currentTrackInfoChanged,
            this, &MainWidget::currentTrackInfoChanged
        );
        connect(
            _trackProgressMonitor, &PreciseTrackProgressMonitor::trackProgressChanged,
            this, &MainWidget::trackProgressChanged
        );
        connect(
            playerController, &PlayerController::playerModeChanged,
            this, &MainWidget::playerModeChanged
        );
        connect(
            playerController, &PlayerController::playerStateChanged,
            this, &MainWidget::playerStateChanged
        );
        connect(
            playerController, &PlayerController::queueLengthChanged,
            this, &MainWidget::queueLengthChanged
        );

        {
            QSettings settings(QCoreApplication::organizationName(),
                               QCoreApplication::applicationName());

            settings.beginGroup("queue");
            _ui->queueTableView->horizontalHeader()->restoreState(
                settings.value("columnsstate").toByteArray()
            );
            settings.endGroup();

            settings.beginGroup("history");
            _ui->historyTableView->horizontalHeader()->restoreState(
                settings.value("columnsstate").toByteArray()
            );
            settings.endGroup();

            settings.beginGroup("historysplitter");
            _ui->splitter->restoreState(settings.value("state").toByteArray());
            settings.endGroup();
        }

        /* synchronize UI with initial state */
        playerModeChanged();
        playerStateChanged();
        queueLengthChanged();
        currentTrackInfoChanged();
        trackProgressChanged(currentTrackMonitor->playerState(),
                             currentTrackMonitor->currentQueueId(),
                             currentTrackMonitor->currentTrackProgressMilliseconds(),
                             currentTrackMonitor->currentTrackLengthMilliseconds());
        volumeChanged();
        dynamicModeEnabledChanged();
    }

    bool MainWidget::eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEventFilter(keyEvent))
                return true;
        }

        return QWidget::eventFilter(object, event);
    }

    void MainWidget::playerModeChanged()
    {
        auto& playerController = _serverInterface->playerController();

        auto mode = playerController.playerMode();
        auto userId = playerController.personalModeUserId();
        auto userLogin = playerController.personalModeUserLogin();

        switch (mode)
        {
            case PlayerMode::Public:
                _ui->playingModeLabel->setText(tr("PUBLIC mode"));
                _ui->userPlayingForLabel->setText("~~~");
                _ui->toPersonalModeButton->setEnabled(true);
                _ui->toPublicModeButton->setEnabled(false);
                break;

            case PlayerMode::Personal:
                _ui->playingModeLabel->setText(tr("PERSONAL mode"));
                _ui->userPlayingForLabel->setText(
                    QString(enDash) + " " + userLogin + " " + enDash
                );

                _ui->toPersonalModeButton->setEnabled(
                                            userId != _serverInterface->userLoggedInId());

                _ui->toPublicModeButton->setEnabled(true);
                break;

            case PlayerMode::Unknown:
                _ui->playingModeLabel->setText(tr("mode unknown"));
                _ui->userPlayingForLabel->setText("???");
                _ui->toPersonalModeButton->setEnabled(false);
                _ui->toPublicModeButton->setEnabled(false);
                break;
        }
    }

    bool MainWidget::keyEventFilter(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Delete:
            if (_ui->queueTableView->hasFocus())
            {
                //qDebug() << "got delete key";

                QModelIndex index = _ui->queueTableView->currentIndex();
                if (index.isValid())
                {
                    quint32 queueID = _queueModel->trackIdAt(index);

                    if (queueID > 0)
                    {
                        _serverInterface->queueController().deleteQueueEntry(queueID);
                        return true;
                    }
                }
            }

            break;
        }

        return false;
    }

    void MainWidget::historyContextMenuRequested(const QPoint& position)
    {
        auto index = _ui->historyTableView->indexAt(position);
        if (!index.isValid())
        {
            qDebug() << "history: index at mouse position not valid";
            return;
        }

        int row = index.row();
        auto hashId = _historyModel->trackHashAt(row);
        if (hashId.isZero())
        {
            qDebug() << "history: no hash known for track at row" << row;
            return;
        }

        if (_historyContextMenu)
            delete _historyContextMenu;
        _historyContextMenu = new QMenu(this);

        auto enqueueFrontAction =
                _historyContextMenu->addAction(tr("Add to front of queue"));
        connect(
            enqueueFrontAction, &QAction::triggered,
            this,
            [this, hashId]()
            {
                qDebug() << "history context menu: enqueue (front) triggered";
                _serverInterface->queueController().insertQueueEntryAtFront(hashId);
            }
        );

        auto enqueueEndAction = _historyContextMenu->addAction(tr("Add to end of queue"));
        connect(
            enqueueEndAction, &QAction::triggered,
            this,
            [this, hashId]()
            {
                qDebug() << "history context menu: enqueue (end) triggered";
                _serverInterface->queueController().insertQueueEntryAtEnd(hashId);
            }
        );

        _historyContextMenu->addSeparator();

        auto trackInfoAction = _historyContextMenu->addAction(tr("Track info"));
        connect(
            trackInfoAction, &QAction::triggered,
            this,
            [this, hashId]()
            {
                qDebug() << "history context menu: track info triggered";
                showTrackInfoDialog(hashId);
            }
        );

        auto popupPosition = _ui->historyTableView->viewport()->mapToGlobal(position);
        _historyContextMenu->popup(popupPosition);
    }

    void MainWidget::queueContextMenuRequested(const QPoint& position)
    {
        auto index = _ui->queueTableView->indexAt(position);
        if (!index.isValid())
        {
            qDebug() << "queue: index at mouse position not valid";
            return;
        }

        auto* queueController = &_serverInterface->queueController();

        int row = index.row();
        auto track = _queueModel->trackAt(index);
        auto queueId = track.queueId();
        qDebug() << "queue: context menu opening for Q-item" << queueId
                 << "at row index" << row;

        if (_queueContextMenu)
            delete _queueContextMenu;
        _queueContextMenu = new QMenu(this);

        QAction* removeAction = _queueContextMenu->addAction(tr("Remove"));
        removeAction->setShortcut(QKeySequence::Delete);
        if (track.isNull())
        {
            removeAction->setEnabled(false);
        }
        else
        {
            connect(
                removeAction, &QAction::triggered,
                this,
                [this, row, queueId]()
                {
                    qDebug() << "queue context menu: remove action triggered for item"
                             << queueId;
                    _queueMediator->removeTrack(row, queueId);
                }
            );
        }

        _queueContextMenu->addSeparator();

        QAction* moveToFrontAction = _queueContextMenu->addAction(tr("Move to front"));
        if (track.isNull())
        {
            moveToFrontAction->setEnabled(false);
        }
        else
        {
            connect(
                moveToFrontAction, &QAction::triggered,
                this,
                [this, row, queueId]()
                {
                    qDebug() << "queue context menu: to-front action triggered for item"
                             << queueId;
                    _queueMediator->moveTrack(row, 0, queueId);
                }
            );
        }

        QAction* moveToEndAction = _queueContextMenu->addAction(tr("Move to end"));
        if (track.isNull())
        {
            moveToEndAction->setEnabled(false);
        }
        else
        {
            connect(
                moveToEndAction, &QAction::triggered,
                this,
                [this, row, queueId]()
                {
                    qDebug() << "queue context menu: to-end action triggered for item"
                             << queueId;
                    _queueMediator->moveTrackToEnd(row, queueId);
                }
            );
        }

        _queueContextMenu->addSeparator();

        QAction* duplicateAction = _queueContextMenu->addAction(tr("Duplicate"));
        if (track.isNull() || !_queueMediator->canDuplicateEntry(queueId))
        {
            duplicateAction->setEnabled(false);
        }
        else
        {
            connect(
                duplicateAction, &QAction::triggered,
                this,
                [this, queueId]()
                {
                    qDebug() << "queue context menu: duplicate action triggered for item"
                             << queueId;
                    _queueMediator->duplicateEntryAsync(queueId);
                }
            );
        }

        QMenu* insertBeforeThisMenu = _queueContextMenu->addMenu(tr("Insert before this"));
        auto* insertBreakBeforeThisAction = insertBeforeThisMenu->addAction(tr("Break"));
        connect(insertBreakBeforeThisAction, &QAction::triggered,
                queueController,
                [queueController, row]()
                {
                    qDebug()
                        << "queue context menu: triggered: insert before this -> break";
                    queueController->insertSpecialItemAtIndex(SpecialQueueItemType::Break,
                                                              row);
                }
        );
        auto* insertBarrierBeforeThisAction =
                insertBeforeThisMenu->addAction(tr("Barrier"));
        connect(insertBarrierBeforeThisAction, &QAction::triggered,
                queueController,
                [queueController, row]()
                {
                    qDebug()
                        << "queue context menu: triggered: insert before this -> barrier";
                    queueController->insertSpecialItemAtIndex(
                                                            SpecialQueueItemType::Barrier,
                                                            row);
                }
        );

        QMenu* insertAfterThisMenu = _queueContextMenu->addMenu(tr("Insert after this"));
        auto* insertBreakAfterThisAction = insertAfterThisMenu->addAction(tr("Break"));
        connect(insertBreakAfterThisAction, &QAction::triggered,
                queueController,
                [queueController, row]()
                {
                    qDebug()
                        << "queue context menu: triggered: insert after this -> break";
                    queueController->insertSpecialItemAtIndex(SpecialQueueItemType::Break,
                                                              row + 1);
                }
        );
        auto* insertBarrierAfterThisAction =
                insertAfterThisMenu->addAction(tr("Barrier"));
        connect(insertBarrierAfterThisAction, &QAction::triggered,
                queueController,
                [queueController, row]()
                {
                    qDebug()
                        << "queue context menu: triggered: insert after this -> barrier";
                    queueController->insertSpecialItemAtIndex(
                                                            SpecialQueueItemType::Barrier,
                                                            row + 1);
                }
        );

        if (!queueController->canInsertBreakAtAnyIndex())
        {
            insertBreakBeforeThisAction->setEnabled(false);
            insertBreakAfterThisAction->setEnabled(false);
        }

        if (!queueController->canInsertBarrier())
        {
            insertBarrierBeforeThisAction->setEnabled(false);
            insertBarrierAfterThisAction->setEnabled(false);
        }

        _queueContextMenu->addSeparator();

        QAction* trackInfoAction = _queueContextMenu->addAction(tr("Track info"));
        if (track.hashId().isZero())
        {
            trackInfoAction->setEnabled(false);
        }
        else
        {
            connect(
                trackInfoAction, &QAction::triggered,
                this,
                [this, track]()
                {
                    qDebug() << "queue context menu: track info action triggered for item"
                             << track.queueId();

                    showTrackInfoDialog(track.hashId(), track.queueId());
                }
            );
        }

        _queueContextMenu->popup(_ui->queueTableView->viewport()->mapToGlobal(position));
    }

    void MainWidget::dynamicModeEnabledChanged()
    {
        auto& dynamicModeController = _serverInterface->dynamicModeController();

        auto enabled = dynamicModeController.dynamicModeEnabled();
        _ui->dynamicModeCheckBox->setEnabled(enabled.isKnown());
        _ui->dynamicModeCheckBox->setChecked(enabled.isTrue());
    }

    void MainWidget::playerStateChanged()
    {
        auto& playerController = _serverInterface->playerController();

        enableDisablePlayerControlButtons();

        QString playStateText;
        switch (playerController.playerState())
        {
            case PlayerState::Playing:
                playStateText = tr("playing");
                break;
            case PlayerState::Paused:
                playStateText = tr("paused");
                break;
            case PlayerState::Stopped:
                playStateText = tr("stopped");
                break;
            default:
                qWarning() << "Unhandled PlayerState:" << playerController.playerState();
                // fall through
            case PlayerState::Unknown:
                break; /* leave text empty */
        }

        _ui->playStateLabel->setText(playStateText);
    }

    void MainWidget::queueLengthChanged()
    {
        auto& playerController = _serverInterface->playerController();

        /* the "play" and "skip" buttons depend on the presence of a next track */
        enableDisablePlayerControlButtons();

        _ui->queueLengthValueLabel->setText(
                    QString::number(playerController.queueLength()));
    }

    void MainWidget::currentTrackChanged()
    {
        currentTrackInfoChanged();
    }

    void MainWidget::currentTrackInfoChanged()
    {
        auto& currentTrackMonitor = _serverInterface->currentTrackMonitor();

        if (currentTrackMonitor.isTrackPresent().isUnknown())
        {
            _ui->artistTitleLabel->clear();
            _ui->trackProgress->setCurrentTrack(-1);
            _ui->lengthValueLabel->clear();
        }
        else if (currentTrackMonitor.currentQueueId() <= 0)
        {
            _ui->artistTitleLabel->setText(tr("<no current track>"));
            _ui->trackProgress->setCurrentTrack(-1);
            _ui->lengthValueLabel->clear();
        }
        else
        {
            auto title = currentTrackMonitor.currentTrackTitle();
            auto artist = currentTrackMonitor.currentTrackArtist();

            if (title.isEmpty() && artist.isEmpty())
            {
                auto filename = currentTrackMonitor.currentTrackPossibleFilename();
                if (!filename.isEmpty())
                {
                    _ui->artistTitleLabel->setText(filename);
                }
                else
                {
                    _ui->artistTitleLabel->setText(tr("<unknown artist/title>"));
                }
            }
            else
            {
                if (title.isEmpty())
                    title = tr("<unknown title>");

                if (artist.isEmpty())
                    artist = tr("<unknown artist>");

                auto artistTitleText = artist + " " + enDash + " " + title;
                _ui->artistTitleLabel->setText(artistTitleText);
            }

            auto trackLength = currentTrackMonitor.currentTrackLengthMilliseconds();
            if (trackLength < 0)
            {
                _ui->lengthValueLabel->setText(tr("?"));
            }
            else
            {
                _ui->lengthValueLabel->setText(
                    Util::millisecondsToLongDisplayTimeText(trackLength)
                );
            }
        }

        enableDisableTrackInfoButton();
    }

    void MainWidget::trackProgressChanged(PlayerState state, quint32 queueId,
                                          qint64 progressInMilliseconds,
                                          qint64 trackLengthInMilliseconds)
    {
        Q_UNUSED(state)
        Q_UNUSED(queueId)

        if (trackLengthInMilliseconds < 0)
        {
            _ui->trackProgress->setCurrentTrack(-1);
        }
        else
        {
            _ui->trackProgress->setCurrentTrack(trackLengthInMilliseconds);
        }

        if (progressInMilliseconds < 0)
        {
            _ui->trackProgress->setCurrentTrack(-1);
        }
        else
        {
            _ui->trackProgress->setTrackPosition(progressInMilliseconds);
        }

        updateTrackTimeDisplay(progressInMilliseconds, trackLengthInMilliseconds);
    }

    void MainWidget::switchTrackTimeDisplayMode()
    {
        _showingTimeRemaining = !_showingTimeRemaining;

        if (_showingTimeRemaining)
        {
            _ui->positionLabel->setText(tr("Remaining:"));
        }
        else
        {
            _ui->positionLabel->setText(tr("Position:"));
        }

        updateTrackTimeDisplay();
    }

    void MainWidget::trackInfoButtonClicked()
    {
        auto& currentTrackMonitor = _serverInterface->currentTrackMonitor();

        auto hash = currentTrackMonitor.currentTrackHash();
        if (hash.isZero())
            return;

        showTrackInfoDialog(hash, currentTrackMonitor.currentQueueId());
    }

    void MainWidget::dynamicModeParametersButtonClicked()
    {
        auto* dynamicModeController = &_serverInterface->dynamicModeController();

        auto dialog = new DynamicModeParametersDialog(this, dynamicModeController);
        connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
        dialog->open();
    }

    void MainWidget::volumeChanged()
    {
        auto volume = _serverInterface->playerController().volume();
        _ui->volumeValueLabel->setText(QString::number(volume));

        _ui->volumeDecreaseButton->setEnabled(volume > 0);
        _ui->volumeIncreaseButton->setEnabled(volume >= 0 && volume < 100);
    }

    void MainWidget::decreaseVolume()
    {
        auto volume = _serverInterface->playerController().volume();

        if (volume > 0)
        {
            auto newVolume = volume > 5 ? volume - 5 : 0;

            _serverInterface->playerController().setVolume(newVolume);
        }
    }

    void MainWidget::increaseVolume()
    {
        auto volume = _serverInterface->playerController().volume();

        if (volume >= 0)
        {
            auto newVolume = volume < 95 ? volume + 5 : 100;

            _serverInterface->playerController().setVolume(newVolume);
        }
    }

    void MainWidget::changeDynamicMode(int checkState)
    {
        auto& dynamicModeController = _serverInterface->dynamicModeController();

        if (checkState == Qt::Checked)
        {
            if (!dynamicModeController.dynamicModeEnabled().isTrue())
            {
                dynamicModeController.enableDynamicMode();
            }
        }
        else
        {
            if (!dynamicModeController.dynamicModeEnabled().isFalse())
            {
                dynamicModeController.disableDynamicMode();
            }
        }
    }

    void MainWidget::enableDisableTrackInfoButton()
    {
        bool haveTrackHash =
               !_serverInterface->currentTrackMonitor().currentTrackHash().isZero();

        _ui->trackInfoButton->setEnabled(haveTrackHash);
    }

    void MainWidget::enableDisablePlayerControlButtons()
    {
        auto& playerController = _serverInterface->playerController();

        _ui->playButton->setEnabled(playerController.canPlay());
        _ui->pauseButton->setEnabled(playerController.canPause());
        _ui->skipButton->setEnabled(playerController.canSkip());
    }

    void MainWidget::updateTrackTimeDisplay()
    {
        auto* currentTrackMonitor = &_serverInterface->currentTrackMonitor();

        auto position = currentTrackMonitor->currentTrackProgressMilliseconds();
        auto trackLength = currentTrackMonitor->currentTrackLengthMilliseconds();

        updateTrackTimeDisplay(position, trackLength);
    }

    void MainWidget::updateTrackTimeDisplay(qint64 positionInMilliseconds,
                                            qint64 trackLengthInMilliseconds)
    {
        if (positionInMilliseconds < 0)
        {
            _ui->positionValueLabel->clear();
            return;
        }

        qint64 timeToDisplay = -1;

        if (_showingTimeRemaining)
        {
            if (trackLengthInMilliseconds < 0)
            {
                _ui->positionValueLabel->clear();
                return;
            }

            timeToDisplay = trackLengthInMilliseconds - positionInMilliseconds;
        }
        else /* show position */
        {
            timeToDisplay = positionInMilliseconds;
        }

        auto text = Util::millisecondsToLongDisplayTimeText(timeToDisplay);
        _ui->positionValueLabel->setText(text);
    }

    void MainWidget::showTrackInfoDialog(LocalHashId hashId, quint32 queueId)
    {
        auto dialog = new TrackInfoDialog(this, _serverInterface, hashId, queueId);
        connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
        dialog->open();
    }
}

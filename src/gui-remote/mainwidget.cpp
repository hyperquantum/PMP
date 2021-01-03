/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/audiodata.h"
#include "common/clientserverinterface.h"
#include "common/currenttrackmonitor.h"
#include "common/dynamicmodecontroller.h"
#include "common/playercontroller.h"
#include "common/queuecontroller.h"
#include "common/queuemonitor.h"
#include "common/serverconnection.h"
#include "common/userdatafetcher.h"
#include "common/util.h"

#include "autopersonalmodeaction.h"
#include "playerhistorymodel.h"
#include "precisetrackprogressmonitor.h"
#include "queueentryinfofetcher.h"
#include "queuemediator.h"
#include "queuemodel.h"
#include "scoreformatdelegate.h"
#include "trackinfodialog.h"

#include <algorithm>

#include <QtDebug>
#include <QKeyEvent>
#include <QMenu>
#include <QSettings>

namespace PMP {

    MainWidget::MainWidget(QWidget *parent) :
        QWidget(parent),
        _ui(new Ui::MainWidget),
        _clientServerInterface(nullptr),
        _trackProgressMonitor(nullptr),
        _queueMediator(nullptr),
        _queueEntryInfoFetcher(nullptr),
        _queueModel(nullptr), _queueContextMenu(nullptr),
        _noRepetitionUpdating(0),
        _historyModel(nullptr), _historyContextMenu(nullptr)
    {
        _ui->setupUi(this);

        _ui->splitter->setStretchFactor(0, 4);
        _ui->splitter->setStretchFactor(1, 8);
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

    void MainWidget::setConnection(ServerConnection* connection,
                                   ClientServerInterface* clientServerInterface)
    {
        _clientServerInterface = clientServerInterface;
        new AutoPersonalModeAction(clientServerInterface);
        _queueMediator = new QueueMediator(connection,
                                           &clientServerInterface->queueMonitor(),
                                           clientServerInterface);
        _queueEntryInfoFetcher =
            new QueueEntryInfoFetcher(connection, _queueMediator, connection);
        _queueModel =
            new QueueModel(
                connection, clientServerInterface, _queueMediator, _queueEntryInfoFetcher
            );
        _historyModel = new PlayerHistoryModel(this, _queueEntryInfoFetcher);
        _historyModel->setConnection(connection);

        _ui->trackInfoButton->setEnabled(false);
        _ui->userPlayingForLabel->setText("");
        _ui->toPersonalModeButton->setText(_clientServerInterface->userLoggedInName());
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

        auto* playerController = &clientServerInterface->playerController();
        auto* currentTrackMonitor = &clientServerInterface->currentTrackMonitor();
        _trackProgressMonitor = new PreciseTrackProgressMonitor(currentTrackMonitor);
        auto* queueController = &clientServerInterface->queueController();
        auto* dynamicModeController = &clientServerInterface->dynamicModeController();

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
            queueController, &QueueController::insertBreakAtFront
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
            _ui->startWaveButton, &QPushButton::clicked,
            this, &MainWidget::startHighScoredTracksWave
        );
        connect(
            _ui->terminateWaveButton, &QPushButton::clicked,
            this, &MainWidget::terminateHighScoredTracksWave
        );
        connect(
            _ui->noRepetitionComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWidget::noRepetitionIndexChanged
        );
        connect(
            dynamicModeController, &DynamicModeController::dynamicModeEnabledChanged,
            this, &MainWidget::dynamicModeEnabledChanged
        );
        connect(
            dynamicModeController, &DynamicModeController::noRepetitionSpanSecondsChanged,
            this, &MainWidget::noRepetitionSpanSecondsChanged
        );
        connect(
            dynamicModeController, &DynamicModeController::waveActiveChanged,
            this, &MainWidget::waveActiveChanged
        );
        connect(
            dynamicModeController, &DynamicModeController::waveProgressChanged,
            this, &MainWidget::waveProgressChanged
        );

        connect(
            _ui->expandButton, &QPushButton::clicked,
            connection, &ServerConnection::expandQueue
        );
        connect(
            _ui->trimButton, &QPushButton::clicked,
            connection, &ServerConnection::trimQueue
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
        noRepetitionSpanSecondsChanged();
        waveActiveChanged();
        waveProgressChanged();
    }

    bool MainWidget::eventFilter(QObject* object, QEvent* event) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEventFilter(keyEvent))
                return true;
        }

        return QWidget::eventFilter(object, event);
    }

    void MainWidget::playerModeChanged()
    {
        auto& playerController = _clientServerInterface->playerController();

        auto mode = playerController.playerMode();
        auto userId = playerController.personalModeUserId();
        auto userLogin = playerController.personalModeUserLogin();

        switch (mode) {
            case PlayerMode::Public:
                _ui->playingModeLabel->setText(tr("PUBLIC mode"));
                _ui->userPlayingForLabel->setText("~~~");
                _ui->toPersonalModeButton->setEnabled(true);
                _ui->toPublicModeButton->setEnabled(false);
                break;

            case PlayerMode::Personal:
                _ui->playingModeLabel->setText(tr("PERSONAL mode"));
                _ui->userPlayingForLabel->setText(
                    QString(Util::EnDash) + " " + userLogin + " " + Util::EnDash
                );

                _ui->toPersonalModeButton->setEnabled(
                            userId != _clientServerInterface->userLoggedInId());

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

    bool MainWidget::keyEventFilter(QKeyEvent* event) {
        switch (event->key()) {
        case Qt::Key_Delete:
            if (_ui->queueTableView->hasFocus()) {
                //qDebug() << "got delete key";

                QModelIndex index = _ui->queueTableView->currentIndex();
                if (index.isValid()) {
                    quint32 queueID = _queueModel->trackIdAt(index);

                    if (queueID > 0)
                    {
                        _clientServerInterface->queueController().deleteQueueEntry(
                                                                                 queueID);
                        return true;
                    }
                }
            }

            break;
        }

        return false;
    }

    void MainWidget::historyContextMenuRequested(const QPoint& position) {
        auto index = _ui->historyTableView->indexAt(position);
        if (!index.isValid()) {
            qDebug() << "history: index at mouse position not valid";
            return;
        }

        int row = index.row();
        auto hash = _historyModel->trackHashAt(row);
        if (hash.isNull()) {
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
            [this, hash]() {
                qDebug() << "history context menu: enqueue (front) triggered";
                _clientServerInterface->queueController().insertQueueEntryAtFront(hash);
            }
        );

        auto enqueueEndAction = _historyContextMenu->addAction(tr("Add to end of queue"));
        connect(
            enqueueEndAction, &QAction::triggered,
            [this, hash]() {
                qDebug() << "history context menu: enqueue (end) triggered";
                _clientServerInterface->queueController().insertQueueEntryAtEnd(hash);
            }
        );

        _historyContextMenu->addSeparator();

        auto trackInfoAction = _historyContextMenu->addAction(tr("Track info"));
        connect(
            trackInfoAction, &QAction::triggered,
            this,
            [this, hash]() {
                qDebug() << "history context menu: track info triggered";

                showTrackInfoDialog(hash);
            }
        );

        auto popupPosition = _ui->historyTableView->viewport()->mapToGlobal(position);
        _historyContextMenu->popup(popupPosition);
    }

    void MainWidget::queueContextMenuRequested(const QPoint& position) {
        auto index = _ui->queueTableView->indexAt(position);
        if (!index.isValid()) {
            qDebug() << "queue: index at mouse position not valid";
            return;
        }

        int row = index.row();
        auto track = _queueModel->trackAt(index);
        auto queueId = track.queueId();
        qDebug() << "queue: context menu opening for Q-item" << queueId;

        if (_queueContextMenu)
            delete _queueContextMenu;
        _queueContextMenu = new QMenu(this);

        QAction* removeAction = _queueContextMenu->addAction(tr("Remove"));
        removeAction->setShortcut(QKeySequence::Delete);
        if (track.isNull()) {
            removeAction->setEnabled(false);
        }
        else {
            connect(
                removeAction, &QAction::triggered,
                this,
                [this, row, queueId]() {
                    qDebug() << "queue context menu: remove action triggered for item"
                             << queueId;
                    _queueMediator->removeTrack(row, queueId);
                }
            );
        }

        _queueContextMenu->addSeparator();

        QAction* duplicateAction = _queueContextMenu->addAction(tr("Duplicate"));
        if (track.isNull() || !_queueMediator->canDuplicateEntry(queueId)) {
            duplicateAction->setEnabled(false);
        }
        else {
            connect(
                duplicateAction, &QAction::triggered,
                this,
                [this, queueId]() {
                    qDebug() << "queue context menu: duplicate action triggered for item"
                             << queueId;
                    _queueMediator->duplicateEntryAsync(queueId);
                }
            );
        }

        _queueContextMenu->addSeparator();

        QAction* moveToFrontAction = _queueContextMenu->addAction(tr("Move to front"));
        if (track.isNull()) {
            moveToFrontAction->setEnabled(false);
        }
        else {
            connect(
                moveToFrontAction, &QAction::triggered,
                this,
                [this, row, queueId]() {
                    qDebug() << "queue context menu: to-front action triggered for item"
                             << queueId;
                    _queueMediator->moveTrack(row, 0, queueId);
                }
            );
        }

        QAction* moveToEndAction = _queueContextMenu->addAction(tr("Move to end"));
        if (track.isNull()) {
            moveToEndAction->setEnabled(false);
        }
        else {
            connect(
                moveToEndAction, &QAction::triggered,
                this,
                [this, row, queueId]() {
                    qDebug() << "queue context menu: to-end action triggered for item"
                             << queueId;
                    _queueMediator->moveTrackToEnd(row, queueId);
                }
            );
        }

        _queueContextMenu->addSeparator();

        QAction* trackInfoAction = _queueContextMenu->addAction(tr("Track info"));
        if (track.hash().isNull()) {
            trackInfoAction->setEnabled(false);
        }
        else {
            connect(
                trackInfoAction, &QAction::triggered,
                this,
                [this, track]() {
                    qDebug() << "queue context menu: track info action triggered for item"
                             << track.queueId();

                    showTrackInfoDialog(track.hash(), track.queueId());
                }
            );
        }

        _queueContextMenu->popup(_ui->queueTableView->viewport()->mapToGlobal(position));
    }

    void MainWidget::dynamicModeEnabledChanged()
    {
        auto& dynamicModeController = _clientServerInterface->dynamicModeController();

        auto enabled = dynamicModeController.dynamicModeEnabled();
        _ui->dynamicModeCheckBox->setEnabled(enabled.isKnown());
        _ui->dynamicModeCheckBox->setChecked(enabled.isTrue());
    }

    void MainWidget::noRepetitionSpanSecondsChanged()
    {
        auto& dynamicModeController = _clientServerInterface->dynamicModeController();

        auto noRepetitionSpanSeconds = dynamicModeController.noRepetitionSpanSeconds();

        _ui->noRepetitionComboBox->setEnabled(noRepetitionSpanSeconds >= 0);

        if (noRepetitionSpanSeconds < 0)
        {
            _noRepetitionUpdating++;
            _ui->noRepetitionComboBox->setCurrentIndex(-1);
            _noRepetitionUpdating--;
            return;
        }

        int noRepetitionIndex = _ui->noRepetitionComboBox->currentIndex();

        if (noRepetitionIndex >= 0
                && _noRepetitionList[noRepetitionIndex] == noRepetitionSpanSeconds)
        {
            return; /* the right item is selected already */
        }

        /* search for non-repetition span in list of choices */
        noRepetitionIndex = -1;
        for (int i = 0; i < _noRepetitionList.size(); ++i)
        {
            if (_noRepetitionList[i] == noRepetitionSpanSeconds)
            {
                noRepetitionIndex = i;
                break;
            }
        }

        if (noRepetitionIndex >= 0)
        { /* found in list */
            _noRepetitionUpdating++;
            _ui->noRepetitionComboBox->setCurrentIndex(noRepetitionIndex);
            _noRepetitionUpdating--;
        }
        else
        { /* not found in list */
            buildNoRepetitionList(noRepetitionSpanSeconds);
        }
    }

    void MainWidget::playerStateChanged() {
        auto& playerController = _clientServerInterface->playerController();

        enableDisablePlayerControlButtons();

        QString playStateText;
        switch (playerController.playerState()) {
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
        auto& playerController = _clientServerInterface->playerController();

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
        auto& currentTrackMonitor = _clientServerInterface->currentTrackMonitor();

        if (currentTrackMonitor.isTrackPresent().isUnknown()) {
            _ui->artistTitleLabel->clear();
            _ui->trackProgress->setCurrentTrack(-1);
            _ui->lengthValueLabel->clear();
        }
        else if (currentTrackMonitor.currentQueueId() <= 0) {
            _ui->artistTitleLabel->setText(tr("<no current track>"));
            _ui->trackProgress->setCurrentTrack(-1);
            _ui->lengthValueLabel->clear();
        }
        else {
            auto title = currentTrackMonitor.currentTrackTitle();
            auto artist = currentTrackMonitor.currentTrackArtist();

            if (title.isEmpty() && artist.isEmpty()) {
                auto filename = currentTrackMonitor.currentTrackPossibleFilename();
                if (!filename.isEmpty()) {
                    _ui->artistTitleLabel->setText(filename);
                }
                else {
                    _ui->artistTitleLabel->setText(tr("<unknown artist/title>"));
                }
            }
            else {
                if (title.isEmpty())
                    title = tr("<unknown title>");

                if (artist.isEmpty())
                    artist = tr("<unknown artist>");

                auto artistTitleText = artist + " " + Util::EnDash + " " + title;
                _ui->artistTitleLabel->setText(artistTitleText);
            }

            auto trackLength = currentTrackMonitor.currentTrackLengthMilliseconds();
            if (trackLength < 0) {
                _ui->lengthValueLabel->setText(tr("?"));
            }
            else {
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

        if (trackLengthInMilliseconds < 0) {
            _ui->trackProgress->setCurrentTrack(-1);
        }
        else {
            _ui->trackProgress->setCurrentTrack(trackLengthInMilliseconds);
        }

        if (progressInMilliseconds < 0) {
            _ui->positionValueLabel->clear();
            _ui->trackProgress->setCurrentTrack(-1);
        }
        else {
            auto text = Util::millisecondsToLongDisplayTimeText(progressInMilliseconds);
            _ui->positionValueLabel->setText(text);

            _ui->trackProgress->setTrackPosition(progressInMilliseconds);
        }
    }

    void MainWidget::trackInfoButtonClicked()
    {
        auto& currentTrackMonitor = _clientServerInterface->currentTrackMonitor();

        auto hash = currentTrackMonitor.currentTrackHash();
        if (hash.isNull()) return;

        showTrackInfoDialog(hash, currentTrackMonitor.currentQueueId());
    }

    void MainWidget::volumeChanged() {
        auto volume = _clientServerInterface->playerController().volume();
        _ui->volumeValueLabel->setText(QString::number(volume));

        _ui->volumeDecreaseButton->setEnabled(volume > 0);
        _ui->volumeIncreaseButton->setEnabled(volume >= 0 && volume < 100);
    }

    void MainWidget::decreaseVolume() {
        auto volume = _clientServerInterface->playerController().volume();

        if (volume > 0) {
            auto newVolume = volume > 5 ? volume - 5 : 0;

            _clientServerInterface->playerController().setVolume(newVolume);
        }
    }

    void MainWidget::increaseVolume() {
        auto volume = _clientServerInterface->playerController().volume();

        if (volume >= 0) {
            auto newVolume = volume < 95 ? volume + 5 : 100;

            _clientServerInterface->playerController().setVolume(newVolume);
        }
    }

    void MainWidget::changeDynamicMode(int checkState)
    {
        auto& dynamicModeController = _clientServerInterface->dynamicModeController();

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

    void MainWidget::startHighScoredTracksWave()
    {
        _clientServerInterface->dynamicModeController().startHighScoredTracksWave();
    }

    void MainWidget::terminateHighScoredTracksWave()
    {
        _clientServerInterface->dynamicModeController().terminateHighScoredTracksWave();
    }

    void MainWidget::buildNoRepetitionList(int spanToSelect) {
        _noRepetitionUpdating++;

        _noRepetitionList.clear();
        _ui->noRepetitionComboBox->clear();

        _noRepetitionList.append(0);
        _noRepetitionList.append(3600); // 1 hour
        _noRepetitionList.append(2 * 3600); // 2 hours
        _noRepetitionList.append(4 * 3600); // 4 hours
        _noRepetitionList.append(6 * 3600); // 6 hours
        _noRepetitionList.append(10 * 3600); // 10 hours
        _noRepetitionList.append(24 * 3600); // 24 hours
        _noRepetitionList.append(2 * 24 * 3600); // 48 hours (2 days)
        _noRepetitionList.append(3 * 24 * 3600); // 72 hours (3 days)
        _noRepetitionList.append(7 * 24 * 3600); // 7 days
        _noRepetitionList.append(14 * 24 * 3600); // 14 days
        _noRepetitionList.append(21 * 24 * 3600); // 21 days (3 weeks)
        _noRepetitionList.append(28 * 24 * 3600); // 28 days (4 weeks)
        _noRepetitionList.append(56 * 24 * 3600); // 56 days (8 weeks)

        int indexOfSpanToSelect = -1;

        if (spanToSelect >= 0) {
            indexOfSpanToSelect = _noRepetitionList.indexOf(spanToSelect);
            if (indexOfSpanToSelect < 0) {
                _noRepetitionList.append(spanToSelect);
                std::sort(_noRepetitionList.begin(), _noRepetitionList.end());
                indexOfSpanToSelect = _noRepetitionList.indexOf(spanToSelect);
            }
        }

        int span;
        foreach(span, _noRepetitionList) {
            _ui->noRepetitionComboBox->addItem(noRepetitionTimeString(span));
        }

        if (indexOfSpanToSelect >= 0) {
            _ui->noRepetitionComboBox->setCurrentIndex(indexOfSpanToSelect);
        }

        _noRepetitionUpdating--;
    }

    QString MainWidget::noRepetitionTimeString(int seconds) {
        QString output;

        if (seconds > 7 * 24 * 60 * 60) {
            int weeks = seconds / (7 * 24 * 60 * 60);
            seconds -= weeks * (7 * 24 * 60 * 60);
            output += QString::number(weeks);
            output += (weeks == 1) ? " week" : " weeks";
        }

        if (seconds > 24 * 60 * 60) {
            int days = seconds / (24 * 60 * 60);
            seconds -= days * (24 * 60 * 60);
            if (output.size() > 0) output += " ";
            output += QString::number(days);
            output += (days == 1) ? " day" : " days";
        }

        if (seconds > 60 * 60) {
            int hours = seconds / (60 * 60);
            seconds -= hours * (60 * 60);
            if (output.size() > 0) output += " ";
            output += QString::number(hours);
            output += (hours == 1) ? " hour" : " hours";
        }

        if (seconds > 60) {
            int minutes = seconds / 60;
            seconds -= minutes * 60;
            if (output.size() > 0) output += " ";
            output += QString::number(minutes);
            output += (minutes == 1) ? " minute" : " minutes";
        }

        if (seconds > 0 || output.size() == 0) {
            if (output.size() > 0) output += " ";
            output += QString::number(seconds);
            output += (seconds == 1) ? " second" : " seconds";
        }

        return output;
    }

    void MainWidget::noRepetitionIndexChanged(int index) {
        if (_noRepetitionUpdating > 0
            || index < 0 || index >= _noRepetitionList.size())
        {
            return;
        }

        int newSpan = _noRepetitionList[index];

        qDebug() << "noRepetitionIndexChanged: index" << index << "  value" << newSpan;

        _clientServerInterface->dynamicModeController().setNoRepetitionSpan(newSpan);
    }

    void MainWidget::waveActiveChanged()
    {
        auto& dynamicModeController = _clientServerInterface->dynamicModeController();

        _ui->startWaveButton->setEnabled(dynamicModeController.canStartWave());
        _ui->terminateWaveButton->setVisible(dynamicModeController.canTerminateWave());
    }

    void MainWidget::waveProgressChanged()
    {
        auto& dynamicModeController = _clientServerInterface->dynamicModeController();

        int progress = dynamicModeController.waveProgress();
        int progressTotal = dynamicModeController.waveProgressTotal();

        if (progress < 0 || progressTotal <= 0)
        {
            _ui->waveProgressValueLabel->setVisible(false);
        }
        else
        {
            auto text =
                    QString::number(progress) + " / " + QString::number(progressTotal);

            _ui->waveProgressValueLabel->setText(text);
            _ui->waveProgressValueLabel->setVisible(true);
        }
    }

    void MainWidget::enableDisableTrackInfoButton()
    {
        bool haveTrackHash =
               !_clientServerInterface->currentTrackMonitor().currentTrackHash().isNull();

        _ui->trackInfoButton->setEnabled(haveTrackHash);
    }

    void MainWidget::enableDisablePlayerControlButtons()
    {
        auto& playerController = _clientServerInterface->playerController();

        _ui->playButton->setEnabled(playerController.canPlay());
        _ui->pauseButton->setEnabled(playerController.canPause());
        _ui->skipButton->setEnabled(playerController.canSkip());
    }

    void MainWidget::showTrackInfoDialog(FileHash hash, quint32 queueId)
    {
        auto dialog = new TrackInfoDialog(this, _clientServerInterface, hash, queueId);
        connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
        dialog->open();
    }

}

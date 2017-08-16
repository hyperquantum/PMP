/*
    Copyright (C) 2014-2017, Kevin Andre <hyperquantum@gmail.com>

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
#include "common/serverconnection.h"

#include "autopersonalmodeaction.h"
#include "currenttrackmonitor.h"
#include "queueentryinfofetcher.h"
#include "queuemediator.h"
#include "queuemodel.h"
#include "queuemonitor.h"
#include "scoreformatdelegate.h"
#include "userdatafetcher.h"

#include <algorithm>

#include <QtDebug>
#include <QKeyEvent>
#include <QMenu>
#include <QSettings>

namespace PMP {

    MainWidget::MainWidget(QWidget *parent) :
        QWidget(parent),
        _ui(new Ui::MainWidget),
        _connection(0), _currentTrackMonitor(0),
        _queueMonitor(0), _queueMediator(0),
        _queueEntryInfoFetcher(0), _userDataFetcher(0), _queueModel(0),
        _volume(-1), _nowPlayingQID(0), _nowPlayingLength(-1),
        _dynamicModeEnabled(false), _noRepetitionUpdating(0)
    {
        _ui->setupUi(this);
    }

    MainWidget::~MainWidget()
    {
        QSettings settings(QCoreApplication::organizationName(),
                           QCoreApplication::applicationName());

        settings.beginGroup("queue");
        settings.setValue(
            "columnsstate", _ui->queueTableView->horizontalHeader()->saveState()
        );

        delete _ui;
    }

    void MainWidget::setConnection(ServerConnection* connection) {
        _connection = connection;
        new AutoPersonalModeAction(connection); /* uses connection as parent */
        _currentTrackMonitor = new CurrentTrackMonitor(_connection);
        _queueMonitor = new QueueMonitor(_connection, _connection);
        _queueMediator = new QueueMediator(_connection, _queueMonitor, _connection);
        _queueEntryInfoFetcher =
            new QueueEntryInfoFetcher(_connection, _queueMediator, _connection);
        _userDataFetcher = new UserDataFetcher(_connection, _connection);
        _queueModel =
            new QueueModel(
                _connection, _queueMediator, _queueEntryInfoFetcher, _userDataFetcher
            );

        _ui->userPlayingForLabel->setText("");
        _ui->toPersonalModeButton->setText(_connection->userLoggedInName());
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

        connect(
            _ui->trackProgress, SIGNAL(seekRequested(qint64)),
            _currentTrackMonitor, SLOT(seekTo(qint64))
        );

        connect(
            _connection, &ServerConnection::receivedUserPlayingFor,
            this, &MainWidget::userPlayingForChanged
        );
        connect(
            _ui->toPublicModeButton, &QPushButton::clicked,
            _connection, &ServerConnection::switchToPublicMode
        );
        connect(
            _ui->toPersonalModeButton, &QPushButton::clicked,
            _connection, &ServerConnection::switchToPersonalMode
        );

        connect(_ui->playButton, SIGNAL(clicked()), _connection, SLOT(play()));
        connect(_ui->pauseButton, SIGNAL(clicked()), _connection, SLOT(pause()));
        connect(_ui->skipButton, SIGNAL(clicked()), _connection, SLOT(skip()));

        connect(
            _ui->insertBreakButton, &QPushButton::clicked,
            _connection, &ServerConnection::insertPauseAtFront
        );

        connect(
            _ui->queueTableView, &QTableView::customContextMenuRequested,
            this, &MainWidget::queueContextMenuRequested
        );

        connect(
            _ui->dynamicModeCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(changeDynamicMode(int))
        );
        connect(
            _ui->noRepetitionComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(noRepetitionIndexChanged(int))
        );
        connect(
            _connection, SIGNAL(dynamicModeStatusReceived(bool, int)),
            this, SLOT(dynamicModeStatusReceived(bool, int))
        );

        connect(
            _ui->expandButton, &QPushButton::clicked,
            _connection, &ServerConnection::expandQueue
        );
        connect(
            _ui->trimButton, &QPushButton::clicked,
            _connection, &ServerConnection::trimQueue
        );

        connect(
            _currentTrackMonitor, SIGNAL(volumeChanged(int)),
            this, SLOT(volumeChanged(int))
        );
        connect(
            _ui->volumeIncreaseButton, SIGNAL(clicked()),
            this, SLOT(increaseVolume())
        );
        connect(
            _ui->volumeDecreaseButton, SIGNAL(clicked()),
            this, SLOT(decreaseVolume())
        );

        connect(
            _currentTrackMonitor, &CurrentTrackMonitor::playing,
            this, &MainWidget::playing
        );
        connect(
            _currentTrackMonitor, &CurrentTrackMonitor::paused, this, &MainWidget::paused
        );
        connect(
            _currentTrackMonitor, &CurrentTrackMonitor::stopped,
            this, &MainWidget::stopped
        );
        connect(
            _currentTrackMonitor, &CurrentTrackMonitor::queueLengthChanged,
            this, &MainWidget::queueLengthChanged
        );
        connect(
            _currentTrackMonitor, SIGNAL(trackProgress(quint32, quint64, int)),
            this, SLOT(trackProgress(quint32, quint64, int))
        );
        connect(
            _currentTrackMonitor, SIGNAL(trackProgress(quint64)),
            this, SLOT(trackProgress(quint64))
        );
        connect(
            _currentTrackMonitor, SIGNAL(receivedTitleArtist(QString, QString)),
            this, SLOT(receivedTitleArtist(QString, QString))
        );
        connect(
            _currentTrackMonitor, SIGNAL(receivedPossibleFilename(QString)),
            this, SLOT(receivedPossibleFilename(QString))
        );

        _connection->requestUserPlayingForMode();
        _connection->requestDynamicModeStatus();

        {
            QSettings settings(QCoreApplication::organizationName(),
                               QCoreApplication::applicationName());

            settings.beginGroup("queue");
            _ui->queueTableView->horizontalHeader()->restoreState(
                settings.value("columnsstate").toByteArray()
            );
        }
    }

    bool MainWidget::eventFilter(QObject* object, QEvent* event) {
        if (_ui->queueTableView->hasFocus()) {
            if (event->type() == QEvent::KeyPress) {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

                if (keyEvent->key() == Qt::Key_Delete) {
                    //qDebug() << "got delete key";

                    QModelIndex index = _ui->queueTableView->currentIndex();
                    if (index.isValid()) {
                        quint32 queueID = _queueModel->trackIdAt(index);

                        if (queueID > 0) {
                            _connection->deleteQueueEntry(queueID);
                            return true;
                        }
                    }
                }
            }
        }

        return QWidget::eventFilter(object, event);
    }

    void MainWidget::queueContextMenuRequested(const QPoint& position) {
        auto index = _ui->queueTableView->indexAt(position);
        if (index.isValid()) {
            int row = index.row();
            quint32 queueID = _queueModel->trackIdAt(index);
            qDebug() << "queue: context menu opening for Q-item" << queueID;

            QMenu* menu = new QMenu(this);

            QAction* removeAction = menu->addAction("Remove");
            removeAction->setShortcut(QKeySequence::Delete);
            connect(
                removeAction, &QAction::triggered,
                [this, row, queueID]() {
                    qDebug() << "queue context menu: remove action triggered for item"
                             << queueID;
                    _queueMediator->removeTrack(row, queueID);
                }
            );

            QAction* moveToFrontAction = menu->addAction("Move to front");
            connect(
                moveToFrontAction, &QAction::triggered,
                [this, row, queueID]() {
                    qDebug() << "queue context menu: to-front action triggered for item"
                             << queueID;
                    _queueMediator->moveTrack(row, 0, queueID);
                }
            );

            menu->popup(_ui->queueTableView->viewport()->mapToGlobal(position));
        }
        else {
            qDebug() << "queue: index at mouse position not valid";
        }
    }

    void MainWidget::playing(quint32 queueID) {
        if (_nowPlayingQID != queueID) {
            _nowPlayingQID = queueID;
            _nowPlayingArtist = "";
            _nowPlayingTitle = "";
            _nowPlayingLength = -1;
            _ui->artistTitleLabel->setText("");
            _ui->trackProgress->setCurrentTrack(-1);
            _ui->lengthValueLabel->setText("");
            _ui->positionValueLabel->setText("");
        }

        _ui->playButton->setEnabled(false);
        _ui->pauseButton->setEnabled(true);
        _ui->skipButton->setEnabled(true);
        _ui->playStateLabel->setText("playing");
    }

    void MainWidget::paused(quint32 queueID) {
        if (_nowPlayingQID != queueID) {
            _nowPlayingQID = queueID;
            _nowPlayingArtist = "";
            _nowPlayingTitle = "";
            _nowPlayingLength = -1;
            _ui->artistTitleLabel->setText("");
            _ui->trackProgress->setCurrentTrack(-1);
            _ui->lengthValueLabel->setText("");
            _ui->positionValueLabel->setText("");
        }

        _ui->playButton->setEnabled(true);
        _ui->pauseButton->setEnabled(false);
        _ui->skipButton->setEnabled(true);
        _ui->playStateLabel->setText("paused");
    }

    void MainWidget::stopped(quint32 queueLength) {
        _nowPlayingQID = 0;
        _nowPlayingArtist = "";
        _nowPlayingTitle = "";
        _nowPlayingLength = -1;

        _ui->artistTitleLabel->setText("<no current track>");
        _ui->trackProgress->setCurrentTrack(-1);
        _ui->lengthValueLabel->setText("");
        _ui->positionValueLabel->setText("");

        _ui->playButton->setEnabled(queueLength > 0);
        _ui->pauseButton->setEnabled(false);
        _ui->skipButton->setEnabled(false);
        _ui->playStateLabel->setText("stopped");
    }

    void MainWidget::queueLengthChanged(quint32 queueLength, int state)
    {
        _ui->queueLengthValueLabel->setText(QString::number(queueLength));

        _ui->playButton->setEnabled(
            state == (int)ServerConnection::Paused
                || (state == (int)ServerConnection::Stopped && queueLength > 0)
        );
    }

    void MainWidget::trackProgress(quint32 queueID, quint64 position, int lengthSeconds) {
        if (queueID != _nowPlayingQID) return;

        //qDebug() << "DISPLAY: nowPlayingTrack, track length (secs):" << lengthInSeconds;

        if (lengthSeconds != _nowPlayingLength) {
            _nowPlayingLength = lengthSeconds;

            if (lengthSeconds < 0) {
                _ui->lengthValueLabel->setText("?");
                _ui->trackProgress->setCurrentTrack(-1);
            }
            else {
                quint64 lengthInMilliseconds = (quint64)lengthSeconds * 1000;

                _ui->lengthValueLabel->setText(
                    AudioData::millisecondsToTimeString(lengthInMilliseconds)
                );

                _ui->trackProgress->setCurrentTrack(lengthInMilliseconds);
            }
        }

        trackProgress(position);
    }

    void MainWidget::trackProgress(quint64 position) {
        //qDebug() << "DISPLAY: trackPositionChanged" << position;

        _ui->positionValueLabel->setText(AudioData::millisecondsToTimeString(position));
        _ui->trackProgress->setTrackPosition(position);
    }

    void MainWidget::receivedTitleArtist(QString title, QString artist) {
        _nowPlayingTitle = title;
        _nowPlayingArtist = artist;

        QString artistToShow = (artist == "") ? "<unknown artist>" : artist;
        QString titleToShow = (title == "") ? "<unknown title>" : title;
        QString artistTitleToShow =
            (artist == "" && title == "")
             ? "<unknown artist/title>"
             : artistToShow + " " + QChar(0x2013) /* <- EN DASH */ + " " + titleToShow;

        _ui->artistTitleLabel->setText(artistTitleToShow);
    }

    void MainWidget::receivedPossibleFilename(QString name) {
        if (_nowPlayingTitle.trimmed() == "") {
            _ui->artistTitleLabel->setText(name);
        }
    }

    void MainWidget::volumeChanged(int percentage) {
        _volume = percentage;
        _ui->volumeValueLabel->setText(QString::number(percentage));

        _ui->volumeDecreaseButton->setEnabled(percentage > 0);
        _ui->volumeIncreaseButton->setEnabled(percentage < 100);
    }

    void MainWidget::decreaseVolume() {
        if (_volume > 0) {
            _connection->setVolume(_volume > 5 ? _volume - 5 : 0);
        }
    }

    void MainWidget::increaseVolume() {
        if (_volume >= 0) {
            _connection->setVolume(_volume < 95 ? _volume + 5 : 100);
        }
    }

    void MainWidget::changeDynamicMode(int checkState) {
        if (checkState == Qt::Checked) {
            if (!_dynamicModeEnabled) {
                _connection->enableDynamicMode();
                _dynamicModeEnabled = true;
            }
        }
        else {
            if (_dynamicModeEnabled)  {
                _connection->disableDynamicMode();
                _dynamicModeEnabled = false;
            }
        }
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

        _connection->setDynamicModeNoRepetitionSpan(newSpan);
    }

    void MainWidget::dynamicModeStatusReceived(bool enabled, int noRepetitionSpan) {
        _dynamicModeEnabled = enabled;
        _ui->dynamicModeCheckBox->setChecked(enabled);

        int noRepetitionIndex = _ui->noRepetitionComboBox->currentIndex();
        if (noRepetitionIndex < 0
            || _noRepetitionList[noRepetitionIndex] != noRepetitionSpan)
        {
            /* search for non-repetition span in list of choices */
            noRepetitionIndex = -1;
            for (int i = 0; i < _noRepetitionList.size(); ++i) {
                if (_noRepetitionList[i] == noRepetitionSpan) {
                    noRepetitionIndex = i;
                    break;
                }
            }

            if (noRepetitionIndex >= 0) { /* found in list */
                _noRepetitionUpdating++;
                _ui->noRepetitionComboBox->setCurrentIndex(noRepetitionIndex);
                _noRepetitionUpdating--;
            }
            else { /* not found in list */
                buildNoRepetitionList(noRepetitionSpan);
            }
        }
    }

    void MainWidget::userPlayingForChanged(quint32 userId, QString login) {
        if (userId == 0) {
            _ui->playingModeLabel->setText("PUBLIC mode");
            _ui->userPlayingForLabel->setText("~~~");
        }
        else {
            _ui->playingModeLabel->setText("PERSONAL mode");
            _ui->userPlayingForLabel->setText(
                QString(QChar(0x2013)) + " " + login + " " + QChar(0x2013)
            );
        }

        _ui->toPersonalModeButton->setEnabled(userId != _connection->userLoggedInId());
        _ui->toPublicModeButton->setEnabled(userId != 0);
    }

}

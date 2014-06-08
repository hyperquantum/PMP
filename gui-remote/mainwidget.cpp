/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/serverconnection.h"

#include "queuemodel.h"
#include "queuemonitor.h"

namespace PMP {

    MainWidget::MainWidget(QWidget *parent) :
        QWidget(parent),
        _ui(new Ui::MainWidget),
        _connection(0), _queueMonitor(0), _queueModel(0),
        _volume(-1), _nowPlayingQID(0), _nowPlayingLength(-1)
    {
        _ui->setupUi(this);
    }

    MainWidget::~MainWidget()
    {
        delete _ui;
    }

    void MainWidget::setConnection(ServerConnection* connection) {
        _connection = connection;
        _queueMonitor = new QueueMonitor(_connection, _connection);
        _queueModel = new QueueModel(_connection, _queueMonitor);

        _ui->queueTableView->setModel(_queueModel);

        connect(_ui->playButton, SIGNAL(clicked()), _connection, SLOT(play()));
        connect(_ui->pauseButton, SIGNAL(clicked()), _connection, SLOT(pause()));
        connect(_ui->skipButton, SIGNAL(clicked()), _connection, SLOT(skip()));

        connect(_connection, SIGNAL(volumeChanged(int)), this, SLOT(volumeChanged(int)));
        connect(_ui->volumeIncreaseButton, SIGNAL(clicked()), this, SLOT(increaseVolume()));
        connect(_ui->volumeDecreaseButton, SIGNAL(clicked()), this, SLOT(decreaseVolume()));

        connect(_connection, SIGNAL(playing()), this, SLOT(playing()));
        connect(_connection, SIGNAL(paused()), this, SLOT(paused()));
        connect(_connection, SIGNAL(stopped()), this, SLOT(stopped()));

        connect(_connection, SIGNAL(noCurrentTrack()), this, SLOT(noCurrentTrack()));
        connect(_connection, SIGNAL(nowPlayingTrack(quint32)), this, SLOT(nowPlayingTrack(quint32)));
        connect(_connection, SIGNAL(nowPlayingTrack(QString, QString, int)), this, SLOT(nowPlayingTrack(QString, QString, int)));
        connect(_connection, SIGNAL(trackPositionChanged(quint64)), this, SLOT(trackPositionChanged(quint64)));
        connect(_connection, SIGNAL(queueLengthChanged(int)), this, SLOT(queueLengthChanged(int)));
        connect(_connection, SIGNAL(receivedTrackInfo(quint32, int, QString, QString)), this, SLOT(receivedTrackInfo(quint32, int, QString, QString)));
    }

    void MainWidget::playing() {
        _ui->stateValueLabel->setText("playing");
    }

    void MainWidget::paused() {
        _ui->stateValueLabel->setText("paused");
    }

    void MainWidget::stopped() {
        _ui->stateValueLabel->setText("stopped");
    }

    void MainWidget::volumeChanged(int percentage) {
        _volume = percentage;
        _ui->volumeValueLabel->setText(QString::number(percentage));
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

    void MainWidget::noCurrentTrack() {
        _nowPlayingQID = 0;
        _nowPlayingArtist = "";
        _nowPlayingTitle = "";
        _nowPlayingLength = -1;
        _ui->titleValueLabel->setText("");
        _ui->artistValueLabel->setText("");
        _ui->lengthValueLabel->setText("");
        _ui->positionValueLabel->setText("");
    }

    void MainWidget::nowPlayingTrack(quint32 queueID) {
        if (queueID != _nowPlayingQID) {
            _nowPlayingQID = queueID;
            _nowPlayingArtist = "";
            _nowPlayingTitle = "";
            _nowPlayingLength = -1;

            _connection->sendTrackInfoRequest(queueID);
        }

        nowPlayingTrack(_nowPlayingTitle, _nowPlayingArtist, _nowPlayingLength);
    }

    void MainWidget::nowPlayingTrack(QString title, QString artist, int lengthInSeconds) {
        _ui->titleValueLabel->setText(title);
        _ui->artistValueLabel->setText(artist);

        if (lengthInSeconds < 0) {
            _ui->lengthValueLabel->setText("?");
        }
        else {
            int sec = lengthInSeconds % 60;
            int min = (lengthInSeconds / 60) % 60;
            int hrs = lengthInSeconds / 3600;

            _ui->lengthValueLabel->setText(
                QString::number(hrs).rightJustified(2, '0')
                 + ":" + QString::number(min).rightJustified(2, '0')
                 + ":" + QString::number(sec).rightJustified(2, '0')
            );
        }
    }

    void MainWidget::trackPositionChanged(quint64 position) {
        int positionInSeconds = position / 1000;

        int partialSec = position % 1000;
        int sec = positionInSeconds % 60;
        int min = (positionInSeconds / 60) % 60;
        int hrs = positionInSeconds / 3600;

        _ui->positionValueLabel->setText(
            QString::number(hrs).rightJustified(2, '0')
             + ":" + QString::number(min).rightJustified(2, '0')
             + ":" + QString::number(sec).rightJustified(2, '0')
             + "." + QString::number(partialSec).rightJustified(3, '0')
        );
    }

    void MainWidget::queueLengthChanged(int length) {
        _ui->queueLengthValueLabel->setText(QString::number(length));
    }

    void MainWidget::receivedTrackInfo(quint32 queueID, int lengthInSeconds, QString title, QString artist) {
        if (_nowPlayingQID != queueID) { return; }

        _nowPlayingArtist = artist;
        _nowPlayingTitle = title;
        _nowPlayingLength = lengthInSeconds;

        nowPlayingTrack(title, artist, lengthInSeconds);
    }

}

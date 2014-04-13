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

#include "serverconnection.h"

namespace PMP {

    MainWidget::MainWidget(QWidget *parent) :
        QWidget(parent),
        _ui(new Ui::MainWidget), _connection(0)
    {
        _ui->setupUi(this);
    }

    MainWidget::~MainWidget()
    {
        delete _ui;
    }

    void MainWidget::setConnection(ServerConnection* connection) {
        _connection = connection;

        connect(_ui->playButton, SIGNAL(clicked()), _connection, SLOT(play()));
        connect(_ui->pauseButton, SIGNAL(clicked()), _connection, SLOT(pause()));
        connect(_ui->skipButton, SIGNAL(clicked()), _connection, SLOT(skip()));

        connect(_connection, SIGNAL(noCurrentTrack()), this, SLOT(noCurrentTrack()));
        connect(_connection, SIGNAL(nowPlayingTrack(QString, QString, int)), this, SLOT(nowPlayingTrack(QString, QString, int)));
    }

    void MainWidget::noCurrentTrack() {
        _ui->titleValueLabel->setText("");
        _ui->artistValueLabel->setText("");
        _ui->lengthValueLabel->setText("");
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

}

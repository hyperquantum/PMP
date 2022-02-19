/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "notificationbar.h"
#include "ui_notificationbar.h"

namespace PMP
{
    Notification::~Notification()
    {
        //
    }

    Notification::Notification(QObject* parent)
     : QObject(parent)
    {
        //
    }

    /* ============================================================================ */

    NotificationBar::NotificationBar(QWidget* parent)
     : QWidget(parent),
       _ui(new Ui::NotificationBar),
       _notification(nullptr)
    {
        _ui->setupUi(this);

        setVisible(false);
    }

    NotificationBar::~NotificationBar()
    {
        _notification->deleteLater();
        delete _ui;
    }

    void NotificationBar::showNotification(Notification* notification)
    {
        if (_notification == notification)
            return;

        if (_notification)
            delete _notification;

        _notification = notification;

        if (notification == nullptr)
        {
            setVisible(false);
            return;
        }

        setUpNotification();
    }

    QSize NotificationBar::minimumSizeHint() const
    {
        auto labelMinimumSize = _ui->notificationTextLabel->minimumSizeHint();
        auto buttonMinimumSize = _ui->firstActionButton->minimumSizeHint();

        return QSize(labelMinimumSize.width() + buttonMinimumSize.width(),
                     qMax(labelMinimumSize.height(), buttonMinimumSize.height()));
    }

    QSize NotificationBar::sizeHint() const
    {
        auto labelSize = _ui->notificationTextLabel->sizeHint();
        auto buttonSize = _ui->firstActionButton->sizeHint();

        return QSize(labelSize.width() + buttonSize.width(),
                     qMax(labelSize.height(), buttonSize.height()));
    }

    void NotificationBar::setUpNotification()
    {
        connect(
            _notification, &Notification::destroyed,
            this,
            [this](QObject* sender)
            {
                if (sender != _notification)
                    return;

                _notification = nullptr;
                setVisible(false);
            }
        );
        connect(
            _notification, &Notification::visibleChanged,
            this, [this]() { setVisible(_notification->visible()); }
        );
        connect(
            _notification, &Notification::notificationTextChanged,
            this,
            [this]()
            {
                _ui->notificationTextLabel->setText(_notification->notificationText());
            }
        );

        _ui->notificationTextLabel->setTextFormat(Qt::PlainText);
        _ui->notificationTextLabel->setText(_notification->notificationText());

        setUpFirstActionButton();

        setVisible(_notification->visible());
    }

    void NotificationBar::setUpFirstActionButton()
    {
        QString action1Text = _notification->actionButton1Text();
        if (action1Text.isEmpty())
        {
            _ui->firstActionButton->setVisible(false);
        }
        else
        {
            connect(
                _ui->firstActionButton, &QPushButton::clicked,
                _notification, &Notification::actionButton1Pushed
            );

            _ui->firstActionButton->setText(action1Text);
            _ui->firstActionButton->setVisible(true);
        }
    }

}

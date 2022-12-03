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
     : QFrame(parent),
       _ui(new Ui::NotificationBar)
    {
        _ui->setupUi(this);

        _ui->notificationTextLabel->setTextFormat(Qt::PlainText);

        setVisible(false);

        connect(
            _ui->scrollBar, &QScrollBar::valueChanged,
            this, &NotificationBar::onScrollBarValueChanged
        );
        connect(
            _ui->firstActionButton, &QPushButton::clicked,
            this, &NotificationBar::onNotificationAction1Clicked
        );
    }

    NotificationBar::~NotificationBar()
    {
        delete _ui;
    }

    void NotificationBar::addNotification(Notification* notification)
    {
        if (notification == nullptr)
            return;

        auto index = _notifications.indexOf(notification);
        if (index >= 0)
            return;

        connectSlots(notification);

        _notifications.append(notification);

        if (!notification->visible())
            return;

        _visibleNotifications.append(notification);
        _visibleNotificationIndex = _visibleNotifications.size() - 1;

        updateUiAfterVisibleIndexChanged();
    }

    QSize NotificationBar::minimumSizeHint() const
    {
        auto labelMinimumSize = _ui->notificationTextLabel->minimumSizeHint();
        auto buttonMinimumSize = _ui->firstActionButton->minimumSizeHint();

        auto layoutMargins = layout()->contentsMargins();
        auto layoutSpacing = layout()->spacing();

        auto width =
            layoutMargins.left() +
            labelMinimumSize.width() +
            layoutSpacing +
            buttonMinimumSize.width() +
            layoutMargins.right();

        auto height =
            layoutMargins.top() +
            qMax(labelMinimumSize.height(), buttonMinimumSize.height()) +
            layoutMargins.bottom();

        return QSize(width, height);
    }

    QSize NotificationBar::sizeHint() const
    {
        auto labelSize = _ui->notificationTextLabel->sizeHint();
        auto buttonSize = _ui->firstActionButton->sizeHint();

        auto layoutMargins = layout()->contentsMargins();
        auto layoutSpacing = layout()->spacing();

        auto width =
            layoutMargins.left() +
            labelSize.width() +
            layoutSpacing +
            buttonSize.width() +
            layoutMargins.right();

        auto height =
            layoutMargins.top() +
            qMax(labelSize.height(), buttonSize.height()) +
            layoutMargins.bottom();

        return QSize(width, height);
    }

    Notification* NotificationBar::getVisibleNotification() const
    {
        if (_visibleNotificationIndex < 0
                || _visibleNotificationIndex >= _visibleNotifications.size())
        {
            return nullptr;
        }

        return _visibleNotifications[_visibleNotificationIndex];
    }

    void NotificationBar::connectSlots(Notification* notification)
    {
        connect(
            notification, &Notification::destroyed,
            this,
            [this, notification](QObject* sender)
            {
                if (sender != notification)
                    return;

                onNotificationDestroyed(notification);
            }
        );

        connect(
            notification, &Notification::visibleChanged,
            this, [this, notification]() { onNotificationVisibleChanged(notification); }
        );

        connect(
            notification, &Notification::notificationTextChanged,
            this, [this, notification]() { onNotificationTextChanged(notification); }
        );
    }

    void NotificationBar::onNotificationDestroyed(Notification* notification)
    {
        int visibleIndex = _visibleNotifications.indexOf(notification);
        if (visibleIndex >= 0)
        {
            _visibleNotifications.removeAt(visibleIndex);

            if (_visibleNotificationIndex >= _visibleNotifications.size())
                _visibleNotificationIndex--;

            updateUiAfterVisibleIndexChanged();
        }

        int notificationsIndex = _notifications.indexOf(notification);
        if (notificationsIndex >= 0)
            _notifications.removeAt(notificationsIndex);
    }

    void NotificationBar::onNotificationVisibleChanged(Notification* notification)
    {
        int index = _visibleNotifications.indexOf(notification);
        bool mustBeVisible = notification->visible();

        if (mustBeVisible == (index >= 0))
            return; /* no visibility change */

        if (index < 0)
        {
            _visibleNotifications.append(notification);
            _visibleNotificationIndex = _visibleNotifications.size() - 1;
        }
        else
        {
            _visibleNotifications.removeAt(index);

            if (_visibleNotificationIndex >= _visibleNotifications.size())
                _visibleNotificationIndex--;
        }

        updateUiAfterVisibleIndexChanged();
    }

    void NotificationBar::onNotificationTextChanged(Notification* notification)
    {
        auto* visibleNotification = getVisibleNotification();
        if (visibleNotification != notification)
            return;

        _ui->notificationTextLabel->setText(notification->notificationText());
    }

    void NotificationBar::onScrollBarValueChanged()
    {
        if (_scrollBarUpdating)
            return;

        _visibleNotificationIndex = _ui->scrollBar->value();
        updateUiAfterVisibleIndexChanged();
    }

    void NotificationBar::onNotificationAction1Clicked()
    {
        auto* notification = getVisibleNotification();
        if (notification == nullptr)
            return;

        notification->actionButton1Pushed();
    }

    void NotificationBar::updateUiAfterVisibleIndexChanged()
    {
        auto* notification = getVisibleNotification();

        if (notification == nullptr)
        {
            setVisible(false);
            return;
        }

        Q_ASSERT_X(_scrollBarUpdating == false,
                   "NotificationBar::updateUiAfterVisibleIndexChanged",
                   "scrollbar is already being updated");
        _scrollBarUpdating = true;
        _ui->scrollBar->setMaximum(_visibleNotifications.size() - 1);
        _ui->scrollBar->setValue(_visibleNotificationIndex);
        _ui->scrollBar->setVisible(_visibleNotifications.size() > 1);
        _scrollBarUpdating = false;

        _ui->notificationTextLabel->setText(notification->notificationText());

        auto action1Text = notification->actionButton1Text();
        if (action1Text.isEmpty())
        {
            _ui->firstActionButton->setVisible(false);
        }
        else
        {
            _ui->firstActionButton->setText(action1Text);
            _ui->firstActionButton->setVisible(true);
        }

        setVisible(true);
    }
}

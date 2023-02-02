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

#ifndef PMP_NOTIFICATIONBAR_H
#define PMP_NOTIFICATIONBAR_H

#include <QFrame>
#include <QVector>

namespace Ui
{
    class NotificationBar;
}

namespace PMP
{
    class Notification : public QObject
    {
        Q_OBJECT
    public:
        virtual ~Notification();

        virtual QString notificationText() const = 0;

        virtual QString actionButton1Text() const = 0;

        virtual bool visible() const = 0;

    public Q_SLOTS:
        virtual void actionButton1Pushed() = 0;

    Q_SIGNALS:
        void visibleChanged();
        void notificationTextChanged();

    protected:
        explicit Notification(QObject* parent = nullptr);
    };

    class NotificationBar : public QFrame
    {
        Q_OBJECT
    public:
        explicit NotificationBar(QWidget* parent = nullptr);
        ~NotificationBar();

        void addNotification(Notification* notification);

        QSize minimumSizeHint() const override;
        QSize sizeHint() const override;

    private:
        Notification* getVisibleNotification() const;
        void connectSlots(Notification* notification);
        void onNotificationDestroyed(Notification* notification);
        void onNotificationVisibleChanged(Notification* notification);
        void onNotificationTextChanged(Notification* notification);
        void onScrollBarValueChanged();
        void onNotificationAction1Clicked();
        void updateUiAfterVisibleIndexChanged();

        Ui::NotificationBar* _ui;
        int _visibleNotificationIndex { -1 };
        bool _scrollBarUpdating { false };
        QVector<Notification*> _notifications;
        QVector<Notification*> _visibleNotifications;
    };

    /*
    class TestNotification : public Notification
    {
        Q_OBJECT
    public:
        TestNotification(QString text) : _text(text) {}

        QString notificationText() const override { return _text; }

        QString actionButton1Text() const override { return "Dismiss"; }

        bool visible() const override { return true; }

        void actionButton1Pushed() override
        {
            this->deleteLater();
        }

    private:
        QString _text;
    };
    */
}
#endif

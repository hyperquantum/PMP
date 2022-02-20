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

        void showNotification(Notification* notification);

        QSize minimumSizeHint() const override;
        QSize sizeHint() const override;

    private:
        void setUpNotification();
        void setUpFirstActionButton();

        Ui::NotificationBar* _ui;
        Notification* _notification;
    };
}
#endif

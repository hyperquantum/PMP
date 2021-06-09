/*
    Copyright (C) 2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COMPATIBILITYINTERFACEVIEW_H
#define PMP_COMPATIBILITYINTERFACEVIEW_H

#include "common/compatibilityinterfaceviewcreator.h"

#include <QObject>
#include <QPointer>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace PMP
{
    class CompatibilityInterface;

    class CompatibilityInterfaceViewCreatorImpl : public CompatibilityInterfaceViewCreator
    {
        Q_OBJECT
    public:
        CompatibilityInterfaceViewCreatorImpl(QWidget* parent, QMenu* menu);

        void createViewForInterface(CompatibilityInterface* interface) override;

    Q_SIGNALS:
        void interfaceMenuActionAdded();

    private:
        QWidget* _parent;
        QMenu* _menu;
    };

    class CompatibilityInterfaceView : public QObject
    {
        Q_OBJECT
    public:
        CompatibilityInterfaceView(QWidget* parent, CompatibilityInterface* interface,
                                   QMenu* menu);

    Q_SIGNALS:


    private Q_SLOTS:
        void menuActionTriggered();

    private:
        void createWindow();
        void focusWindow();

        QWidget* _parent;
        CompatibilityInterface* _interface;
        QPointer<QWidget> _window;
    };
}
#endif

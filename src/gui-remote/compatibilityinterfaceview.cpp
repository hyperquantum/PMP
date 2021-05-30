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

#include "compatibilityinterfaceview.h"

#include "common/compatibilityinterface.h"

#include <QAction>
#include <QMenu>
#include <QMessageBox>

namespace PMP
{

    CompatibilityInterfaceViewCreatorImpl::CompatibilityInterfaceViewCreatorImpl(
                                                                          QWidget* parent,
                                                                          QMenu* menu)
     : CompatibilityInterfaceViewCreator(parent),
       _parent(parent),
       _menu(menu)
    {
        //
    }

    void CompatibilityInterfaceViewCreatorImpl::createViewForInterface(
                                                        CompatibilityInterface* interface)
    {
        new CompatibilityInterfaceView(_parent, interface, _menu);
        Q_EMIT interfaceMenuActionAdded();
    }

    CompatibilityInterfaceView::CompatibilityInterfaceView(QWidget* parent,
                                                        CompatibilityInterface* interface,
                                                        QMenu* menu)
     : QObject(parent),
       _parent(parent)
    {
        QAction* titleAction = new QAction(interface->title(), this);
        connect(
            titleAction, &QAction::triggered,
            this,
            [this, interface]()
            {
                menuActionTriggered(interface);
            }
        );

        menu->addAction(titleAction);
    }

    void CompatibilityInterfaceView::menuActionTriggered(
                                                        CompatibilityInterface* interface)
    {
        QMessageBox::information(_parent,
                                 "CompatibilityInterfaceView",
                                 "Clicked on menu action: " + interface->title());
    }

}

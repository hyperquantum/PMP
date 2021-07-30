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

#include "compatibilityuicontrollercollection.h"

#include "compatibilityuicontrollers.h"

namespace PMP
{
    CompatibilityUiControllerCollection::CompatibilityUiControllerCollection(
            QObject* parent,
            ServerInterface* serverInterface)
     : QObject(parent),
       _serverInterface(serverInterface),
       _indexationController(nullptr),
       _testController(nullptr)
    {
        //
    }

    void CompatibilityUiControllerCollection::activateIndexationController()
    {
        if (_indexationController) /* active already? */
            return;

        _indexationController = new IndexationUiController(4890, _serverInterface);
        activateController(_indexationController);
    }

    void CompatibilityUiControllerCollection::activateTestController()
    {
        if (_testController) /* active already? */
            return;

        _testController = new TestUiController(4321, _serverInterface);
        activateController(_testController);
    }

    void CompatibilityUiControllerCollection::activateController(
                                                    CompatibilityUiController* controller)
    {
        _controllersById.insert(controller->id(), controller);

        connectSignals(controller);

        qDebug() << "activated compatibility UI controller:"
                 << "ID:" << controller->id() << ";"
                 << "title in English:"
                 << controller->getTitle(UserInterfaceLanguage::English);
    }

    void CompatibilityUiControllerCollection::connectSignals(
                                                    CompatibilityUiController* controller)
    {
        auto interfaceId = controller->id();

        connect(
            controller, &CompatibilityUiController::textChanged,
            this,
            [this, interfaceId]()
            {
                Q_EMIT textChanged(interfaceId);
            }
        );
        connect(
            controller, &CompatibilityUiController::stateChanged,
            this,
            [this, interfaceId]()
            {
                Q_EMIT stateChanged(interfaceId);
            }
        );
        connect(
            controller, &CompatibilityUiController::actionCaptionChanged,
            this,
            [this, interfaceId](int actionId)
            {
                Q_EMIT actionCaptionChanged(interfaceId, actionId);
            }
        );
        connect(
            controller, &CompatibilityUiController::actionStateChanged,
            this,
            [this, interfaceId](int actionId)
            {
                Q_EMIT actionStateChanged(interfaceId, actionId);
            }
        );
        connect(
            controller, &CompatibilityUiController::actionSuccessful,
            this,
            [this, interfaceId](int actionId, uint clientReference)
            {
                Q_EMIT actionSuccessful(interfaceId, actionId, clientReference);
            }
        );
        connect(
            controller, &CompatibilityUiController::actionFailed,
            this,
            [this, interfaceId](int actionId, uint clientReference)
            {
                Q_EMIT actionFailed(interfaceId, actionId, clientReference);
            }
        );
    }

}

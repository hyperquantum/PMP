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

#include "compatibilityuicontrollers.h"

#include "serverinterface.h"

#include <QtDebug>
#include <QTimer>

namespace PMP
{

    /* ======== IndexationUiController ======== */

    namespace
    {
        static const int startFullIndexationActionId = 5340;
    }

    IndexationUiController::IndexationUiController(int id,
                                                   ServerInterface* serverInterface)
     : CompatibilityUiController(serverInterface, id,
                                 CompatibilityUiPriority::Informational),
       _serverInterface(serverInterface),
       _fullIndexationRunning(serverInterface->isFullIndexationRunning())
    {
        connect(
            serverInterface, &ServerInterface::fullIndexationRunStatusChanged,
            this,
            [this](bool running)
            {
                if (running == _fullIndexationRunning)
                    return;

                _fullIndexationRunning = running;

                if (running)
                    setPriority(CompatibilityUiPriority::Informational);
                else
                    setPriority(CompatibilityUiPriority::Optional);

                Q_EMIT textChanged();
                Q_EMIT actionStateChanged(startFullIndexationActionId);
            }
        );
    }

    QString IndexationUiController::getTitle(UserInterfaceLanguage language) const
    {
        Q_UNUSED(language)

        /* always in English for now */
        return "Indexation";
    }

    CompatibilityUiControllerText IndexationUiController::getText(
                                                     UserInterfaceLanguage language) const
    {
        Q_UNUSED(language)

        /* always in English for now */
        CompatibilityUiControllerText text;
        //text.language = UserInterfaceLanguage::English;
        if (_fullIndexationRunning)
        {
            text.caption = "Full indexation running";
            text.description =
                    "The PMP server is performing a thorough indexation of all files.";
        }
        else
        {
            text.caption = "File indexation";
            text.description =
                    "Use this button to start a thorough indexation of all files.";
        }

        return text;
    }

    QVector<int> IndexationUiController::getActionIds() const
    {
        return { startFullIndexationActionId };
    }

    QString IndexationUiController::getActionCaption(int actionId,
                                                     UserInterfaceLanguage language) const
    {
        Q_UNUSED(language) /* always in English for now */

        if (actionId != startFullIndexationActionId)
            return {};

        return "Start full indexation";
    }

    CompatibilityUiActionState IndexationUiController::getActionState(int actionId) const
    {
        if (actionId != startFullIndexationActionId)
            return {};

        bool visible = true;
        bool enabled = !_fullIndexationRunning;
        bool disableWhenTriggered = true;

        return CompatibilityUiActionState(visible, enabled, disableWhenTriggered);
    }

    /* ======== TestUiController ======== */

    TestUiController::TestUiController(int id, ServerInterface* serverInterface)
     : CompatibilityUiController(serverInterface, id,
                                 CompatibilityUiPriority::Optional),
       _actionState(true)
    {
        auto* timer = new QTimer(this);
        timer->setInterval(5000);

        connect(
            timer, &QTimer::timeout,
            this,
            [this]()
            {
                _actionState = !_actionState;
                Q_EMIT actionCaptionChanged(1234);
            }
        );

        timer->start();
    }

    QString TestUiController::getTitle(UserInterfaceLanguage language) const
    {
        Q_UNUSED(language)

        /* always in English for now */
        return "Test";
    }

    CompatibilityUiControllerText TestUiController::getText(
                                                     UserInterfaceLanguage language) const
    {
        Q_UNUSED(language)

        /* always in English for now */
        CompatibilityUiControllerText text;
        //text.language = UserInterfaceLanguage::English;
        text.caption = "This is a test";
        text.description =
                "This is only meant as a test for the compatibility UI mechanism.";

        return text;
    }

    QVector<int> TestUiController::getActionIds() const
    {
        return { 12345, 12567, 1234 };
    }

    QString TestUiController::getActionCaption(int actionId,
                                                     UserInterfaceLanguage language) const
    {
        Q_UNUSED(language) /* always in English for now */

        if (actionId == 1234)
        {
            if (_actionState)
                return "Get to the choppa!";
            else
                return "I'll be back";
        }
        else if (actionId == 12345)
        {
            return "P. Sherman";
        }
        else if (actionId == 12567)
        {
            return "A113";
        }
        else
            return {};
    }

    CompatibilityUiActionState TestUiController::getActionState(int actionId) const
    {
        bool visible;
        bool enabled;
        bool disableWhenTriggered;

        if (actionId == 1234)
        {
            visible = true;
            enabled = true;
            disableWhenTriggered = true;
        }
        else if (actionId == 12345)
        {
            visible = true;
            enabled = false;
            disableWhenTriggered = false;
        }
        else if (actionId == 12567)
        {
            visible = false;
            enabled = true;
            disableWhenTriggered = false;
        }
        else
            return {};

        return CompatibilityUiActionState(visible, enabled, disableWhenTriggered);
    }

    /* ======== CompatibilityUiControllerCollection ======== */

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
                 << "title:" << controller->getTitle(UserInterfaceLanguage::English);
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
    }

}

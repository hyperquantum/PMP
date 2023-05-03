/*
    Copyright (C) 2021-2023, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP::Server
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

    void IndexationUiController::runActionAsync(int actionId,
                                                UserInterfaceLanguage language,
                                                uint clientReference)
    {
        Q_UNUSED(language)

        if (actionId != startFullIndexationActionId)
            return; // TODO : report error

        if (_fullIndexationRunning)
        {
            Q_EMIT actionFailed(actionId, clientReference);
            return;
        }

        _serverInterface->startFullIndexation();
        Q_EMIT actionSuccessful(actionId, clientReference);
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

    void TestUiController::runActionAsync(int actionId, UserInterfaceLanguage language,
                                          uint clientReference)
    {
        Q_UNUSED(actionId)
        Q_UNUSED(language)
        Q_UNUSED(clientReference)
        // TODO
    }
}

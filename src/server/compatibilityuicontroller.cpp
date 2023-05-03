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

#include "compatibilityuicontroller.h"

namespace PMP::Server
{
    UserInterfaceLanguage CompatibilityUiController::getSupportedLanguage(
                                                        UserInterfaceLanguage firstChoice,
                                                        UserInterfaceLanguage alternative)
    {
        if (firstChoice != UserInterfaceLanguage::Invalid)
            return firstChoice;

        if (alternative != UserInterfaceLanguage::Invalid)
            return alternative;

        return UserInterfaceLanguage::Invalid;
    }

    CompatibilityUiController::CompatibilityUiController(QObject* parent, int id,
                                                         CompatibilityUiPriority priority)
     : QObject(parent),
       _id(id),
       _state(priority)
    {
        //
    }

    CompatibilityUiController::~CompatibilityUiController()
    {
        //
    }

    void CompatibilityUiController::setState(CompatibilityUiState const& newState)
    {
        if (_state == newState)
            return; /* no change */

        _state = newState;
        Q_EMIT stateChanged();
    }

    void CompatibilityUiController::setPriority(CompatibilityUiPriority priority)
    {
        if (_state.priority() == priority)
            return; /* no change */

        _state.setPriority(priority);
        Q_EMIT stateChanged();
    }
}

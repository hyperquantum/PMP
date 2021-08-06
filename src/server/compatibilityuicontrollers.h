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

#ifndef PMP_COMPATIBILITYUICONTROLLERS_H
#define PMP_COMPATIBILITYUICONTROLLERS_H

#include "compatibilityuicontroller.h"

namespace PMP
{
    class ServerInterface;

    class IndexationUiController : public CompatibilityUiController
    {
        Q_OBJECT
    public:
        IndexationUiController(int id, ServerInterface* serverInterface);

        QString getTitle(UserInterfaceLanguage language) const override;
        CompatibilityUiControllerText getText(
                                           UserInterfaceLanguage language) const override;

        QVector<int> getActionIds() const override;
        QString getActionCaption(int actionId,
                                 UserInterfaceLanguage language) const override;
        CompatibilityUiActionState getActionState(int actionId) const override;
        void runActionAsync(int actionId, UserInterfaceLanguage language,
                            uint clientReference) override;

    private:
        ServerInterface* _serverInterface;
        bool _fullIndexationRunning;
    };

    class TestUiController : public CompatibilityUiController
    {
        Q_OBJECT
    public:
        TestUiController(int id, ServerInterface* serverInterface);

        QString getTitle(UserInterfaceLanguage language) const override;
        CompatibilityUiControllerText getText(
                                           UserInterfaceLanguage language) const override;

        QVector<int> getActionIds() const override;
        QString getActionCaption(int actionId,
                                 UserInterfaceLanguage language) const override;
        CompatibilityUiActionState getActionState(int actionId) const override;
        void runActionAsync(int actionId, UserInterfaceLanguage language,
                            uint clientReference) override;

    private:
        bool _actionState;
    };
}
#endif

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

#ifndef PMP_COMPATIBILITYINTERFACECONTROLLERIMPL_H
#define PMP_COMPATIBILITYINTERFACECONTROLLERIMPL_H

#include "compatibilityinterfacecontroller.h"
#include "compatibilityui.h"

#include <QHash>
#include <QVector>

namespace PMP
{
    class CompatibilityInterfaceImpl;
    class ServerConnection;

    class CompatibilityInterfaceControllerImpl : public CompatibilityInterfaceController
    {
        Q_OBJECT
    public:
        CompatibilityInterfaceControllerImpl(ServerConnection* connection,
                                             UserInterfaceLanguage language);

        ~CompatibilityInterfaceControllerImpl() {}

        QVector<int> interfaceIds() const;
        CompatibilityInterface* getInterface(int interfaceId) const override;

    private Q_SLOTS:
        void connected();
        void connectionBroken();

        void compatibilityInterfaceAnnouncementReceived(QVector<int> interfaceIds);
        void compatibilityInterfaceDefinitionReceived(int interfaceId,
                                                      CompatibilityUiState state,
                                                      UserInterfaceLanguage language,
                                                      QString title, QString caption,
                                                      QString description,
                                                      QVector<int> actionIds);
        void compatibilityInterfaceActionDefinitionReceived(int interfaceId, int actionId,
                                                         CompatibilityUiActionState state,
                                                         UserInterfaceLanguage language,
                                                         QString caption);
        void compatibilityInterfaceStateChanged(int interfaceId,
                                                CompatibilityUiState state);
        void compatibilityInterfaceTextChanged(int interfaceId,
                                               UserInterfaceLanguage language,
                                               QString caption, QString description);
        void compatibilityInterfaceActionStateChanged(int interfaceId, int actionId,
                                                      CompatibilityUiActionState state);
        void compatibilityInterfaceActionTextChanged(int interfaceId, int actionId,
                                                     UserInterfaceLanguage language,
                                                     QString caption);

    private:
        ServerConnection* _connection;
        UserInterfaceLanguage _language;
        QHash<int, CompatibilityInterfaceImpl*> _interfaces;
    };

}
#endif

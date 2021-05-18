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

#include "compatibilityinterfacecontrollerimpl.h"

#include "compatibilityinterfaceimpl.h"
#include "serverconnection.h"

namespace PMP
{
    CompatibilityInterfaceControllerImpl::CompatibilityInterfaceControllerImpl(
                                                           ServerConnection* connection,
                                                           UserInterfaceLanguage language)
     : CompatibilityInterfaceController(connection),
       _language(language)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &CompatibilityInterfaceControllerImpl::connected
        );
        connect(
            _connection, &ServerConnection::connectionBroken,
            this, &CompatibilityInterfaceControllerImpl::connectionBroken
        );

        connect(
            _connection, &ServerConnection::compatibilityInterfaceAnnouncementReceived,
            this,
            &CompatibilityInterfaceControllerImpl::compatibilityInterfaceAnnouncementReceived
        );
        connect(
            _connection, &ServerConnection::compatibilityInterfaceDefinitionReceived,
            this,
            &CompatibilityInterfaceControllerImpl::compatibilityInterfaceDefinitionReceived
        );
        connect(
            _connection,
            &ServerConnection::compatibilityInterfaceActionDefinitionReceived,
            this,
            &CompatibilityInterfaceControllerImpl::compatibilityInterfaceActionDefinitionReceived
        );
        connect(
            _connection, &ServerConnection::compatibilityInterfaceStateChanged,
            this,
            &CompatibilityInterfaceControllerImpl::compatibilityInterfaceStateChanged
        );
        connect(
            _connection, &ServerConnection::compatibilityInterfaceTextChanged,
            this, &CompatibilityInterfaceControllerImpl::compatibilityInterfaceTextChanged
        );
        connect(
            _connection, &ServerConnection::compatibilityInterfaceActionStateChanged,
            this,
            &CompatibilityInterfaceControllerImpl::compatibilityInterfaceActionStateChanged
        );
        connect(
            _connection, &ServerConnection::compatibilityInterfaceActionTextChanged,
            this,
            &CompatibilityInterfaceControllerImpl::compatibilityInterfaceActionTextChanged
        );

        if (_connection->isConnected())
            connected();
    }

    QVector<int> CompatibilityInterfaceControllerImpl::interfaceIds() const
    {
        return _interfaces.keys().toVector();
    }

    CompatibilityInterface* CompatibilityInterfaceControllerImpl::getInterface(
                                                                    int interfaceId) const
    {
        return _interfaces.value(interfaceId, nullptr);
    }

    void CompatibilityInterfaceControllerImpl::connected()
    {
        auto interfaceIds = _connection->getCompatibilityInterfaceIds();

        _connection->sendCompatibilityInterfaceDefinitionsRequest(_language,
                                                                  interfaceIds);
    }

    void CompatibilityInterfaceControllerImpl::connectionBroken()
    {
        auto interfaces = _interfaces.values();
        _interfaces.clear();

        qDeleteAll(interfaces);
    }

    void CompatibilityInterfaceControllerImpl::compatibilityInterfaceAnnouncementReceived(
                                                                QVector<int> interfaceIds)
    {
        _connection->sendCompatibilityInterfaceDefinitionsRequest(_language,
                                                                  interfaceIds);
    }

    void CompatibilityInterfaceControllerImpl::compatibilityInterfaceDefinitionReceived(
                                                           int interfaceId,
                                                           CompatibilityUiState state,
                                                           UserInterfaceLanguage language,
                                                           QString title, QString caption,
                                                           QString description,
                                                           QVector<int> actionIds)
    {
        auto* interface = _interfaces.value(interfaceId, nullptr);
        if (!interface)
        {
            interface = new CompatibilityInterfaceImpl(this, interfaceId, state, language,
                                                       title, caption, description,
                                                       actionIds);

            _interfaces.insert(interfaceId, interface);

            Q_EMIT newInterfaceNowAvailable(interfaceId);
        }
        else
        {
            interface->setState(state);
            interface->setText(language, title, caption, description);
        }
    }

    void CompatibilityInterfaceControllerImpl::compatibilityInterfaceActionDefinitionReceived(
                                                         int interfaceId, int actionId,
                                                         CompatibilityUiActionState state,
                                                         UserInterfaceLanguage language,
                                                         QString caption)
    {
        auto* interface = _interfaces.value(interfaceId, nullptr);
        if (!interface)
            return;

        auto* action = interface->getAction(actionId);
        if (!action)
            return;

        action->setState(state);
        action->setCaption(language, caption);
    }

    void CompatibilityInterfaceControllerImpl::compatibilityInterfaceStateChanged(
                                                               int interfaceId,
                                                               CompatibilityUiState state)
    {
        auto* interface = _interfaces.value(interfaceId, nullptr);
        if (!interface)
            return;

        interface->setState(state);
    }

    void CompatibilityInterfaceControllerImpl::compatibilityInterfaceTextChanged(
                                                           int interfaceId,
                                                           UserInterfaceLanguage language,
                                                           QString caption,
                                                           QString description)
    {
        auto* interface = _interfaces.value(interfaceId, nullptr);
        if (!interface)
            return;

        interface->setText(language, caption, description);
    }

    void CompatibilityInterfaceControllerImpl::compatibilityInterfaceActionStateChanged(
                                                         int interfaceId, int actionId,
                                                         CompatibilityUiActionState state)
    {
        auto* interface = _interfaces.value(interfaceId, nullptr);
        if (!interface)
            return;

        auto* action = interface->getAction(actionId);
        if (!action)
            return;

        action->setState(state);
    }

    void CompatibilityInterfaceControllerImpl::compatibilityInterfaceActionTextChanged(
                                                           int interfaceId, int actionId,
                                                           UserInterfaceLanguage language,
                                                           QString caption)
    {
        auto* interface = _interfaces.value(interfaceId, nullptr);
        if (!interface)
            return;

        auto* action = interface->getAction(actionId);
        if (!action)
            return;

        action->setCaption(language, caption);
    }

}

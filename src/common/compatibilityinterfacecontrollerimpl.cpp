/*
    Copyright (C) 2021-2022, Kevin Andre <hyperquantum@gmail.com>

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
#include "compatibilityinterfaceviewcreator.h"
#include "containerutil.h"
#include "serverconnection.h"

#include <QTimer>

namespace PMP
{
    CompatibilityInterfaceControllerImpl::CompatibilityInterfaceControllerImpl(
                                                           ServerConnection* connection,
                                                           UserInterfaceLanguage language)
     : CompatibilityInterfaceController(connection),
       _connection(connection),
       _languagePreferred(language),
       _languageConfirmed(UserInterfaceLanguage::Invalid),
       _canFetchDefinitions(false)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &CompatibilityInterfaceControllerImpl::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &CompatibilityInterfaceControllerImpl::disconnected
        );

        connect(
            _connection, &ServerConnection::compatibilityInterfaceAnnouncementReceived,
            this,
            &CompatibilityInterfaceControllerImpl::compatibilityInterfaceAnnouncementReceived
        );
        connect(
            _connection,
            &ServerConnection::compatibilityInterfaceLanguageSelectionSucceeded,
            this,
            &CompatibilityInterfaceControllerImpl::compatibilityInterfaceLanguageSelectionSucceeded
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
        return ContainerUtil::keysToVector(_interfaces);
    }

    CompatibilityInterface* CompatibilityInterfaceControllerImpl::getInterface(
                                                                    int interfaceId) const
    {
        return _interfaces.value(interfaceId, nullptr);
    }

    void CompatibilityInterfaceControllerImpl::registerViewCreator(
                                               CompatibilityInterfaceViewCreator* creator)
    {
        connect(
            this, &CompatibilityInterfaceControllerImpl::newInterfaceNowAvailable,
            this,
            [this, creator](int interfaceId)
            {
                auto* interface = _interfaces[interfaceId];

                QTimer::singleShot(
                    0, creator,
                    [creator, interface]() { creator->createViewForInterface(interface); }
                );
            }
        );

        for (auto interface : qAsConst(_interfaces))
        {
            QTimer::singleShot(
                0, creator,
                [creator, interface]() { creator->createViewForInterface(interface); }
            );
        }
    }

    void CompatibilityInterfaceControllerImpl::connected()
    {
        _connection->sendCompatibilityInterfaceLanguageSelectionRequest(
                                                                      _languagePreferred);

        auto interfaceIds = _connection->getCompatibilityInterfaceIds();

        for (auto interfaceId : qAsConst(interfaceIds))
            _interfaceDefinitionsToFetch.insert(interfaceId);

        /* this is a failsafe in case language selection is never confirmed */
        QTimer::singleShot(
            500, this,
            [this]()
            {
                _canFetchDefinitions = true;
                fetchDefinitionsPending();
            }
        );
    }

    void CompatibilityInterfaceControllerImpl::disconnected()
    {
        _canFetchDefinitions = false;

        auto interfaces = _interfaces.values();
        _interfaces.clear();
        qDeleteAll(interfaces);
    }

    void CompatibilityInterfaceControllerImpl::compatibilityInterfaceAnnouncementReceived(
                                                                QVector<int> interfaceIds)
    {
        ContainerUtil::addToSet(interfaceIds, _interfaceDefinitionsToFetch);

        fetchDefinitionsPending();
    }

    void CompatibilityInterfaceControllerImpl::compatibilityInterfaceLanguageSelectionSucceeded(
                                                           UserInterfaceLanguage language)
    {
        _languageConfirmed = language;
        qDebug() << "compatibility interface language successfully set to" << language;

        _canFetchDefinitions = true;
        fetchDefinitionsPending();
    }

    void CompatibilityInterfaceControllerImpl::compatibilityInterfaceDefinitionReceived(
                                                           int interfaceId,
                                                           CompatibilityUiState state,
                                                           UserInterfaceLanguage language,
                                                           QString title, QString caption,
                                                           QString description,
                                                           QVector<int> actionIds)
    {
        if (_languageConfirmed == UserInterfaceLanguage::Invalid
                && language != UserInterfaceLanguage::Invalid)
        {
            _languageConfirmed = language;
            qDebug() << "confirmed language set to" << language
                     << "upon receiving compatibility interface definition";
        }

        if (language != _languageConfirmed)
        {
            qWarning() << "compatibility interface definition language wrong:"
                       << "expecting" << _languageConfirmed << "but received" << language;
        }

        auto* interface = _interfaces.value(interfaceId, nullptr);
        if (!interface)
        {
            interface = new CompatibilityInterfaceImpl(this, interfaceId, state, language,
                                                       title, caption, description,
                                                       actionIds);

            _interfaces.insert(interfaceId, interface);

            connect(
                interface, &CompatibilityInterfaceImpl::actionTriggerRequested,
                _connection,
                [this, interfaceId](int actionId)
                {
                    auto requestId =
                            _connection->sendCompatibilityInterfaceTriggerActionRequest(
                                                                              interfaceId,
                                                                              actionId);
                    // TODO : use the request ID to track the result of the request
                    Q_UNUSED(requestId)
                }
            );

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
        if (language != _languageConfirmed)
        {
            qWarning() << "compatibility interface definition language wrong:"
                       << "expecting" << _languageConfirmed << "but received" << language;
        }

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
        if (language != _languageConfirmed)
        {
            qWarning() << "compatibility interface definition language wrong:"
                       << "expecting" << _languageConfirmed << "but received" << language;
        }

        auto* interface = _interfaces.value(interfaceId, nullptr);
        if (!interface)
            return;

        auto* action = interface->getAction(actionId);
        if (!action)
            return;

        action->setCaption(language, caption);
    }

    void CompatibilityInterfaceControllerImpl::fetchDefinitionsPending()
    {
        if (!_canFetchDefinitions)
            return;

        if (_interfaceDefinitionsToFetch.isEmpty())
            return;

        auto interfaceIds = ContainerUtil::flushIntoVector(_interfaceDefinitionsToFetch);

        _connection->sendCompatibilityInterfaceDefinitionsRequest(interfaceIds);
    }

}

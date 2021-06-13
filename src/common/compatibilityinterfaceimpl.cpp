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

#include "compatibilityinterfaceimpl.h"

namespace PMP
{

    CompatibilityInterfaceActionImpl::CompatibilityInterfaceActionImpl(
                                                       CompatibilityInterfaceImpl* parent,
                                                       int id)
     : CompatibilityInterfaceAction(parent),
       _parent(parent),
       _id(id),
       _state()
    {
        //
    }

    CompatibilityUiActionState CompatibilityInterfaceActionImpl::state() const
    {
        return _state;
    }

    void CompatibilityInterfaceActionImpl::setState(
                                                  const CompatibilityUiActionState& state)
    {
        if (_state == state)
            return;

        _state = state;
        Q_EMIT stateChanged();
    }

    QString CompatibilityInterfaceActionImpl::caption() const
    {
        return _caption;
    }

    void CompatibilityInterfaceActionImpl::setCaption(UserInterfaceLanguage language,
                                                      QString caption)
    {
        if (_parent->language() != language)
            return; /* wrong language */

        if (_caption == caption)
            return;

        _caption = caption;
        Q_EMIT captionChanged();
    }

    CompatibilityInterfaceImpl::CompatibilityInterfaceImpl(QObject* parent, int id,
                                                        CompatibilityUiState const& state,
                                                        UserInterfaceLanguage language,
                                                        QString title, QString caption,
                                                        QString description,
                                                        QVector<int> actionIds)
     : CompatibilityInterface(parent),
       _id(id),
       _state(state),
       _language(language),
       _title(title),
       _caption(caption),
       _description(description)
    {
        for (int actionId : actionIds)
        {
            auto* action = new CompatibilityInterfaceActionImpl(this, actionId);
            _actions.insert(actionId, action);
        }
    }

    CompatibilityUiState CompatibilityInterfaceImpl::state() const
    {
        return _state;
    }

    void CompatibilityInterfaceImpl::setState(const CompatibilityUiState& state)
    {
        if (_state == state)
            return;

        _state = state;
        Q_EMIT stateChanged();
    }

    QString CompatibilityInterfaceImpl::title() const
    {
        return _title;
    }

    QString CompatibilityInterfaceImpl::caption() const
    {
        return _caption;
    }

    QString CompatibilityInterfaceImpl::description() const
    {
        return _description;
    }

    void CompatibilityInterfaceImpl::setText(UserInterfaceLanguage language,
                                             QString caption, QString description)
    {
        if (_language != language)
            return; /* wrong language */

        if (_caption == caption && _description == description)
            return; /* nothing changes */

        _caption = caption;
        _description = description;
        Q_EMIT textChanged();
    }

    void CompatibilityInterfaceImpl::setText(UserInterfaceLanguage language,
                                             QString title, QString caption,
                                             QString description)
    {
        if (_language != language)
            return; /* wrong language */

        if (_title == title && _caption == caption && _description == description)
            return; /* nothing changes */

        _title = title;
        _caption = caption;
        _description = description;
        Q_EMIT textChanged();
    }

    QVector<int> CompatibilityInterfaceImpl::getActionIds() const
    {
        return _actions.keys().toVector();
    }

    CompatibilityInterfaceActionImpl* CompatibilityInterfaceImpl::getAction(
                                                                       int actionId) const
    {
        return _actions.value(actionId, nullptr);
    }

}

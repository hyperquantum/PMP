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

#ifndef PMP_COMPATIBILITYUI_H
#define PMP_COMPATIBILITYUI_H

#include <QtDebug>

namespace PMP
{
    enum class UserInterfaceLanguage
    {
        Invalid = 0,
        English,
    };

    inline QDebug operator<<(QDebug debug, UserInterfaceLanguage language)
    {
        switch (language)
        {
            case UserInterfaceLanguage::Invalid:
                debug << "UserInterfaceLanguage::Invalid";
                return debug;

            case UserInterfaceLanguage::English:
                debug << "UserInterfaceLanguage::English";
                return debug;
        }

        debug << int(language);
        return debug;
    }

    enum class CompatibilityUiPriority
    {
        Undefined = 0,
        Optional,
        Informational,
        Important,
        Urgent,
    };

    inline QDebug operator<<(QDebug debug, CompatibilityUiPriority uiPriority)
    {
        switch (uiPriority)
        {
            case CompatibilityUiPriority::Undefined:
                debug << "CompatibilityUiPriority::Undefined";
                return debug;

            case CompatibilityUiPriority::Optional:
                debug << "CompatibilityUiPriority::Optional";
                return debug;

            case CompatibilityUiPriority::Informational:
                debug << "CompatibilityUiPriority::Informational";
                return debug;

            case CompatibilityUiPriority::Important:
                debug << "CompatibilityUiPriority::Important";
                return debug;

            case CompatibilityUiPriority::Urgent:
                debug << "CompatibilityUiPriority::Urgent";
                return debug;
        }

        debug << int(uiPriority);
        return debug;
    }

    class CompatibilityUiState
    {
    public:
        CompatibilityUiState() : _priority(CompatibilityUiPriority::Undefined) {}
        CompatibilityUiState(CompatibilityUiPriority priority) : _priority(priority) {}

        CompatibilityUiPriority priority() const { return _priority; }
        void setPriority(CompatibilityUiPriority priority) { _priority = priority; }

    private:
        CompatibilityUiPriority _priority;
    };

    inline bool operator==(CompatibilityUiState const& state1,
                           CompatibilityUiState const& state2)
    {
        return state1.priority() == state2.priority();
    }

    inline bool operator!=(CompatibilityUiState const& state1,
                           CompatibilityUiState const& state2)
    {
        return !(state1 == state2);
    }

    class CompatibilityUiActionState
    {
    public:
        CompatibilityUiActionState()
         : _visible(false),
           _enabled(false),
           _disableWhenTriggered(false)
        {
            //
        }

        CompatibilityUiActionState(bool visible, bool enabled, bool disableWhenTriggered)
         : _visible(visible),
           _enabled(enabled),
           _disableWhenTriggered(disableWhenTriggered)
        {
            //
        }

        bool visible() const { return _visible; }
        void setVisible(bool visible) { _visible = visible; }

        bool enabled() const { return _enabled; }
        void setEnabled(bool enabled) { _enabled = enabled; }

        bool disableWhenTriggered() const { return _disableWhenTriggered; }
        void setDisableWhenTriggered(bool disableWhenTriggered)
        {
            _disableWhenTriggered = disableWhenTriggered;
        }

    private:
        bool _visible;
        bool _enabled;
        bool _disableWhenTriggered;
    };

    inline bool operator==(CompatibilityUiActionState const& state1,
                           CompatibilityUiActionState const& state2)
    {
        return state1.visible() == state2.visible()
            && state1.enabled() == state2.enabled();
    }

    inline bool operator!=(CompatibilityUiActionState const& state1,
                           CompatibilityUiActionState const& state2)
    {
        return !(state1 == state2);
    }
}

Q_DECLARE_METATYPE(PMP::CompatibilityUiActionState)
Q_DECLARE_METATYPE(PMP::CompatibilityUiState)
Q_DECLARE_METATYPE(PMP::UserInterfaceLanguage)

#endif

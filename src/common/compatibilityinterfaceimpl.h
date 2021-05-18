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

#ifndef PMP_COMPATIBILITYINTERFACEIMPL_H
#define PMP_COMPATIBILITYINTERFACEIMPL_H

#include "compatibilityinterface.h"

#include <QHash>
#include <QVector>

namespace PMP
{

    class CompatibilityInterfaceImpl;

    class CompatibilityInterfaceActionImpl : public CompatibilityInterfaceAction
    {
        Q_OBJECT
    public:
        CompatibilityInterfaceActionImpl(CompatibilityInterfaceImpl* parent, int id);

        int id() const override { return _id; }

        CompatibilityUiActionState state() const override;
        void setState(CompatibilityUiActionState const& state);

        QString caption() const override;
        void setCaption(UserInterfaceLanguage language, QString caption);

    private:
        CompatibilityInterfaceImpl* _parent;
        int _id;
        CompatibilityUiActionState _state;
        QString _caption;
    };

    class CompatibilityInterfaceImpl : public CompatibilityInterface
    {
        Q_OBJECT
    public:
        CompatibilityInterfaceImpl(QObject* parent, int id,
                                   CompatibilityUiState const& state,
                                   UserInterfaceLanguage language, QString title,
                                   QString caption, QString description,
                                   QVector<int> actionIds);

        int id() const override { return _id; }
        UserInterfaceLanguage language() const { return _language; }

        CompatibilityUiState state() const override;
        void setState(CompatibilityUiState const& state);

        QString title() const override;
        QString caption() const override;
        QString description() const override;
        void setText(UserInterfaceLanguage language, QString caption,
                     QString description);
        void setText(UserInterfaceLanguage language, QString title, QString caption,
                     QString description);

        CompatibilityInterfaceActionImpl* getAction(int actionId) const override;

    private:
        int _id;
        CompatibilityUiState _state;
        UserInterfaceLanguage _language;
        QString _title;
        QString _caption;
        QString _description;
        QHash<int, CompatibilityInterfaceActionImpl*> _actions;
    };
}
#endif

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

#ifndef PMP_COMPATIBILITYUICONTROLLER_H
#define PMP_COMPATIBILITYUICONTROLLER_H

#include "common/compatibilityui.h"

#include <QObject>
#include <QVector>

namespace PMP
{
    struct CompatibilityUiControllerText
    {
        QString caption;
        QString description;
    };

    class CompatibilityUiController : public QObject
    {
        Q_OBJECT
    public:

        static UserInterfaceLanguage getSupportedLanguage(
                                                       UserInterfaceLanguage firstChoice,
                                                       UserInterfaceLanguage alternative);

        CompatibilityUiController(QObject* parent, int id,
                                  CompatibilityUiPriority priority);
        virtual ~CompatibilityUiController();

        int id() const { return _id; }
        virtual QString getTitle(UserInterfaceLanguage language) const = 0;
        virtual CompatibilityUiControllerText getText(
                                                UserInterfaceLanguage language) const = 0;
        CompatibilityUiState getState() const { return _state; }

        virtual QVector<int> getActionIds() const = 0;
        virtual QString getActionCaption(int actionId,
                                         UserInterfaceLanguage language) const = 0;
        virtual CompatibilityUiActionState getActionState(int actionId) const = 0;
        virtual void runActionAsync(int actionId, uint clientReference) = 0;

    Q_SIGNALS:
        void textChanged();
        void stateChanged();
        void actionCaptionChanged(int actionId);
        void actionStateChanged(int actionId);
        void actionSuccessful(int actionId, uint clientReference);
        void actionFailed(int actionId, uint clientReference);

    protected:
        void setState(CompatibilityUiState const& newState);
        void setPriority(CompatibilityUiPriority priority);

    private:
        int _id;
        CompatibilityUiState _state;
    };
}
#endif

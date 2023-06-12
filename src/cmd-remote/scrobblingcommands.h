/*
    Copyright (C) 2022-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SCROBBLINGCOMMANDS_H
#define PMP_SCROBBLINGCOMMANDS_H

#include "commandbase.h"

#include "common/scrobblingprovider.h"

namespace PMP
{
    class ScrobblingActivationCommand : public CommandBase
    {
        Q_OBJECT
    public:
        ScrobblingActivationCommand(ScrobblingProvider provider, bool enable);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        ScrobblingProvider _provider;
        bool _enable;
    };

    class ScrobblingStatusCommand : public CommandBase
    {
        Q_OBJECT
    public:
        ScrobblingStatusCommand(ScrobblingProvider provider);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        ScrobblingProvider _provider;
    };

    class ScrobblingAuthenticateCommand : public CommandBase
    {
        Q_OBJECT
    public:
        ScrobblingAuthenticateCommand(ScrobblingProvider provider);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        ScrobblingProvider _provider;
    };
}
#endif

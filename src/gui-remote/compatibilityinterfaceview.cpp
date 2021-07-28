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

#include "compatibilityinterfaceview.h"

#include "common/compatibilityinterface.h"

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace PMP
{

    /* ======= CompatibilityInterfaceViewCreatorImpl =======*/

    CompatibilityInterfaceViewCreatorImpl::CompatibilityInterfaceViewCreatorImpl(
                                                                          QWidget* parent,
                                                                          QMenu* menu)
     : CompatibilityInterfaceViewCreator(parent),
       _parent(parent),
       _menu(menu)
    {
        //
    }

    void CompatibilityInterfaceViewCreatorImpl::createViewForInterface(
                                                        CompatibilityInterface* interface)
    {
        new CompatibilityInterfaceView(_parent, interface, _menu);
        Q_EMIT interfaceMenuActionAdded();
    }

    /* ======= CompatibilityInterfaceView =======*/

    CompatibilityInterfaceView::CompatibilityInterfaceView(QWidget* parent,
                                                        CompatibilityInterface* interface,
                                                        QMenu* menu)
     : QObject(parent),
       _parent(parent),
       _interface(interface)
    {
        QAction* titleAction = new QAction(interface->title(), this);
        connect(
            titleAction, &QAction::triggered,
            this, &CompatibilityInterfaceView::menuActionTriggered
        );

        menu->addAction(titleAction);
    }

    void CompatibilityInterfaceView::menuActionTriggered()
    {
        if (!_window)
            createWindow();
        else
            focusWindow();
    }

    void CompatibilityInterfaceView::createWindow()
    {
        QWidget* window = new QWidget(nullptr, Qt::Tool | Qt::WindowStaysOnTopHint);
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->setWindowTitle(_interface->title());

        QLabel* captionLabel = new QLabel();
        captionLabel->setTextFormat(Qt::PlainText);
        captionLabel->setText(_interface->caption());
        captionLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

        QLabel* descriptionLabel = new QLabel();
        descriptionLabel->setTextFormat(Qt::PlainText);
        descriptionLabel->setText(_interface->description());
        descriptionLabel->setWordWrap(true);

        QVBoxLayout* verticalLayout = new QVBoxLayout(window);
        verticalLayout->addWidget(captionLabel);
        verticalLayout->addWidget(descriptionLabel);

        connect(
            _interface, &CompatibilityInterface::textChanged,
            window,
            [this, captionLabel, descriptionLabel]()
            {
                captionLabel->setText(_interface->caption());
                descriptionLabel->setText(_interface->description());
            }
        );

        auto actionIds = _interface->getActionIds();
        for (auto actionId : qAsConst(actionIds))
        {
            auto* action = _interface->getAction(actionId);
            auto state = action->state();
            auto* button = new QPushButton(action->caption());
            button->setEnabled(state.enabled());
            button->setVisible(state.visible());

            verticalLayout->addWidget(button);

            connect(
                action, &CompatibilityInterfaceAction::captionChanged,
                button, [action, button]() { button->setText(action->caption()); }
            );
            connect(
                action, &CompatibilityInterfaceAction::stateChanged,
                button,
                [action, button]()
                {
                    auto state = action->state();
                    button->setEnabled(state.enabled());
                    button->setVisible(state.visible());
                }
            );
            connect(
                button, &QPushButton::clicked,
                this, [this, action, button]() { triggerInterfaceAction(action, button); }
            );
        }

        _window = window;

        window->show();
    }

    void CompatibilityInterfaceView::focusWindow()
    {
        if (_window->isMinimized())
        {
            auto oldState = _window->windowState();
            _window->setWindowState((oldState & ~Qt::WindowMinimized) | Qt::WindowActive);
        }

        _window->activateWindow();
        _window->raise();
    }

    void CompatibilityInterfaceView::triggerInterfaceAction(
                                                     CompatibilityInterfaceAction* action,
                                                     QPushButton* button)
    {
        if (action->state().disableWhenTriggered())
            button->setEnabled(false);

        action->triggerAction();
    }

}

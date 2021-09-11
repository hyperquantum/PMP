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

#include "clickablelabel.h"

#include <QLayout>

namespace PMP
{

    ClickableLabel::ClickableLabel(QWidget* parent, Qt::WindowFlags f)
     : QLabel(parent, f)
    {
        setCursor(QCursor(Qt::PointingHandCursor));
    }

    ClickableLabel::~ClickableLabel()
    {
        //
    }

    ClickableLabel* ClickableLabel::replace(QLabel*& existingLabel)
    {
        auto* clickable = new ClickableLabel();
        clickable->setText(existingLabel->text());

        auto parent = existingLabel->parentWidget();

        auto layoutItem = parent->layout()->replaceWidget(existingLabel, clickable);

        delete layoutItem;
        delete existingLabel;

        existingLabel = clickable;
        return clickable;
    }

    void ClickableLabel::mousePressEvent(QMouseEvent* event)
    {
        Q_UNUSED(event)

        Q_EMIT clicked();
    }

}

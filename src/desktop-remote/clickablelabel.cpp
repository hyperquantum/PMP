/*
    Copyright (C) 2021-2026, Kevin André <hyperquantum@gmail.com>

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
#include <QMouseEvent>

namespace PMP
{
    ClickableLabel::ClickableLabel(QWidget* parent, Qt::WindowFlags f)
     : QLabel(parent, f)
    {
        setCursor(Qt::PointingHandCursor);
    }

    void ClickableLabel::setClickable(bool clickable)
    {
        if (_clickable == clickable)
            return;

        _clickable = clickable;

        if (_clickable)
            setCursor(Qt::PointingHandCursor);
        else
            unsetCursor();

        Q_EMIT clickableChanged();
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

        if (!_clickable)
            return;

        Q_EMIT clicked(event->pos());
    }
}

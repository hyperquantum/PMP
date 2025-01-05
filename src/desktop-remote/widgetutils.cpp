/*
    Copyright (C) 2025, Kevin André <hyperquantum@gmail.com>

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

#include "widgetutils.h"

#include <QFont>
#include <QFontMetrics>
#include <QLabel>

namespace PMP::WidgetUtils
{
    void setRelativeFontSize(QLabel* label, double scaleFactor)
    {
        QFont font = label->font();
        double currentPointSize = font.pointSize();

        /* If the font size is not explicitly set (pointSize <= 0), calculate it
               from QFontMetrics */
        if (currentPointSize <= 0)
        {
            QFontMetrics metrics(font);

            /* approximation to convert height to point size */
            currentPointSize = metrics.height() / 1.2;
        }

        font.setPointSizeF(currentPointSize * scaleFactor);
        label->setFont(font);
    }
}

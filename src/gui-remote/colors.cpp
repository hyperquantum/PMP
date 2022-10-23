/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "colors.h"

namespace PMP
{
    Colors::Colors(const QColor& widgetBorder,
                   const QColor& inactiveItemForeground,
                   QVector<QColor> itemBackgroundHighlightColors,
                   const QColor& specialQueueItemBackground,
                   const QColor& specialQueueItemForeground,
                   const QColor& trackProgressWidgetEmpty,
                   const QColor& trackProgressWidgetBackground,
                   const QColor& trackProgressWidgetBorder,
                   const QColor& trackProgressWidgetProgress)
     : widgetBorder(widgetBorder),
       inactiveItemForeground(inactiveItemForeground),
       itemBackgroundHighlightColors(itemBackgroundHighlightColors),
       specialQueueItemBackground(specialQueueItemBackground),
       specialQueueItemForeground(specialQueueItemForeground),
       trackProgressWidgetEmpty(trackProgressWidgetEmpty),
       trackProgressWidgetBackground(trackProgressWidgetBackground),
       trackProgressWidgetBorder(trackProgressWidgetBorder),
       trackProgressWidgetProgress(trackProgressWidgetProgress)
    {
        //
    }

    const Colors& Colors::instance()
    {
        return _lightScheme;
    }

    const Colors Colors::_lightScheme =
        Colors(
            /* widgetBorder */ QColor::fromRgb(0x7A, 0x7A, 0x7A),
            /* inactiveItemForeground */ Qt::gray,
            /* itemBackgroundHighlightColors */ {
                Qt::yellow,
                QColor(0xAF, 0xEE, 0xEE), // paleturquoise
                QColor(0xFF, 0x69, 0xB4)  // hotpink
            },
            /* specialQueueItemBackground */ QColor::fromRgb(0xFF, 0xC4, 0x73),
            /* specialQueueItemForeground */ Qt::black,
            /* trackProgressWidgetEmpty */ QColor::fromHsl(207, 255, 230),
            /* trackProgressWidgetBackground */ QColor::fromHsl(207, 255, 230),
            /* trackProgressWidgetBorder */ QColor::fromHsl(207, 255, 180),
            /* trackProgressWidgetProgress */ QColor::fromHsl(207, 255, 180)
        );

}

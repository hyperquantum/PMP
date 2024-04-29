/*
    Copyright (C) 2020-2024, Kevin Andre <hyperquantum@gmail.com>

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
                   const QColor& historyErrorItemBackground,
                   const QColor& historyErrorItemForeground,
                   const QColor& trackProgressWidgetEmpty,
                   const QColor& trackProgressWidgetBackground,
                   const QColor& trackProgressWidgetBorder,
                   const QColor& trackProgressWidgetProgress,
                   const QColor& linkText,
                   const QColor& spinnerBackground,
                   const QColor& spinnerLines)
     : widgetBorder(widgetBorder),
       inactiveItemForeground(inactiveItemForeground),
       itemBackgroundHighlightColors(itemBackgroundHighlightColors),
       specialQueueItemBackground(specialQueueItemBackground),
       specialQueueItemForeground(specialQueueItemForeground),
       historyErrorItemBackground(historyErrorItemBackground),
       historyErrorItemForeground(historyErrorItemForeground),
       trackProgressWidgetEmpty(trackProgressWidgetEmpty),
       trackProgressWidgetBackground(trackProgressWidgetBackground),
       trackProgressWidgetBorder(trackProgressWidgetBorder),
       trackProgressWidgetProgress(trackProgressWidgetProgress),
       linkText(linkText),
       spinnerBackground(spinnerBackground),
       spinnerLines(spinnerLines)
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
            /* itemBackgroundHighlightColors */
            {
                QRgb(0xFFFF00),
                QRgb(0x2EC0FF),
                QRgb(0xFF8FC7),
                QRgb(0x00FF67),
            },
            /* specialQueueItemBackground */ QRgb(0xFFB866),
            /* specialQueueItemForeground */ Qt::black,
            /* historyErrorItemBackground */ Qt::white,
            /* historyErrorItemForeground */ Qt::red,
            /* trackProgressWidgetEmpty */ QRgb(0xCCF0FF),
            /* trackProgressWidgetBackground */ QRgb(0xCCF0FF),
            /* trackProgressWidgetBorder */ QRgb(0x0AB5FF),
            /* trackProgressWidgetProgress */ QRgb(0x0AB5FF),
            /* linkText */ Qt::darkGreen, // TODO : find a real color
            /* spinnerBackground */ Qt::white,
            /* spinnerLines */ Qt::black
        );

}

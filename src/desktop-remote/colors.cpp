/*
    Copyright (C) 2020-2025, Kevin André <hyperquantum@gmail.com>

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

#include <QGuiApplication>
#include <QStyleHints>
#include <QtGlobal>

namespace PMP
{
    Colors::Colors(const QColor& inactiveItemForeground,
                   QVector<QColor> itemBackgroundHighlightColors,
                   const QColor& specialQueueItemBackground,
                   const QColor& specialQueueItemForeground,
                   const QColor& historyErrorItemBackground,
                   const QColor& historyErrorItemForeground,
                   const QColor& trackProgressWidgetEmpty,
                   const QColor& trackProgressWidgetBackground,
                   const QColor& trackProgressWidgetBorder,
                   const QColor& trackProgressWidgetProgress,
                   const QColor& notificationBarBackground,
                   const QColor& notificationBarBorder,
                   const QColor& notificationBarText)
     : inactiveItemForeground(inactiveItemForeground),
       itemBackgroundHighlightColors(itemBackgroundHighlightColors),
       specialQueueItemBackground(specialQueueItemBackground),
       specialQueueItemForeground(specialQueueItemForeground),
       historyErrorItemBackground(historyErrorItemBackground),
       historyErrorItemForeground(historyErrorItemForeground),
       trackProgressWidgetEmpty(trackProgressWidgetEmpty),
       trackProgressWidgetBackground(trackProgressWidgetBackground),
       trackProgressWidgetBorder(trackProgressWidgetBorder),
       trackProgressWidgetProgress(trackProgressWidgetProgress),
       notificationBarBackground(notificationBarBackground),
       notificationBarBorder(notificationBarBorder),
       notificationBarText(notificationBarText)
    {
        //
    }

    bool Colors::isDarkMode()
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
        return false;
#endif
    }

    const Colors& Colors::instance()
    {
        return isDarkMode() ? _darkScheme : _lightScheme;
    }

    const Colors Colors::_lightScheme =
        Colors(
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
            /* notificationBarBackground */ QRgb(0xFFFFFF),
            /* notificationBarBorder */ QRgb(0x0AB5FF),
            /* notificationBarText */ QRgb(0x000000)
        );

    const Colors Colors::_darkScheme =
        Colors(
            /* inactiveItemForeground */ Qt::gray,
            /* itemBackgroundHighlightColors */ {
                QColor::fromHsl(120, 255, 50),
                QColor::fromHsl(0, 255, 50),
                QColor::fromHsl(300, 255, 50),
            },
            /* specialQueueItemBackground */ QColor::fromRgb(50, 65, 75),
            /* specialQueueItemForeground */ QColor::fromRgb(20, 140, 210),
            /* historyErrorItemBackground */ Qt::darkRed,
            /* historyErrorItemForeground */ Qt::lightGray,
            /* trackProgressWidgetEmpty */ QRgb(0x004766),
            /* trackProgressWidgetBackground */ QRgb(0x002433),
            /* trackProgressWidgetBorder */ QRgb(0x006B99),
            /* trackProgressWidgetProgress */ QRgb(0x006B99),
            /* notificationBarBackground */ QRgb(0x2E2E2E),
            /* notificationBarBorder */ QRgb(0x0AB5FF),
            /* notificationBarText */ QRgb(0xFFFFFF)
        );

}

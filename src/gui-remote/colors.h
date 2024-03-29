/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COLORS_H
#define PMP_COLORS_H

#include <QColor>
#include <QVector>

namespace PMP
{
    class Colors
    {
    public:
        Colors(const QColor& widgetBorder,
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
               const QColor& spinnerLines);

        static const Colors& instance();

        const QColor widgetBorder;
        const QColor inactiveItemForeground;
        const QVector<QColor> itemBackgroundHighlightColors;
        const QColor specialQueueItemBackground;
        const QColor specialQueueItemForeground;
        const QColor historyErrorItemBackground;
        const QColor historyErrorItemForeground;
        const QColor trackProgressWidgetEmpty;
        const QColor trackProgressWidgetBackground;
        const QColor trackProgressWidgetBorder;
        const QColor trackProgressWidgetProgress;
        const QColor linkText;
        const QColor spinnerBackground;
        const QColor spinnerLines;

    private:
        static const Colors _lightScheme;
    };
}
#endif

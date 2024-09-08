/*
    Copyright (C) 2024, Kevin André <hyperquantum@gmail.com>

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

#include "searchutil.h"

namespace PMP
{
    QString SearchUtil::toSearchString(QString const& text)
    {
        const auto normalized = text.toLower().normalized(QString::NormalizationForm_KD);

        QString withoutAccents;
        for (QChar const& character : normalized)
        {
            auto category = character.category();

            if (category == QChar::Mark_NonSpacing
                || category == QChar::Mark_SpacingCombining
                || category == QChar::Mark_Enclosing)
            {
                continue; /* do not include this character */
            }

            withoutAccents.append(character);
        }

        return withoutAccents.simplified();
    }
}

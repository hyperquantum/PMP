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

#include "searchrank.h"

#include <QList>
#include <QMap>

namespace
{
    // Function to find all occurrences of a search word in a field
    QVector<int> findAllOccurrences(const QString& field, const QString& word)
    {
        QVector<int> positions;
        int pos = field.indexOf(word, 0, Qt::CaseInsensitive);
        while (pos != -1)
        {
            positions.append(pos);
            pos = field.indexOf(word, pos + 1, Qt::CaseInsensitive);
        }
        return positions;
    }

    int computeBestMatch(int wordIndex, const QStringList& words,
                         const QMap<int, QVector<int>>& allPositions, QVector<bool>& used,
                         int lastCharPosOfPrevMatch)
    {
        QString word = words[wordIndex];

        if (!allPositions.contains(wordIndex))
        {
            if (wordIndex >= words.size())
                return 0;

            return computeBestMatch(wordIndex + 1, words, allPositions, used,
                                    lastCharPosOfPrevMatch);
        }

        int bestScore = 0; // Start from zero instead of a negative score

        auto const& positions = allPositions[wordIndex];
        for (int pos : positions)
        {
            // Check if any character in the range is already used
            bool isOverlapping =
                wordIndex > 0 && std::any_of(used.begin() + pos,
                                             used.begin() + pos + word.length(),
                                             [](bool u) { return u; });
            if (isOverlapping) continue; // Skip overlapping matches

            // Compute proximity bonus using the last matched character position
            int proximityBonus = 0;
            if (lastCharPosOfPrevMatch >= 0 && pos > lastCharPosOfPrevMatch)
            {
                proximityBonus = std::max(0, 11 - (pos - lastCharPosOfPrevMatch));
            }

            int score = 3 + proximityBonus;

            // recurse if there are more words
            if (wordIndex < words.size() - 1)
            {
                // Mark this range as used
                for (int i = pos; i < pos + word.length(); ++i)
                {
                    used[i] = true;
                }

                score += computeBestMatch(wordIndex + 1, words, allPositions, used,
                                          pos + word.length() - 1);

                // Reset usage (backtracking)
                for (int i = pos; i < pos + word.length(); ++i)
                {
                    used[i] = false;
                }
            }

            bestScore = std::max(bestScore, score);
        }

        return bestScore;
    }

    // Function to score matches in a field
    int scoreFieldMatch(const QString& field, const QStringList& words)
    {
        if (words.isEmpty())
            return 0;

        QMap<int, QVector<int>> allPositions;
        for (int i = 0; i < words.size(); ++i)
        {
            allPositions[i] = findAllOccurrences(field, words[i]);
        }

        QVector<bool> used(field.length(), false);
        return computeBestMatch(0, words, allPositions, used, -1);
    }
}

namespace PMP::Client
{
    bool SearchRank::isBetterMatchThan(const SearchQuery& query, QString s1, QString s2)
    {
        return compareMatches(query, s1, s2) < 0;
    }

    int SearchRank::compareMatches(const SearchQuery& query, QString s1, QString s2)
    {
        auto score1 = getMatchScore(query, s1);
        auto score2 = getMatchScore(query, s2);

        if (score1 > score2)
            return -1;
        if (score2 > score1)
            return 1;

        return 0;
    }

    int SearchRank::getMatchScore(const SearchQuery& query, QString s)
    {
        return scoreFieldMatch(s, query._searchParts);
    }
}

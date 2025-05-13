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
    struct Occurrence
    {
        int position;
        bool atStartOfWord;
        bool isFullWord;
    };

    // Function to find all occurrences of a search word in a field
    QVector<Occurrence> findAllOccurrences(const QString& field, const QString& word)
    {
        QVector<Occurrence> occurrences;
        int pos = field.indexOf(word, 0, Qt::CaseInsensitive);
        while (pos >= 0)
        {
            bool atStartOfWord = pos == 0 || field[pos - 1] == ' ';
            bool atEndOfWord =
                pos == field.size() - word.size() || field[pos + word.size()] == ' ';

            Occurrence occurrence =
                {
                    .position = pos,
                    .atStartOfWord = atStartOfWord,
                    .isFullWord = atStartOfWord && atEndOfWord
                };

            occurrences.append(occurrence);

            pos = field.indexOf(word, pos + 1, Qt::CaseInsensitive);
        }

        return occurrences;
    }

    int computeBestMatch(int wordIndex, const QStringList& words,
                         const QMap<int, QVector<Occurrence>>& allOccurrences,
                         QVector<bool>& used, int lastCharPosOfPrevMatch)
    {
        QString word = words[wordIndex];

        if (!allOccurrences.contains(wordIndex))
        {
            if (wordIndex >= words.size())
                return 0;

            return computeBestMatch(wordIndex + 1, words, allOccurrences, used,
                                    lastCharPosOfPrevMatch);
        }

        int bestScore = 0;

        auto const& occurrences = allOccurrences[wordIndex];
        for (auto const& occurrence : occurrences)
        {
            int pos = occurrence.position;

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

            // full word bonus also includes start of word bonus
            int startOfWordBonus = occurrence.atStartOfWord ? 2 : 0;
            int fullWordBonus = occurrence.isFullWord ? 1 : 0;
            int score = 3 + proximityBonus + startOfWordBonus + fullWordBonus;

            // recurse if there are more words
            if (wordIndex < words.size() - 1)
            {
                // Mark this range as used
                for (int i = pos; i < pos + word.length(); ++i)
                {
                    used[i] = true;
                }

                score += computeBestMatch(wordIndex + 1, words, allOccurrences, used,
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
    int scoreFieldMatch(const QString& field, const PMP::Client::SearchQuery& query)
    {
        if (query.isEmpty())
            return 0;

        auto words = query.words();

        QMap<int, QVector<Occurrence>> allOccurrences;
        for (int i = 0; i < words.size(); ++i)
        {
            allOccurrences[i] = findAllOccurrences(field, words[i]);
        }

        QVector<bool> used(field.length(), false);
        return computeBestMatch(0, words, allOccurrences, used, -1);
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
        return scoreFieldMatch(s, query);
    }
}

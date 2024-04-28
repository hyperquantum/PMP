/*
    Copyright (C) 2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "passwordstrengthevaluator.h"

//#include <QtDebug>
#include <QVector>

namespace PMP
{
    namespace
    {
        struct BlockFeatures
        {
            bool hasLowercaseLetters { false };
            bool hasUppercaseLetters { false };
            bool hasDigits { false };
            bool hasSpecialCharacters { false };

            bool isComplete { false };
        };

        QVector<BlockFeatures> getBlocksForPassword(QString const& password)
        {
            QVector<BlockFeatures> blocks { BlockFeatures() };

            for (int i = 0; i < password.size(); ++i)
            {
                auto& currentBlock = blocks.last();

                QChar c = password[i];

                if (c.isDigit())
                {
                    currentBlock.hasDigits = true;
                }
                else if (c.isLetter())
                {
                    if (c.isLower())
                        currentBlock.hasLowercaseLetters = true;
                    else
                        currentBlock.hasUppercaseLetters = true;
                }
                else
                {
                    currentBlock.hasSpecialCharacters = true;
                }

                if ((i + 1) % 4 == 0) /* block complete? */
                {
                    currentBlock.isComplete = true;

                    blocks.append(BlockFeatures());
                }
            }

            return blocks;
        }

        int getBlockRating(BlockFeatures const& block)
        {
            int featuresBitSet = (block.hasLowercaseLetters ? 0b1000 : 0)
                                 + (block.hasUppercaseLetters ? 0b0100 : 0)
                                 + (block.hasDigits ? 0b0010 : 0)
                                 + (block.hasSpecialCharacters ? 0b0001 : 0);

            int score;
            switch (featuresBitSet)
            {
                /* only a single feature */
            case 0b1000: score = 5; break;
            case 0b0100: score = 5; break;
            case 0b0010: score = 5; break;
            case 0b0001: score = 6; break;

                /* two features */
            case 0b1100: score = 10; break;
            case 0b1010: score = 10; break;
            case 0b1001: score = 11; break;
            case 0b0110: score = 10; break;
            case 0b0101: score = 11; break;
            case 0b0011: score = 11; break;

                /* three features */
            case 0b1110: score = 15; break;
            case 0b1101: score = 16; break;
            case 0b1011: score = 16; break;
            case 0b0111: score = 16; break;

                /* four features */
            case 0b1111: score = 21; break;

                /* zero features / fallback */
            case 0b0000: default: score = 0; break;
            }

            if (!block.isComplete)
                score = (score - 1) / 2;

            return score;
        }

        int sumOfBlockRatings(QVector<BlockFeatures> const& blocks)
        {
            BlockFeatures wholePasswordFeatures;

            int sum = 0;
            for (auto& block : blocks)
            {
                wholePasswordFeatures.hasLowercaseLetters |= block.hasLowercaseLetters;
                wholePasswordFeatures.hasUppercaseLetters |= block.hasUppercaseLetters;
                wholePasswordFeatures.hasDigits |= block.hasDigits;
                wholePasswordFeatures.hasSpecialCharacters |= block.hasSpecialCharacters;

                sum += getBlockRating(block);
            }

            wholePasswordFeatures.isComplete = true;

            sum += getBlockRating(wholePasswordFeatures);

            return sum;
        }

        /*
        QDebug operator<<(QDebug debug, PasswordStrengthRating rating)
        {
            switch (rating)
            {
            case PasswordStrengthRating::TooWeak: debug << "too weak"; return debug;
            case PasswordStrengthRating::Acceptable: debug << "acceptable";return debug;
            case PasswordStrengthRating::Good: debug << "good"; return debug;
            case PasswordStrengthRating::VeryGood: debug << "very good"; return debug;
            case PasswordStrengthRating::Excellent: debug << "excellent"; return debug;
            }

            debug << int(rating);
            return debug;
        }
        */
    }

    PasswordStrengthRating PasswordStrengthEvaluator::getPasswordRating(
                                                                const QString& password)
    {
        int score = getPasswordScore(password);

        return convertScoreToRating(score);
    }

    int PasswordStrengthEvaluator::getPasswordScore(const QString& password)
    {
        int passwordCharCount = password.size();

        auto const blocks = getBlocksForPassword(password);
        int blockRatingsTotal = sumOfBlockRatings(blocks);

        int pointsToSubtract = pointsToSubtractForPatterns(password);

        int total = passwordCharCount + blockRatingsTotal - pointsToSubtract;

        /* ONLY FOR DEBUGGING!
        qDebug() << "score of" << password << ":"
                 << passwordCharCount << "+" << blockRatingsTotal
                 << "-" << pointsToSubtract << "=" << total
                 << "<--" << ratingForScore(total);
        */

        return total;
    }

    PasswordStrengthRating PasswordStrengthEvaluator::convertScoreToRating(int score)
    {
        if (score < 35)
            return PasswordStrengthRating::TooWeak; /* 0 to 34 */

        if (score < 47)
            return PasswordStrengthRating::Acceptable; /* 35 to 46 */

        if (score < 59)
            return PasswordStrengthRating::Good; /* 47 to 58 */

        if (score < 71)
            return PasswordStrengthRating::VeryGood; /* 59 to 70 */

        return PasswordStrengthRating::Excellent; /* 71 and up */
    }

    int PasswordStrengthEvaluator::pointsToSubtractForPatterns(const QString& password)
    {
        int pointsToSubtract = 0;

        bool inPattern = false;
        int currentPatternPoints = 0;

        for (int i = 2; i < password.size(); ++i)
        {
            QChar character1 = password[i - 2];
            int character1Numeric = character1.unicode();
            QChar character2 = password[i - 1];
            int character2Numeric = character2.unicode();
            QChar character3 = password[i];
            int character3Numberic = character3.unicode();

            int diff1 = character2Numeric - character1Numeric;
            int diff2 = character3Numberic - character2Numeric;

            /* punish patterns such as "eeeee", "123456", "98765", "ghijklm" etc... */
            if (diff1 == diff2)
            {
                if (inPattern)
                {
                    currentPatternPoints += 8;
                }
                else
                {
                    currentPatternPoints += 4;
                }

                inPattern = true;
            }
            else
            {
                pointsToSubtract += currentPatternPoints;

                inPattern = false;
                currentPatternPoints = 0;
            }
        }

        pointsToSubtract += currentPatternPoints;

        return pointsToSubtract;
    }
}

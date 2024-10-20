/*
    Copyright (C) 2016-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "util.h"

#include "common/unicodechars.h"
#include "common/version.h"

#include <QAtomicInt>
#include <QDateTime>
#include <QtDebug>
#include <QThread>

namespace PMP
{
    unsigned Util::getRandomSeed()
    {
        /* Because std::random_device seems to be not random at all on MINGW 4.8, we use
         * the system time and an incrementing counter instead. */

        static QAtomicInt counter;
        auto counterValue = unsigned(counter.fetchAndAddOrdered(1));

        unsigned clockValue = quint64(QDateTime::currentMSecsSinceEpoch()) & 0xFFFFFFFFu;

        //qDebug() << "counterValue:" << counterValue << "  clockValue:" << clockValue;

        auto result = counterValue ^ clockValue;
        qDebug() << "Util::getRandomSeed returning" << result;

        /* we want to avoid returning the same result twice by accident (if the clock
         * advance and the counter difference cancel out each other), so we sleep a
         * little bit */
        QThread::msleep(8);

        return result;
    }

    QString Util::secondsToHoursMinuteSecondsText(qint32 totalSeconds)
    {
        if (totalSeconds < 0) { return "?"; }

        int sec = totalSeconds % 60;
        int min = (totalSeconds / 60) % 60;
        int hrs = (totalSeconds / 60) / 60;

        return QString::number(hrs).rightJustified(2, '0')
                + ":" + QString::number(min).rightJustified(2, '0')
                + ":" + QString::number(sec).rightJustified(2, '0');
    }

    QString Util::millisecondsToShortDisplayTimeText(qint64 milliseconds)
    {
        QString prefix;
        if (milliseconds < 0)
        {
            milliseconds = -milliseconds;
            prefix = "-";
        }

        int partialSeconds = milliseconds % 1000;
        int totalSeconds = int(milliseconds / 1000);

        int sec = totalSeconds % 60;
        int totalMinutes = totalSeconds / 60;
        int min = totalMinutes % 60;
        int hrs = totalMinutes / 60;

        if (hrs != 0)
        {
            return prefix + QString::number(hrs).rightJustified(2, '0')
                    + ":" + QString::number(min).rightJustified(2, '0')
                    + ":" + QString::number(sec).rightJustified(2, '0')
                    + "." + QString::number(partialSeconds / 100);
        }

        return prefix + QString::number(min).rightJustified(2, '0')
                + ":" + QString::number(sec).rightJustified(2, '0')
                + "." + QString::number(partialSeconds / 100);
    }

    QString Util::millisecondsToLongDisplayTimeText(qint64 milliseconds)
    {
        QString prefix;
        if (milliseconds < 0)
        {
            milliseconds = -milliseconds;
            prefix = "-";
        }

        int partialSeconds = milliseconds % 1000;
        int totalSeconds = int(milliseconds / 1000);

        int sec = totalSeconds % 60;
        int totalMinutes = totalSeconds / 60;
        int min = totalMinutes % 60;
        int hrs = totalMinutes / 60;

        return prefix + QString::number(hrs).rightJustified(2, '0')
                + ":" + QString::number(min).rightJustified(2, '0')
                + ":" + QString::number(sec).rightJustified(2, '0')
                + "." + QString::number(partialSeconds).rightJustified(3, '0');
    }

    QString Util::undeterminedLongDisplayTimeText()
    {
        QString text = "--:--:--.---";

        /* make sure the string is just as wide as a string with actual digits */
        text.replace("-", UnicodeChars::figureDash);

        return text;
    }

    QString Util::getCountdownTimeText(qint64 millisecondsRemaining)
    {
        if (millisecondsRemaining < 0)
            millisecondsRemaining = 0;

        int totalSeconds = int(millisecondsRemaining / 1000);
        int seconds = totalSeconds % 60;

        int totalMinutes = totalSeconds / 60;
        int minutes = totalMinutes % 60;

        int hours = totalMinutes / 60;

        return QString::number(hours).rightJustified(2, '0')
                + ":" + QString::number(minutes).rightJustified(2, '0')
                + ":" + QString::number(seconds).rightJustified(2, '0');
    }

    int Util::getCountdownUpdateIntervalMs(qint64 millisecondsRemaining)
    {
        if (millisecondsRemaining < 0)
            millisecondsRemaining = -millisecondsRemaining;

        if (millisecondsRemaining < 10 * 1000)
            return 250;

        return 1000;
    }

    QString Util::getHowLongAgoText(SimpleDuration howLongAgo)
    {
        if (howLongAgo.amount() < 0)
        {
            qWarning() << "getHowLongAgoText received a negative number:"
                       << howLongAgo.amount()
                       << " (unit" << int(howLongAgo.unit()) << ")";
            return "";
        }

        if (howLongAgo.amount() == 0)
            return QString("just now");

        auto amount = howLongAgo.amount();

        switch (howLongAgo.unit())
        {
        case PMP::DurationUnit::Seconds:
            return amount == 1
                    ? QString("1 second ago")
                    : QString("%1 seconds ago").arg(amount);

        case PMP::DurationUnit::Minutes:
            return amount == 1
                    ? QString("1 minute ago")
                    : QString("%1 minutes ago").arg(amount);

        case PMP::DurationUnit::Hours:
            return amount == 1
                    ? QString("1 hour ago")
                    : QString("%1 hours ago").arg(amount);

        case PMP::DurationUnit::Days:
            return amount == 1
                    ? QString("1 day ago")
                    : QString("%1 days ago").arg(amount);

        case PMP::DurationUnit::Weeks:
            return amount == 1
                    ? QString("1 week ago")
                    : QString("%1 weeks ago").arg(amount);

        case PMP::DurationUnit::Months:
            return amount == 1
                    ? QString("1 month ago")
                    : QString("%1 months ago").arg(amount);

        case PMP::DurationUnit::Years:
            return amount == 1
                    ? QString("1 year ago")
                    : QString("%1 years ago").arg(amount);
        }

        qWarning() << "unknown/unhandled DurationUnit value:" << int(howLongAgo.unit());
        return "";
    }

    SimpleDuration Util::getHowLongAgoDuration(int secondsAgo)
    {
        if (secondsAgo < 0)
        {
            qWarning() << "getHowLongAgoDuration received a negative number:"
                       << secondsAgo;
            return {};
        }

        const int secondsPerMinute = 60;
        const int secondsPerHour = 60 * secondsPerMinute;
        const int secondsPerDay = 24 * secondsPerHour;
        const int secondsPerWeek = 7 * secondsPerDay;
        const int secondsPerMonth = 30 * secondsPerDay; // good enough here
        const int secondsPer3Months = 91 * secondsPerDay; // good enough here
        const int secondsPerYear = 365 * secondsPerDay; // good enough here
        const int secondsPer4Years = (366 + 3 * 365) * secondsPerDay; // good enough here

        if (secondsAgo == 0)
            return {};

        if (secondsAgo < secondsPerMinute)
            return SimpleDuration(secondsAgo, DurationUnit::Seconds);

        if (secondsAgo < secondsPerHour)
        {
            int minutes = secondsAgo / secondsPerMinute;
            return SimpleDuration(minutes, DurationUnit::Minutes);
        }

        if (secondsAgo < secondsPerDay)
        {
            int hours = secondsAgo / secondsPerHour;
            return SimpleDuration(hours, DurationUnit::Hours);
        }

        if (secondsAgo < secondsPerWeek)
        {
            int days = secondsAgo / secondsPerDay;
            return SimpleDuration(days, DurationUnit::Days);
        }

        if (secondsAgo < secondsPerMonth)
        {
            int weeks = secondsAgo / secondsPerWeek;
            return SimpleDuration(weeks, DurationUnit::Weeks);
        }

        if (secondsAgo < secondsPerYear)
        {
            int months;
            if (secondsAgo < 3 * secondsPerMonth)
            {
                months = secondsAgo / secondsPerMonth;
            }
            else
            {
                int threeMonths = secondsAgo / secondsPer3Months;
                int remainingMonths =
                        (secondsAgo - threeMonths * secondsPer3Months) / secondsPerMonth;
                months = 3 * threeMonths + qMin(remainingMonths, 2);
            }

            return SimpleDuration(months, DurationUnit::Months);
        }

        int years;
        if (secondsAgo < 4 * secondsPerYear)
        {
            years = secondsAgo / secondsPerYear;
        }
        else
        {
            int fourYears = secondsAgo / secondsPer4Years;
            int remainingYears =
                    (secondsAgo - fourYears * secondsPer4Years) / secondsPerYear;
            years = 4 * fourYears + qMin(remainingYears, 3);
        }

        return SimpleDuration(years, DurationUnit::Years);
    }

    SimpleDuration Util::getHowLongAgoDuration(QDateTime pastTime, QDateTime now)
    {
        if (pastTime > now)
        {
            qWarning() << "getHowLongAgoDuration: pastTime not in the past;"
                       << "pastTime:" << pastTime << " now:" << now;
            return {};
        }

        auto secondsAgo = pastTime.secsTo(now);

        return getHowLongAgoDuration(secondsAgo);
    }

    SimpleDuration Util::getHowLongAgoDuration(QDateTime pastTime)
    {
        return getHowLongAgoDuration(pastTime, QDateTime::currentDateTimeUtc());
    }

    int Util::getHowLongAgoUpdateIntervalMs(int secondsAgo)
    {
        const int milliseconds = 1;
        const int seconds = 1000 * milliseconds;

        if (secondsAgo < 0)
        {
            /* pretend it is a positive number and return the result for that value */
            secondsAgo = -secondsAgo;
        }

        if (secondsAgo < 60)
            return 250 * milliseconds;

        if (secondsAgo < 60 * 60)
            return 1 * seconds;

        return 60 * seconds;
    }

    TextAndUpdateInterval Util::getHowLongAgoInfo(int secondsAgo)
    {
        return TextAndUpdateInterval(getHowLongAgoText(getHowLongAgoDuration(secondsAgo)),
                                     getHowLongAgoUpdateIntervalMs(secondsAgo));
    }

    TextAndUpdateInterval Util::getHowLongAgoInfo(QDateTime pastTime, QDateTime now)
    {
        auto secondsAgo = pastTime.secsTo(now);

        return getHowLongAgoInfo(secondsAgo);
    }

    TextAndUpdateInterval Util::getHowLongAgoInfo(QDateTime pastTime)
    {
        return getHowLongAgoInfo(pastTime, QDateTime::currentDateTimeUtc());
    }

    QString Util::getCopyrightLine(bool mustBeAscii)
    {
        auto line = QString("Copyright %1 %2 %3");

        if (mustBeAscii)
        {
            line =
                line.arg("(C)",
                         PMP_COPYRIGHT_YEARS,
                         "Kevin Andre");
        }
        else
        {
            line =
                line.arg(UnicodeChars::copyright,
                         QString(PMP_COPYRIGHT_YEARS).replace('-', UnicodeChars::enDash),
                         QString("Kevin Andr") + UnicodeChars::eAcute);
        }

        return line;
    }

    QByteArray Util::generateZeroedMemory(int byteCount)
    {
        return QByteArray(byteCount, '\0');
    }
}

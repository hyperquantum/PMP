/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/logging.h"
#include "common/version.h"

#include "colors.h"
#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QtDebug>
#include <QTextStream>

using namespace PMP;

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Remote");
    QCoreApplication::setApplicationVersion(PMP_VERSION_DISPLAY);
    QCoreApplication::setOrganizationName(PMP_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(PMP_ORGANIZATION_DOMAIN);

    Logging::enableTextFileOnlyLogging();
    Logging::setFilenameTag("GR"); /* GR = GUI-Remote */
    Logging::cleanupOldLogfiles();
    /* TODO: do a log cleanup regularly, because the user might keep the client running
     *       for several days without closing it. */

    QFile styleSheetFile(":qdarkstyle/style.qss");
    if (styleSheetFile.exists())
    {
        if (styleSheetFile.open(QFile::ReadOnly | QFile::Text))
        {
            QTextStream ts(&styleSheetFile);
            app.setStyleSheet(ts.readAll());

            /* HACK: fix link color in QLabel by changing the palette */
            QPalette newPallette(app.palette());
            newPallette.setColor(QPalette::Link, Colors::instance().linkText);
            newPallette.setColor(QPalette::LinkVisited, Colors::instance().linkText);
            app.setPalette(newPallette);
        }
        else
        {
            qWarning() << "could not load qdarkstyle style sheet; open() failed";
        }
    }
    else
    {
        qWarning() << "could not load qdarkstyle style sheet; file not found";
    }

    MainWindow window;
    window.show();

    auto exitCode = app.exec();

    qDebug() << "Exiting with code" << exitCode;

    return exitCode;
}

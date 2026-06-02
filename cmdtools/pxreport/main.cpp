/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Tue Sep 04 19:42:15 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
    email                : ndoebelin@gmx.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QGuiApplication>
#include <QCommandLineParser>
#include "pxreport.h"

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QGuiApplication app(argc, argv);

    QGuiApplication::setApplicationName("PxReport 0.1");
    QGuiApplication::setOrganizationName("doebelin.org");
    //QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("Profex Report: Generate a HTML report from a BGMN refinement project.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption savFileOption(QStringList() << "s" << "savfile", "Control file (*.sav)", "savfile");
    QCommandLineOption lstFileOption(QStringList() << "l" << "lstfile", "List file (*.lst) (optional)", "lstfile");
    QCommandLineOption parFileOption(QStringList() << "p" << "parfile", "Parameter file (*.par) (optional)", "parfile");
    QCommandLineOption diaFileOption(QStringList() << "d" << "diafile", "Diagram file (*.dia) (optional)", "diafile");
    QCommandLineOption outFileOption(QStringList() << "o" << "htmlfile", "Report file (*.html)", "htmlfile");
    parser.addOption(savFileOption);
    parser.addOption(lstFileOption);
    parser.addOption(parFileOption);
    parser.addOption(diaFileOption);
    parser.addOption(outFileOption);

    parser.process(app);

    QString sfile = parser.value(savFileOption);
    QString lfile = parser.value(lstFileOption);
    QString pfile = parser.value(parFileOption);
    QString dfile = parser.value(diaFileOption);
    QString ofile = parser.value(outFileOption);

    PxReport(sfile, lfile, pfile, dfile, ofile);

    return 0;
}


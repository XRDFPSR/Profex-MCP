/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Tue Nov 15 19:42:15 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#include <QCoreApplication>
#include <QCommandLineParser>
#include "pxanytoxy.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("PxAnyToXy 0.2");
    QCoreApplication::setOrganizationName("doebelin.org");
    //QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("Profex AnyToXy");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inFormatOption(QStringList() << "i" << "input", "Input format", "informat");
    QCommandLineOption outFormatOption(QStringList() << "o" << "output", "Output format, default = ASCII_XY", "outformat");
    QCommandLineOption nScanOption(QStringList() << "n" << "number", "Scan number (1...n), default = 1", "scannumber");
    QCommandLineOption lstFormatsOption(QStringList() << "l" << "list", "List available formats");
    parser.addOption(inFormatOption);
    parser.addOption(outFormatOption);
    parser.addOption(nScanOption);
    parser.addOption(lstFormatsOption);
    parser.addPositionalArgument("scanfile", "Input scan file", "scanfile");

    parser.process(app);

    if (parser.isSet(lstFormatsOption)) {
        PxAnyToXy::listFormats();
        return 0;
    }

    QStringList sfiles = parser.positionalArguments();
    QString iformat = parser.value(inFormatOption);
    QString oformat = parser.value(outFormatOption);
    QString nscan = parser.value(nScanOption);

    int n = 0;

    if (oformat.isEmpty()) oformat = "ASCII_XY";
    if (!nscan.isEmpty()) n = nscan.toInt();

    int err = 0;

    for (int i = 0; i < sfiles.size(); ++i) {
        if (!PxAnyToXy::convert(sfiles.at(i), iformat, oformat, n)) ++err;
    }

    return err > 0 ? 1 : 0;
}


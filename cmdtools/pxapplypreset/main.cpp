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
#include "pxapplypreset.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("PxApplyPreset 0.2");
    QCoreApplication::setOrganizationName("doebelin.org");
    //QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("Profex ApplyPreset: Apply refinement presets to XRD data files.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption presetFileOption(QStringList() << "p" << "presetfile", "Preset file (*.pfp)", "presetfile");
    QCommandLineOption sampleIdOption(QStringList() << "i" << "sampleid", "Sample ID", "sampleid");
    QCommandLineOption forceOption(QStringList() << "f" << "force", "Force overwriting of existing files");
    QCommandLineOption listOption(QStringList() << "l" << "listdir", "List available presets in directory listdir", "listdir");
    parser.addOption(presetFileOption);
    parser.addOption(sampleIdOption);
    parser.addOption(forceOption);
    parser.addOption(listOption);
    parser.addPositionalArgument("scanfile", "Scan File (ASCII_XY Format)", "scanfile");

    parser.process(app);

    if (!parser.value(listOption).isEmpty()) {
        PxApplyPreset::listPresets(parser.value(listOption));
        return 0;
    }

    QStringList sfiles = parser.positionalArguments();
    QString pfile = parser.value(presetFileOption);
    QString id = parser.value(sampleIdOption);
    bool f = parser.isSet(forceOption);

    PxApplyPreset(sfiles.first(), pfile, id, f);

    return 0;
}


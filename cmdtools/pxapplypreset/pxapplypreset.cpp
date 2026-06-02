/***************************************************************************
                          pxapplypreset.cpp  -  description
                             -------------------
    begin                : Tue Nov 24 19:42:15 CEST 2022
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

#include "pxapplypreset.h"
#include "bgmnpresethandler.h"
#include "bgmnfileio.h"
#include "parser/bgmnsavparser.h"
#include <QTextStream>

PxApplyPreset::PxApplyPreset(const QString &scan, const QString &preset, const QString &sampleId, bool force)
{
    QTextStream cout(stdout);
    QFileInfo fiScan(scan);

    BgmnPresetHandler phandler;
    QDomDocument presetXml = phandler.parsePresetFile(preset);

    if (presetXml.elementsByTagName("device").size()) {
        QStringList missingFiles, wrongChecksum;
        bool doApply = phandler.runFileConsistencyChecks(preset, presetXml, missingFiles, wrongChecksum);

        if (missingFiles.size() > 0) {
            cout << QString("Aborted because the following files are missing: %1").arg(missingFiles.join(", ")) << Qt::endl;
            doApply = false;
        } else if (wrongChecksum.size() > 0) {
            cout << QString("The following files have a wrong checksum: %1").arg(wrongChecksum.join(", ")) << Qt::endl;

            if (force) {
                cout << QString("Preset application was forced by the user. Continuing...") << Qt::endl;
                doApply = true;
            } else {
                cout << QString("Preset application aborted.") << Qt::endl;
            }
        }

        if (doApply) {
            int nThreads = QThread::idealThreadCount();
            QString controlFile = fiScan.absolutePath() + QDir::separator() + fiScan.completeBaseName() + ".sav";
            QString cFileContent = phandler.createProject(preset, controlFile, presetXml, sampleId, nThreads);
            BgmnFileIO::writeTextFile(controlFile, adjustOutputFiles(controlFile, cFileContent, scan, sampleId));
        }
    }
}

QString PxApplyPreset::adjustOutputFiles(const QString &controlFile, const QString &content, const QString &scanFile, const QString &sampleId)
{
    QFileInfo fiCtrl(controlFile);
    QFileInfo fiScan(scanFile);
    BgmnSavParser sparser(content, controlFile);

    sparser.setDiagramFile(QString("%1.dia").arg(fiCtrl.completeBaseName()));
    sparser.setOutputFile(QString("%1.par").arg(fiCtrl.completeBaseName()));
    sparser.setListFile(QString("%1.lst").arg(fiCtrl.completeBaseName()));
    sparser.setStrucOutBaseName(fiCtrl.completeBaseName());
    sparser.setSampleId(sampleId);
    sparser.setValFile(QStringList(fiScan.fileName()));

    return sparser.getContent();
}

void PxApplyPreset::listPresets(const QString &d)
{
    QTextStream cout(stdout);
    cout << QString("Preset file               Preset name (version)") << Qt::endl;
    cout << QString("-----------------------------------------------") << Qt::endl;

    QMap<QString, QString> presets = BgmnPresetHandler::getPresets(QStringList(d), "r");

    QMapIterator<QString, QString> it(presets);

    while (it.hasNext()) {
        it.next();
        QFileInfo fi(it.value());
        cout << QString("%1 %2").arg(fi.fileName(), -25).arg(it.key()) << Qt::endl;
    }
}

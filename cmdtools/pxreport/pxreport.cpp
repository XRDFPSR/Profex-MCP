/***************************************************************************
                          pxreport.cpp  -  description
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

#include "pxreport.h"
#include "../libXrdIO/parser/bgmnhtmlreportgenerator.h"
#include "../libXrdIO/import/bgmndiaimport.h"
#include "../libXrdIO/graphtosvg.h"
#include "bgmnfileio.h"
#include <QTextStream>
#include <QSettings>
#include <QFont>

PxReport::PxReport(const QString &savFile, const QString &lstFile, const QString &parFile, const QString &diaFile, const QString &outFile)
{
    QTextStream cout(stdout);
    QFileInfo fiSav(savFile);

    if (!fiSav.exists())     {
        cout << QString("SAV file not found: %1").arg(fiSav.absoluteFilePath()) << Qt::endl;
        return;
    }

    BgmnSavParser sparser(fiSav.absoluteFilePath());

    QFileInfo fiLst(lstFile.isEmpty() ? fiSav.absolutePath() + QDir::separator() + sparser.listFile() : lstFile);
    QFileInfo fiPar(parFile.isEmpty() ? fiSav.absolutePath() + QDir::separator() + sparser.outputFile() : parFile);
    QFileInfo fiDia(diaFile.isEmpty() ? fiSav.absolutePath() + QDir::separator() + sparser.diagramFile() : diaFile);

    int errors = 0;

    if (!fiLst.exists()) {
        cout << QString("LST file not found: %1").arg(fiLst.absoluteFilePath()) << Qt::endl;
        ++errors;
    }

    if (!fiPar.exists()) {
        cout << QString("PAR file not found: %1").arg(fiPar.absoluteFilePath()) << Qt::endl;
        ++errors;
    }

    if (!fiDia.exists()) {
        cout << QString("DIA file not found: %1").arg(fiDia.absoluteFilePath()) << Qt::endl;
        ++errors;
    }

    if (outFile.isEmpty()) {
        cout << QString("No HTML output file specified.") << Qt::endl;
        ++errors;
    }

    if (errors) return;

    double eps1 = sparser.getEpsN(1);
    double eps2 = sparser.getEpsN(2);
    double eps3 = sparser.getEpsN(3);

    QSettings settings("doebelin.org", "Profex5");

    bool ok;
    BgmnLstParser lparser(fiLst.absoluteFilePath(), ok);
    BgmnHtmlReportGenerator reportGen(lparser);
    reportGen.setDocumentStructure(settings.value("bgmnProject/report/documentStructure", BgmnFileIO::readTextFile(":/resources/report-structure.xml")).toString());

    QMap<QString, QVariant> projectData;
    projectData.insert("rawFileName", fiDia.absoluteFilePath());
    projectData.insert("workingDir", fiLst.absolutePath());
    projectData.insert("sampleId", sparser.sampleId());
    projectData.insert("geqFile", sparser.deviceFile());
    // projectData.insert("wavelength", graphView->getWaveLength());
    projectData.insert("wavelengthFile", sparser.getLambda());
    projectData.insert("globalGoalsIncludes", QVariant(getGlobalIncludeList()));
    projectData.insert("localGoalsIncludes", QVariant(getLocalIncludeList()));
    projectData.insert("quantGoals100percent", QVariant(settings.value("bgmnProject/quantGoals100percent", QVariant(false))));
    projectData.insert("parFileName", fiPar.absoluteFilePath());
    projectData.insert("chemistryData", getChemistryHtmlTable(sparser, lparser, reportGen.chemistryTableMode()));

    projectData.insert("diffpatternSvg", getSvg(fiDia.absoluteFilePath(), reportGen.patternAspectRatio(), eps1, eps2, eps3));

    QFont reportFont;
    reportFont.setFamily("Sans Serif");
    reportFont.setPointSize(10);

    reportGen.setProjectData(projectData);

    reportGen.generateDocument();
    BgmnFileIO::writeTextFile(outFile, reportGen.getContents());
}

QByteArray PxReport::getSvg(const QString &dia, double aspectRatio, double eps1, double eps2, double eps3)
{
    QVector<Scan> scanHeap;

    BgmnDiaImport diaImport;
    int n = diaImport.load(dia, scanHeap, true);

    if (n > 0) {
        GraphToSvg svgGen(scanHeap, eps1, eps2, eps3);
        return svgGen.getSvg(aspectRatio);
    }

    return QByteArray();
}

QStringList PxReport::getGlobalIncludeList()
{
    QSettings settings("doebelin.org", "Profex5");
    QString globalGoals = settings.value("bgmnProject/reportedGlobalGoals", global::defaultBgmnGlobalGoals).toString();
    if (globalGoals.isEmpty()) globalGoals = global::defaultBgmnGlobalGoals;
    return globalGoals.isEmpty() ? QStringList(".*") : globalGoals.split("\n");
}

QStringList PxReport::getLocalIncludeList()
{
    QSettings settings("doebelin.org", "Profex5");
    QString localGoals = settings.value("bgmnProject/reportedLocalParameters", global::defaultBgmnLocalGoals).toString();
    if (localGoals.isEmpty()) localGoals = global::defaultBgmnLocalGoals;
    return localGoals.isEmpty() ? global::defaultBgmnLocalGoals.split("\n") : localGoals.split("\n");
}

QString PxReport::getChemistryHtmlTable(BgmnSavParser &sparser, BgmnLstParser &lparser, ChemistryMode mode)
{
    ChemTableData data;
    data.setData(sparser, lparser, true);
    return data.getHtmlTable(mode);
}

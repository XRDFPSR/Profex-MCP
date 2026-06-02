/***************************************************************************
                          bgmnreporthtmlgenerator.h  -  description
                             -------------------
    begin                : Jun 28 18:55:07 CET 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#include <QString>
#include <QMap>
#include <QDomDocument>
#include "bgmnlstparser.h"
#include "chemtablestruct.h"

#ifndef BGMNREPORTHTMLGENERATOR_H
#define BGMNREPORTHTMLGENERATOR_H

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnHtmlReportGenerator
{
public:
    BgmnHtmlReportGenerator();
    BgmnHtmlReportGenerator(const QString &);
    BgmnHtmlReportGenerator(const BgmnLstParser &);
    inline void setFormat(const QString &s) {format = s;}
    inline void setFileName(const QString &s) {fileName = s;}
    void setDocumentStructure(const QString &);
    void setProjectData(const QMap<QString, QVariant> &);
    void setLstFile(const QString &);
    void generateDocument();

    double patternAspectRatio();
    ChemistryMode chemistryTableMode();
    QString getContents();

private:
    SettingsManager *settings;
    BgmnLstParser lparser;
    QString contents;
    QMap<QString, QVariant> projectData;
    QDomDocument docStructure;
    QString headerPixmap;
    QString format;
    QString fileName;
    bool skipErrors;

    QString htmlHeader();
    QString headerLogo();
    QString sampleInfoTable(const QDomElement &);
    QString globalGoalsTable(const QDomElement &);
    QString diffPatternSection(const QDomElement &);
    QString localGoalsTable(const QDomElement &);
    QString getChemistryTable(const QDomElement &);
    QString getHklListTable(const QDomElement &);
    QString composeStyleSheed();

    int digits(double);
};

#endif // BGMNREPORTHTMLGENERATOR_H

/***************************************************************************
                          bgmnpresethandler.h  -  description
                             -------------------
    begin                : Sat Jan 07 10:30:00 CEST 2017
    copyright            : (C) 2017 by Nicola Doebelin
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

#ifndef BGMNPRESETHANDLER_H
#define BGMNPRESETHANDLER_H

#include <QString>
#include <QByteArray>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDomElement>
#include <QDomDocument>
#include "structs.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

using namespace global;

class XRDIO_EXPORT BgmnPresetHandler
{
public:
    BgmnPresetHandler();

    /* return presets in directories
     *   type = a: all presets
     *   type = b: baseline presets
     *   type = r: refinement presets
     *   type = c: curve fit presets
     *   type = i: peak integral presets
     *   type = e: excel workbook presets
     */
    static QMap<QString, QString> getPresets(const QStringList &dirs, const QString &type = "a");

    QDomDocument createPresetXmlDocument(const QString &presetFileName);

    QDomDocument createPreset(const QString &cfile,
                      const QString &pfile,
                      const QString &cfileContent,
                      QDomDocument &doc,
                      bool writeFiles);

    QDomDocument parsePresetFile(const QString &);
    bool runFileConsistencyChecks(const QString &pfile, const QDomDocument &doc, QStringList &, QStringList &);
    QString createProject(const QString &pfile, const QString &cfile, const QDomDocument &doc, const QString &id, int nth);

private:
    struct PresetFileInfo {
        QString name;
        QString version;
        bool hasBaseLine;
        bool hasRefinement;
        bool hasCurveFit;
        bool hasPeakIntegrals;
        bool hasExcelExport;

        PresetFileInfo():
            name(QString()),
            version(QString()),
            hasBaseLine(false),
            hasRefinement(false),
            hasCurveFit(false),
            hasPeakIntegrals(false),
            hasExcelExport(false) {}

        PresetFileInfo(QString n, QString v):
            name(n),
            version(v),
            hasBaseLine(false),
            hasRefinement(false),
            hasCurveFit(false),
            hasPeakIntegrals(false),
            hasExcelExport(false) {}
    };

    QStringList allTags;
    QStringList fileTags;

    static PresetFileInfo getPresetFileInfo(const QString &);
    bool createPresetDir(const QString &);
    QList<BgmnPresetFile> stringsToFileList(const QStringList &, const QFileInfo &);
    BgmnPresetFile writeTemplateFile(const QFileInfo &, const QString &);
    QList<BgmnPresetFile> copyFilesToTemplate(const QList<BgmnPresetFile> &, const QString &);
    FileType getFileType(const QFileInfo &, const QFileInfo &);
    QStringList allDevFiles(const QString &, const QFileInfo &, bool);

    void createPresetXml(const QList<BgmnPresetFile> &, QDomDocument &);

    bool writePresetFile(const QDomDocument &, const QFileInfo &);
    QList<BgmnPresetFile> getFilesOfType(const QList<BgmnPresetFile> &, FileType);
    QList<BgmnPresetFile> domElementsToFiles(const QDomDocument &);
    QList<BgmnPresetFile> toAbsolutePath(const QList<BgmnPresetFile> &, const QString &);
    QStringList checkExisting(const QList<BgmnPresetFile> &);
    QStringList checkMD5sum(const QList<BgmnPresetFile> &);
    QStringList checkOverwrite(const QList<BgmnPresetFile> &, const QString &);
    bool fileChecks(const QList<BgmnPresetFile> &, QStringList &, QStringList &);
    void copyFilesToDestination(const QList<BgmnPresetFile> &, const QString &, const QList<FileType> &);
    QString toDestinationFilePath(const BgmnPresetFile &, const QString &);
};

#endif // BGMNPRESETHANDLER_H

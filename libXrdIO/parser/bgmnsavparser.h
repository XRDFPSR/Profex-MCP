/***************************************************************************
                          savparser.h  -  description
                             -------------------
    begin                : Fri Feb 03 11:00:00 CEST 2012
    copyright            : (C) 2012 by Nicola Doebelin
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

#ifndef SAVPARSER_H
#define SAVPARSER_H

#include <QString>
#include <QStringList>
#include "../settingsmanager.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnSavParser
{
public:
    explicit BgmnSavParser(const QString &str, const QString &name);
    explicit BgmnSavParser(const QString &file, bool * ok = nullptr);

    bool loadFile(const QString &);
    void setContent(const QString &);
    void addStructureFiles(const QStringList &, bool goals = true, bool multi = false);
    int removeStructureFile(const QString &, const QString &, bool goals = true);
    bool saveFile();

    void setSampleId(const QString &);
    void setPresetName(const QString &);
    void setDeviceFile(const QString &);
    void setValFile(const QStringList &);
    void setOutputFile(const QString &);
    void setListFile(const QString &);
    void setDiagramFile(const QString &);
    void setUntFile(const QString &);
    void setUntcFile(const QString &);
    void setTubeTailsFile(const QString &);
    void setStrucOutBaseName(const QString &);
    void setNumberOfThreads(int);
    void setInternalStandard(const QString &, double);
    void unsetInternalStandard();
    void renumberGoals();
    int clearValFiles();
    int generateValFiles(const QString &, const QStringList &, int);
    void clearStrucOutFiles();
    void generateStrucOutFiles(const QString &);
    void overrideStrucFileNames(const QStringList &);
    void writeGoals();

    void addAmorphous(const QString &);

    inline QString controlFile() const {return fileName;}
    QString deviceFile() const;
    QStringList valFile() const;
    QString outputFile() const;
    QString listFile() const;
    QString diagramFile() const;
    QString untFile() const;
    QString untcFile() const;
    QString tubeTailsFile() const;
    QString internalStandard() const;
    QString sampleId() const;
    QString getLambda() const;
    double internalStandardQuantity() const;
    double limitOfDetection() const;
    double limitOfQuantification() const;
    double minimumEsd() const;
    double getWmin() const;
    double getWmax() const;
    double getSynchrotron() const;
    double getNeutron() const;
    int numberOfThreads() const;
    double getEpsN(int) const;
    bool hasEpsN(int) const;
    bool hasTubeTails() const;

    QString getContent() const;
    QStringList getStruc(const QString &dir = QString()) const;
    int hasPhase(const QString &) const;
    QString createTemplate() const;
    QStringList getStrucOut(const QString &dir = QString()) const;
    QStringList getSimpleStrucOut(const QString &dir = QString()) const;
    QStringList getResOut(const QString &dir = QString()) const;
    QStringList getFcfOut(const QString &dir = QString()) const;
    QStringList getPdbOut(const QString &dir = QString()) const;
    QStringList getGoals() const;
    QHash<QString, QString> getAllStrucQuantGoals(const QString &dir = QString()) const;
    QHash<QString, QString> getAllStrucPhaseNames(const QString &dir = QString()) const;

private:
    SettingsManager *settings;
    QString fileName;
    QString rxFileNames;
    QStringList content;
    QString tubeTailsFileName;
    QString untFileName;
    QString untcFileName;

    QString qprefixRel;
    QString sumvarRel;
    QString qprefixAbs;
    QString sumvarAbs;

    QStringList getAllPhaseNames() const;
    QString getParameterValue(const QRegularExpression &, int) const;
    QStringList getAllMatches(const QRegularExpression &, int m = 1) const;
    QStringList getNonQuantificationGoals() const;
    int removeAllLines(const QRegularExpression &);
    void removeSumsAndGoals();
    int createQuantGoalRel(const QStringList &, QStringList &);
    int createQuantGoalAbs(const QStringList &, const QString &, bool, QStringList &);
    int clearPhases();

    QStringList addDirToFiles(const QStringList &, const QString &) const;
};

#endif // SAVPARSER_H

/***************************************************************************
                          importhandler.h  -  description
                             -------------------
    begin                : Thu June 18, 2013
    copyright            : (C) 2013 by Nicola Doebelin
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

#ifndef IMPORTHANDLER_H
#define IMPORTHANDLER_H

#include "genericimport.h"
#include "../settingsmanager.h"
#include "../scan.h"
#include "../hkl.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT ImportHandler
{
public:
    explicit ImportHandler();
    ~ImportHandler();

    int load(const QString &f, const QString &uid, QVector<Scan> &scanHeap, bool minimal = false);

    int saveAs(const QString &in, const QString &uid_in, const QString &out, const QString &uid_out, int snumber = -1, const QString &fsep = QString());

    GenericImport * isSupported(const QString &f, const QString &uid);

    // returns information about import filters
    QStringList filters();
    QStringList rawFilters();
    QStringList projectFilters();
    QStringList extensions();
    QStringList descriptions();
    QStringList uniqueIds();

    // returns a map with all importers
    const QMap<QString, GenericImport *> &importers() {return formats;}

    // returns the currently loaded file's uid
    QString fileUid() {return currentUid;}

    // tries to find a uid based on the file extension
    QString uidByFileName(const QString &);
    QString uidByFilter(const QString &);

    void getExclRegs(const QString &, QVector<double> &);
    void getExclRegs(QVector<double> &);
    void getReflections(const QString &, QVector<Scan> &);
    void getReflections(QVector<Scan> &);


    int getNumberOfPhases();
    
private:
    QMap<QString, GenericImport *> formats;
    QMap<QString, int> headerSize;
    QString file;
    SettingsManager *settings;
    QString currentUid;
    QStringList rawFilterList;
    QStringList projectFilterList;
    QStringList nonMergeUids;

    void sumScans(QVector<Scan> &);
    void averageScans(QVector<Scan> &);
    QByteArray headerOfFile(const QString &, const QString &);
};

#endif // IMPORTHANDLER_H

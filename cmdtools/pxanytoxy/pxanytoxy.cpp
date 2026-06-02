/***************************************************************************
                          pxanytoxy.cpp  -  description
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

#include "pxanytoxy.h"
#include "import/importhandler.h"
#include "export/exporthandler.h"
#include <QFileInfo>
#include <QTextStream>

PxAnyToXy::PxAnyToXy()
{
}

QStringList PxAnyToXy::getUniqueIds()
{
    ImportHandler ihandler;
    return ihandler.uniqueIds();
}

bool PxAnyToXy::convert(const QString &file, const QString &iformat, const QString &oformat, int nscan)
{
    ImportHandler ihandler;
    ExportHandler ehandler;

    if (!ihandler.isSupported(file, iformat)) return false;
    if (!ehandler.isSupported(oformat)) return false;

    QFileInfo fi(file);
    QFileInfo fo(fi.absolutePath() + QDir::separator() + fi.completeBaseName() + "." + ehandler.extensionByUid(oformat));
    QTextStream cout(stdout);

    int n = ihandler.saveAs(fi.absoluteFilePath(), iformat, fo.absoluteFilePath(), oformat, nscan - 1, QString(" "));
    if (n <= 0) {
        cout << QString("No scans read from file %1").arg(fi.absoluteFilePath()) << Qt::endl;
        return false;
    }

    cout << QString("Scan no. %1 written to file %2").arg(fi.absoluteFilePath(), fo.absoluteFilePath()) << Qt::endl;
    return true;
}

void PxAnyToXy::listFormats()
{
    QTextStream cout(stdout);

    ImportHandler ihandler;
    ExportHandler ehandler;

    QStringList iFormats = ihandler.uniqueIds();
    QStringList iDescr   = ihandler.descriptions();

    cout << QString("Supported input formats:") << Qt::endl;
     for (int i = 0; i < qMin(iFormats.size(), iDescr.size()); ++i) {
        cout << QString("%1 %2").arg(iFormats.at(i), -20).arg(iDescr.at(i)) << Qt::endl;
    }

    cout << Qt::endl;

    QStringList eFormats = ehandler.uniqueIds();
    QStringList eDescr   = ehandler.descriptions();

    cout << QString("Supported output formats:") << Qt::endl;
    for (int i = 0; i < qMin(eFormats.size(), eDescr.size()); ++i) {
        cout << QString("%1 %2").arg(eFormats.at(i), -20).arg(eDescr.at(i)) << Qt::endl;
    }
}

/***************************************************************************
                          exporthandler.cpp  -  description
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

#include "exporthandler.h"
#include "fullprofdatexport.h"
#include "textureplusexport.h"
#include "philipsudfexport.h"
#include "asciixyexport.h"
#include "asciitxtexport.h"
#include "asciihklexport.h"
#include "gnuplotexport.h"
#include "graceexport.h"
#include "pdcifexport.h"
#include "gsasstdexport.h"
// #include "profexpgxexport.h"

ExportHandler::ExportHandler()
{
    FullprofDatExport *fpDatExport = new FullprofDatExport();
    PhilipsUdfExport *phUdfExport  = new PhilipsUdfExport();
    TexturePlusExport *tPlusExport = new TexturePlusExport();
    AsciiXyExport *asciiExport     = new AsciiXyExport();
    AsciiTxtExport *asciiTxtExport = new AsciiTxtExport();
    AsciiHklExport *asciiHklExport = new AsciiHklExport();
    GnuPlotExport *gpExport        = new GnuPlotExport();
    GraceExport *graceExport       = new GraceExport();
    PdCifExport *pdCifExport       = new PdCifExport();
    GsasStdExport *gsasStdExport   = new GsasStdExport();
    // ProfexPgxExport *pgxExport     = new ProfexPgxExport();

    formats.insert(fpDatExport->uniqueId(),    fpDatExport);
    formats.insert(phUdfExport->uniqueId(),    phUdfExport);
    formats.insert(tPlusExport->uniqueId(),    tPlusExport);
    formats.insert(asciiExport->uniqueId(),    asciiExport);
    formats.insert(asciiTxtExport->uniqueId(), asciiTxtExport);
    formats.insert(asciiHklExport->uniqueId(), asciiHklExport);
    formats.insert(gpExport->uniqueId(),       gpExport);
    formats.insert(graceExport->uniqueId(),    graceExport);
    formats.insert(pdCifExport->uniqueId(),    pdCifExport);
    formats.insert(gsasStdExport->uniqueId(),  gsasStdExport);
    // formats.insert(pgxExport->uniqueId(),      pgxExport);
}

ExportHandler::~ExportHandler()
{
    formats.clear();
}

QMap<QString, QString> ExportHandler::uidsByFilter()
{
    QMap<QString, QString> out;

    QMap<QString, GenericExport *>::const_iterator it = formats.constBegin();
    while (it != formats.constEnd()) {
        out[it.value()->filter()] = it.value()->uniqueId();
        ++it;
    }

    return out;
}

int ExportHandler::save(const QString &id, const QString &f, const Scan &scan, const QMap<QString, QVariant> &flags)
{
    if (!formats.contains(id)) {
        qDebug() << QString("ExportHandler::save(): Unknown file format ID: %1").arg(id);
        return -1;
    }

    return formats[id]->save(f, scan, flags);
}

int ExportHandler::save(const QString &id, const QString &f, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags)
{
    if (!formats.contains(id)) {
        qDebug() << QString("ExportHandler::save(): Unknown file format ID: %1").arg(id);
        return -1;
    }

    return formats[id]->save(f, scanHeap, flags);
}

bool ExportHandler::hasMultiScanSupport(const QString &id)
{
    return formats.contains(id) ? formats[id]->hasMultiScanSupport() : false;
}

GenericExport * ExportHandler::exporter(const QString &id, bool *ok)
{
    if (!formats.contains(id)) {
        *ok = false;
        return 0;
    }

    *ok = true;
    return static_cast<GenericExport *>(formats[id]);
}

QString ExportHandler::extensionByFilter(const QString &f)
{
    QMap<QString, QString> uids = uidsByFilter();

    if (!uids.contains(f)) {
        return QString();
    }

    return formats.contains(uids[f]) ? formats[uids[f]]->extension() : QString();
}

QString ExportHandler::extensionByUid(const QString &u)
{
    return formats.contains(u) ? formats[u]->extension() : QString();
}

GenericExport * ExportHandler::isSupported(const QString &uid)
{
    return formats.contains(uid) ? formats.value(uid) : nullptr;
}

QStringList ExportHandler::uniqueIds()
{
    return formats.keys();
}

QStringList ExportHandler::descriptions()
{
    QStringList out;

    QMap<QString, GenericExport *>::const_iterator it = formats.constBegin();
    while (it != formats.constEnd()) {
        out.append(it.value()->description());
        ++it;
    }

    return out;
}

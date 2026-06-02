/***************************************************************************
                          pdcifexport.h  -  description
                             -------------------
    begin                : Mon Jul 04 14:06:07 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#ifndef PDCIFEXPORT_H
#define PDCIFEXPORT_H

#include "genericexport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT PdCifExport : public GenericExport
{
public:
    PdCifExport(QObject * = 0);

    inline QString filter() {return "Powder CIF file (*.cif *.CIF)";}
    inline QString extension() {return "cif";}
    inline QString description() {return "Powder CIF file containing powder diffraction raw data";}

    // these are the virtual functions required by genericexport
    int save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());

    inline bool hasMultiScanSupport() {return true;}

private:
    int saveSingle(const QString &file, const Scan &scan, const QString &difId);
    int saveRietveldSet(const QString &file, const QVector<Scan> &scanHeap, const QString &difId);

};

#endif // PDCIFEXPORT_H

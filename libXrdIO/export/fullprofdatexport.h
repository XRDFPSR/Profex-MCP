/***************************************************************************
                          datexport.h  -  description
                             -------------------
    begin                : Mon Jan 16, 2009
    copyright            : (C) 2009 by Nicola Doebelin
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

#ifndef FULLPROFDATEXPORT_H
#define FULLPROFDATEXPORT_H

#include "genericexport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT FullprofDatExport : public GenericExport
{
    Q_OBJECT

    public:
    FullprofDatExport(QObject * = 0);

    QString filter() {return "Fullprof DAT version 10 format (*.dat *.DAT)";}
    QString extension() {return "dat";}
    QString description() {return "Fullprof version 10";}

    int save(const QString &, const Scan &, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &, const QVector<Scan> &, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int saveSum(const QString &, const QVector<Scan> &);

    bool hasMultiScanSupport() {return false;}

    signals:
	void maxSteps(int);
	void progress(int);
};
#endif

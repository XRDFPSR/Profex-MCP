/***************************************************************************
                          gnuplotexport.h  -  description
                             -------------------
    begin                : Tue Apr 16 20:16:07 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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


#ifndef GNUPLOTEXPORT_H
#define GNUPLOTEXPORT_H

#include "genericexport.h"
#include "../settingsmanager.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT GnuPlotExport : public GenericExport
{
public:
    GnuPlotExport(QObject *parent = 0);

    QString filter() {return "GNUPlot (*.gpl *.GPL)";}
    QString extension() {return "gpl";}
    QString description() {return "GNUPlot Script";}

    // these are the virtual functions required by genericexport
    int save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());

    bool hasMultiScanSupport() {return true;}

private:
    SettingsManager *settings;

    int getBackgroundScanIndex(const QVector<Scan> &scanHeap);
    QString getFileHeaderString(const QVector<Scan> &);
    QString getHklTickInlineString(const Scan &, double _xStart, double _xEnd, int i);
    QString getScanDataString(const Scan &s, int scale);
    QString getHklDataString(const Scan &s, int scale);
    QString getScanPlotLine(const Scan &scan, const QString &symbol, double offset, int i);
    QString getHklPlotLine(const Scan &scan, double offset, int i);
};

#endif // GNUPLOTEXPORT_H

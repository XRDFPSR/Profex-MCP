/***************************************************************************
                          graceexport.h  -  description
                             -------------------
    begin                : Tue Feb 18 20:28:07 CEST 2016
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


#ifndef GRACEEXPORT_H
#define GRACEEXPORT_H

#include "genericexport.h"
#include "../settingsmanager.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT GraceExport : public GenericExport
{
public:
    GraceExport(QObject *parent = nullptr);

    QString filter() {return "Grace Plot (*.agr *.AGR)";}
    QString extension() {return "agr";}
    QString description() {return "Grace Plot File";}

    // these are the virtual functions required by genericexport
    int save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());

    bool hasMultiScanSupport() {return true;}

private:
    SettingsManager *settings;
    QList<QColor> _colors;
    QList<QColor> _fillColors;
    QStringList _names;
    int _hklSymbol;
    bool _fillActiveScans;
    bool _activeScansBold;
    double _xmax;
    double _xmin;
    double _ymax;
    double _ymin;

    void parseScanRanges(const QVector<Scan> &scanHeap);
    QString getFileHeaderString(const QVector<Scan> &scanHeap);
    QString getScanHeaderString(const Scan &, int currentScanNumber, int totalScanNumber);
    QString getHklTickHeaderString(const Scan &, int, int);
    QString getHklScanHeaderString(const Scan &, int);
    QString getScanDataString(const Scan &, int);
    QString getHklTickDataString(const Scan &, int setNumber, int phaseNumber);
    QString getHklScanDataString(const Scan &, int);
    double getMajorTickSpacing(double);
};

#endif // GRACEEXPORT_H

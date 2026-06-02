/***************************************************************************
                          thermotxlimport.h  -  description
                             -------------------
    begin                : Wed Nov 15 14:37:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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



#ifndef THERMOTXLIMPORT_H
#define THERMOTXLIMPORT_H

#include <QDomNode>
#include <QVariantHash>
#include "genericimport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT ThermoTxlImport : public GenericImport
{
public:
    ThermoTxlImport(QObject * = 0);

    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    QString uniqueId() {return "THERMO_TXL";}

private:
    QString _globalFileName;
    QString _globalOrigFileName;
    QString _globalOperatorName;
    double  _globalWaveLength;

    void parseGlobalHeader(const QDomElement &);
    void parseGlobalOrigFileName(const QDomElement &);
    void parseGlobalOperatorName(const QDomElement &);
    void parseGlobalWaveLength(const QDomElement &);

    bool parseScan(const QDomElement &, Scan &, int);
    bool parseScanHeader(const QDomElement &, Scan &);
    bool parseScanData(const QDomElement &, Scan &);
    bool parseScanPeakData(const QDomElement &, Scan &);
    QString parseScanId(const QDomElement &);

    bool xmlToDouble(const QString &, double &);
    bool xmlToDoubleMapped(const QString &, double &);
    void checkScanId(Scan &, const QString &fn, int n);
};

#endif // THERMOTXLIMPORT_H

/***************************************************************************
                          fullprofprfimport.h  -  description
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

#ifndef FULLPROFPRFIMPORT_H
#define FULLPROFPRFIMPORT_H

#include "genericimport.h"
#include "../hkl.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT FullprofPrfImport : public GenericImport
{

public:
    FullprofPrfImport(QObject * = 0);
        ~FullprofPrfImport();

    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    void getExclRegs(QVector<double> &);
    void getReflections(QVector<Hkl> &);
	int getNumberOfPhases();
    QString uniqueId() {return "FPPRF3_PRF";}

private:
    QList<QByteArray> _contents;

    void getScans(QVector<Scan> &, const QString &);
    void getSubScans(const QString &, QVector<Scan> &);
    double getWavelength();
	int getNumberOfExcludedRegions();
	int getNumberOfDataPoints();
	int getNumberOfHkl();
	int startIndexReflections();
	int startIndexHkl();
};
#endif // FULLPROFPRFIMPORT_H

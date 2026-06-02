/***************************************************************************
                          fitykcorundumexport.h  -  description
                             -------------------
    begin                : Tue Jul 09 15:06:07 CEST 2014
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

#ifndef FITYKCORUNDUMEXPORT_H
#define FITYKCORUNDUMEXPORT_H

#include "genericexport.h"

class FitykFitExport : public GenericExport
{
public:
    FitykFitExport(QObject * = 0, bool iactive = false);

    inline QString filter() {return "Fityk session (*.fit *.FIT)";}
    inline QString extension() {return "fit";}
    inline QString description() {return "";}

    // these are the virtual functions required by genericexport
    int save(const QString &file, Scan &scan);
    int save(const QString &file, QVector<Scan> &scanHeap);

    inline bool hasMultiScanSupport() {return true;}

private:
    QString compileHeader(const QString &f, double m, double max);
    QString definesString();

    QString getFullRange(const QString &file, Scan &scan);
    QString getCorundumRange(const QString &file, Scan &scan);
    QString getBTcpRange(const QString &file, Scan &scan);
    QString getCaORange(const QString &file, Scan &scan);
    QString getHARange(const QString &file, Scan &scan);

    bool interactive;
};

#endif // FITYKCORUNDUMEXPORT_H

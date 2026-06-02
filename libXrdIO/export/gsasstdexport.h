/***************************************************************************
                          xyexport.h  -  description
                             -------------------
    begin                : Sun Oct 04, 2009
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

#ifndef GSASEXPORT_H
#define GSASEXPORT_H

#include <QtMath>
#include "genericexport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT GsasStdExport : public GenericExport
{
    public:
    GsasStdExport(QObject * = 0);

    inline QString filter() {return "GSAS Standard Powder File (*.fxye *.FXYE)";}
    inline QString extension() {return "fxye";}
    inline QString description() {return "GSAS Standard Powder File in FXY format";}

    int save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());

    // returns the content as a string
    QString getData(const Scan &scan);
    QString getData(const QVector<Scan> &scanHeap);

    inline bool hasMultiScanSupport() {return true;}

private:
    QString getHeader(const Scan &scan);
    QString getGlobalComment();
    QString getBank(const Scan &scan, int i);
    QString getBankComment(const Scan &scan);
    QString getValues(const Scan &scan);
    bool writeToFile(const QString &, const QString &);
};
#endif

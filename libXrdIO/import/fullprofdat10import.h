/***************************************************************************
                          fullprofdat10import.h  -  description
                             -------------------
    begin                : Sat Oct 03, 2009
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

#ifndef FULLPROFDAT10IMPORT_H
#define FULLPROFDAT10IMPORT_H

#include "genericimport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT FullprofDat10Import : public GenericImport
{
    Q_OBJECT

    public:
        FullprofDat10Import(QObject * = 0);

        bool isSupported(const QByteArray &);
        int load(const QString &, QVector<Scan> &, bool minimal = false);
        QString uniqueId() {return "FPDAT10_DAT";}
};
#endif // FULLPROFDAT10IMPORT_H


/***************************************************************************
                          brukerbrmlimport.h  -  description
                             -------------------
    begin                : Sun Aug 25 19:00:00 CEST 2013
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



#ifndef BRUKERBRMLIMPORT_H
#define BRUKERBRMLIMPORT_H

#include <QDomNode>
#include <QVariantHash>
#include "genericimport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BrukerBrmlImport : public GenericImport
{
public:
    BrukerBrmlImport(QObject * = 0);

    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    QString uniqueId() {return "BRUKER_BRML";}

private:
    int loadCompressedArchive(const QString &, QVector<Scan> &, bool minimal = false);
    int loadSingleXmlFile(const QString &, QVector<Scan> &, bool minimal = false);

    int parseUncompressedContainer(const QDomNode &, QVector<Scan> &, const QString &, const QList<double> &, const QString &);
    int parseCompressedContainer(const QDomNode &, QVector<Scan> &, const QString &, const QList<double> &, const QString &);

    void parseDetector(const QDomNode &node, QVariantHash &auxInfo);
    void parseTrack(const QDomNode &node, QVariantHash &auxInfo, const QString &prefix);
    void parseMeasurementContainerWavelength(const QString &f, double &ka1, double &ka2, double &kb);
    void parseRawDataWavelength(const QDomDocument &, double &ka1, double &ka2, double &kb);

    QByteArray gUncompress(const QByteArray &);

    bool verbose;
};

#endif // BRUKERBRMLIMPORT_H

/***************************************************************************
                          panalyticalxrdmlimport.h  -  description
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

#ifndef PANALYTICALXRDMLIMPORT_H
#define PANALYTICALXRDMLIMPORT_H

#include "genericimport.h"

#include <QtXml>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT PanalyticalXrdmlImport : public GenericImport
{
    Q_OBJECT

    public:
        PanalyticalXrdmlImport(QObject * = 0);

        bool isSupported(const QByteArray &);
        int load(const QString &, QVector<Scan> &, bool minimal = false);
        QString uniqueId() {return "PANALYTICAL_XRDML";}

private:
        double version;

        void parseUsedWavelength(const QDomNode &, QVariantHash &);
        void parseIncidentBeamPath(const QDomNode &, QVariantHash &);
        void parseDiffractedBeamPath(const QDomNode &, QVariantHash &);
        void parseScanHeader(const QDomNode &, Scan &);
        bool parseScanDataPoints(const QDomNode &, Scan &, bool);

        void parseDivergenceSlit(const QDomNode &, QVariantHash &, const QString &);
        void parseSollerSlit(const QDomNode &, QVariantHash &, const QString &);
        void parseMask(const QDomNode &, QVariantHash &, const QString &);
        void parseXrayMirror(const QDomNode &, QVariantHash &, const QString &);
        void parseMonochromator(const QDomNode &, QVariantHash &, const QString &);
        void parseAntiScatterSlit(const QDomNode &, QVariantHash &, const QString &);
        void parseFilter(const QDomNode &, QVariantHash &, const QString &);

        double commonBeamAttenuationFactor(const QDomNode &);
        QList<double> beamAttenuationFactors(const QDomNode &);
        double commonDivergenceCorrection(const QDomNode &);
        QList<double> divergenceCorrections(const QDomNode &);
};

#endif //PANALYTICALXRDMLIMPORT_H

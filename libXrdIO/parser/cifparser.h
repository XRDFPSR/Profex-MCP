/***************************************************************************
                          cifparser.cpp  -  description
                             -------------------
    begin                : Tue Aug 27 20:30:00 CEST 2013
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

#ifndef CIFPARSER_H
#define CIFPARSER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QDomDocument>
#include "../crystal/crystalstructure.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT CifParser
{
public:
    CifParser(const QString &);

    QString cifString() const;
    CrystalStructure getCrystalStructure(bool *ok = 0) const;

private:
    typedef QMap<QString, QString> Atom;

    QString cifOrig;
    QString cifStripped;
    QStringList cifList;
    QString filename;
    QDomDocument xmlDoc;

    void composeStrString();

    QString xmlString();
    QDomDocument getXmlDocument();
    QString stripComments(const QString &);

    QString phaseName();
    QList<Atom> atomBlock(const QString &);
    QMap<QString, double> bAniso();
    QStringList symOps();

    QString stripStdDev(const QString &);

    QString getStringByTag(const QString &);
    double getDoubleByTag(const QString &, bool *ok = 0);
    QStringList getLoopBlock(const QString &);

    QString getDatabasePrefix();

    double bisoFromUiso(double uiso);
    double bisoFromBeta(double a, double b, double c,
                        double alpha, double beta, double gamma,
                        double b11, double b22, double b33,
                        double b12, double b13, double b23);
    double bisoFromBaniso(double alpha, double beta, double gamma,
                          double b11, double b22, double b33,
                          double b12, double b13, double b23);
    double bisoFromUaniso(double alpha, double beta, double gamma,
                          double u11, double u22, double u33,
                          double u12, double u13, double u23);
};

#endif // CIFPARSER_H

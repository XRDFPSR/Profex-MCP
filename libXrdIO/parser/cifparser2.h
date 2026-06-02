/***************************************************************************
                          cifparser2.h  -  description
                             -------------------
    begin                : Wed Feb 03 20:30:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#ifndef CIFPARSER2_H
#define CIFPARSER2_H

#include "../crystal/crystalstructure.h"
#include <QFileInfo>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT CifParser2
{
public:
    explicit CifParser2();
    explicit CifParser2(const QString &);

    void parseSourceString(const QString &, const QString &);
    QString cifString() const;
    CrystalStructure getCrystalStructure(int, bool *ok = 0) const;
    QList<CrystalStructure> getAllCrystalStructures(bool *ok = 0) const;

private:
    QStringList cifStr;
    QFileInfo cifFile;
    QStringList commentHeader;
    QList<CrystalStructure> cifDataCrystStructures;
    const QString ERRSTR = "ERROR";

    QList<QMap<QString, QStringList> > parseCif(const QStringList &);
    QStringList stripComments(const QStringList &);
    QList<QStringList> splitDataBlocks(const QStringList &);
    QList<QMap<QString, QStringList> > unifyDataBlocks(const QList<QMap<QString, QStringList> > &);
    QList<QMap<QString, QStringList> > unifyDataBlocksSpringerMaterials(const QList<QMap<QString, QStringList> > &);
    QList<QMap<QString, QStringList> > unifyDataBlocksGlobalHeader(const QList<QMap<QString, QStringList> > &);
    QStringList tokenize(const QStringList &);
    QString multiLineToken(const QStringList &l, int &n);
    QMap<QString, QStringList> parseTokens(const QStringList&);
    QStringList getParameter(const QString &, const QMap<QString, QStringList> &m);
    CrystalStructure toStructure(const QMap<QString, QStringList> &);

    int parseParameter(QMap<QString, QStringList> &, const QStringList &, int);
    int parseLoop(QMap<QString, QStringList> &, const QStringList &, int);
    QString stripStdDev(const QString &);
    QString getDatabaseCode(const QMap<QString, QStringList> &);
    QString getPhaseName(const QMap<QString, QStringList> &);
    QString getFormula(const QMap<QString, QStringList> &);
    int getITnum(const QMap<QString, QStringList> &);
    QString getHMsymb(const QMap<QString, QStringList> &);
    QList<QStringList> getSymOps(const QMap<QString, QStringList> &);

    double getCellA(const QMap<QString, QStringList> &);
    double getCellB(const QMap<QString, QStringList> &);
    double getCellC(const QMap<QString, QStringList> &);
    double getCellAlpha(const QMap<QString, QStringList> &);
    double getCellBeta(const QMap<QString, QStringList> &);
    double getCellGamma(const QMap<QString, QStringList> &);
    double getCellParam(const QMap<QString, QStringList> &, const QString &);

    QList<CrystalAtom> getAtomList(const QMap<QString, QStringList> &, const CrystalStructure &cStructure);
    QMap<QString, QMap<int, QString> > getAtomDataStructure(const QMap<QString, QStringList> &);

    bool getAtomIsDummy(const QMap<QString, QMap<int, QString> > &, int);
    bool getAtomCoordinates(const QMap<QString, QMap<int, QString> > &, int, double &, double &, double &);
    bool getAtomType(const QMap<QString, QMap<int, QString> > &, int, QString &);
    bool getAtomBiso(const QMap<QString, QMap<int, QString> > &, int, double &, const CrystalStructure &);
    bool getAtomOccupancy(const QMap<QString, QMap<int, QString> > &, int, double &);
    bool getAtomWyckoffAndMultiplicity(const QMap<QString, QMap<int, QString> > &, int, QString &, int &);
};

#endif // CIFPARSER2_H

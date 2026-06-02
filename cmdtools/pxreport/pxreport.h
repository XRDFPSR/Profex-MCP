/***************************************************************************
                          pxreport.h  -  description
                             -------------------
    begin                : Tue Sep 04 19:42:15 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#include <QString>
#include "../libXrdIO/parser/bgmnlstparser.h"
#include "../libXrdIO/parser/bgmnsavparser.h"
#include "../libXrdIO/chemtabledata.h"

class PxReport {

public:
    PxReport(const QString &savFile, const QString &lstFile, const QString &parFile, const QString &diaFile, const QString &outFile);

private:
    QByteArray getSvg(const QString &dia, double aspectRatio, double eps1, double eps2, double eps3);
    QStringList getGlobalIncludeList();
    QStringList getLocalIncludeList();
    QString getChemistryHtmlTable(BgmnSavParser &sparser, BgmnLstParser &, ChemistryMode mode);
};

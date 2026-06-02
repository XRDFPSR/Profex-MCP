/***************************************************************************
                          textureplusexport.cpp  -  description
                             -------------------
    begin                : Thu June 17, 2013
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

#include "textureplusexport.h"

TexturePlusExport::TexturePlusExport(QObject *parent) :
    GenericExport(parent)
{
    uId = "TEXPLUS_XYP";
}

int TexturePlusExport::save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &)
{
    qDebug() << QString("TexturePlusExport::save(): Saving scan to file %1").arg(file);
    QString ostr;
    QTextStream str(&ostr);

    str << "TexturePlus input file created with Profex" << '\r' << '\n';
    str << "C RT I A R" << '\r' << '\n';
    str << "C Raw Data" << '\r' << '\n';
    str << "C Count time=" << '\r' << '\n';
    str << "C " << QString("%1").arg(scan.waveLength(), 0, 'f', 5) << '\r' << '\n';
    str << "C START ANGLE = " << QString("%1").arg(scan.startAngle(), 0, 'f', 6) << '\r' << '\n';
    str << "C END ANGLE = " << QString("%1").arg(scan.endAngle(), 0, 'f', 6) << '\r' << '\n';
    str << "C STEP SIZE = " << QString("%1").arg(scan.stepSize(), 0, 'f', 6) << '\r' << '\n';

    QVector<double> avec = scan.pDataAngle();
    QVector<double> ivec = scan.pDataIntensity();

    int size = qMin(avec.size(), ivec.size());

    for (int i = 0; i < size; ++i) {
        str << QString("%1 %2").arg(avec.at(i), 0, 'f', 6).arg(ivec.at(i), 0, 'f', 6) << '\r' << '\n';
    }

    return writeFile(file, ostr);
}

int TexturePlusExport::save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &)
{
    if (!scanHeap.size()) return 0;
    return save(file, scanHeap[0]);
}

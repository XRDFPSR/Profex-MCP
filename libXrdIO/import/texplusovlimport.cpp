/***************************************************************************
                          texplusovlimport.cpp  -  description
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

#include "texplusovlimport.h"

TexplusOvlImport::TexplusOvlImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "ovl";
    descr = QLatin1String("TexturePlus corrected rocking curve");
}

bool TexplusOvlImport::isSupported(const QByteArray &ba)
{
    QString header = QString(ba).left(QString(ba).indexOf("\n"));
    QRegularExpression rx("^\\s*Omega\\s+Mean\\s+Negative\\s+Positive\\s*$");
    QRegularExpressionMatch rm = rx.match(header);
    return rm.hasMatch();
}

int TexplusOvlImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    qDebug() << QString("TexplusOvlImport::load(): Loading file %1").arg(file);
    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.size() < 4) {
        qDebug() << QString("TexplusOvlImport::load(): Insufficient lines read from file %1").arg(file);
        return -1;
    }

    QMap<double, double> values;

    double ang = 0.0;
    double vpos = 0.0;
    double vneg = 0.0;

    bool b_ang;
    bool b_vpos;
    bool b_vneg;

    for (int j = 4; j < content.size(); ++j) {
        QStringList line = content.at(j).split(QRegularExpression("\\s+"));
        if (line.size() < 4) {
            continue;
        }

        ang = line.at(0).toDouble(&b_ang);
        vpos = line.at(2).toDouble(&b_vpos);
        vneg = line.at(3).toDouble(&b_vneg);

        if (b_ang && b_vpos && b_vneg) {
            values.insert(-ang, vneg);
            values.insert(ang, vpos);
        }
    }

    QVector<double> vec_a;
    QVector<double> vec_i;

    QMap<double, double>::const_iterator k = values.constBegin();
    while (k != values.constEnd()) {
        vec_a.push_back(k.key());
        vec_i.push_back(k.value());
        ++k;
    }

    Scan scan(file);
    scan.setSourceFileName(file);
    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);
    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return scanHeap.size();
}

/***************************************************************************
                          seifertvalimport.cpp  -  description
                             -------------------
    begin                : Thu Mar 14, 2013
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

#include "seifertvalimport.h"

SeifertValImport::SeifertValImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "val";
    descr = QLatin1String("Seifert/FPM VAL scan");

    nMode = 0;
    nScans = 0;
    nWavelength = "";
}

bool SeifertValImport::isSupported(const QByteArray &ba)
{
    if (QString(ba).left(7) == "FilePar") {
        return true;
    }

    return false;
}

int SeifertValImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    qDebug() << QString("SeifertValImport::load(): Loading file %1").arg(file);

    QFile ff(file);

    if (!ff.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << QString("SeifertValImport::load(): Could not read file %1").arg(file);
        return -1;
    }

    QStringList content(QString::fromLatin1(ff.readAll()).split(QRegularExpression("\\r\\n|\\n")));
    ff.close();

    if (content.size() < 11) {
        qDebug() << QString("SeifertValImport::load(): No valid data found in file %1").arg(file);
        return -1;
    }

    nMode = content.at(3).toInt();
    nWavelength = content.at(6);
    nScans = content.at(10).toInt();

    QRegularExpression rxBerPar("^BerPar\\d+$");
    QList<int> beginningBerPar;

    // find the indexes of all "BerPar[nn]" lines
    int idx = content.indexOf(rxBerPar, 0);

    while (idx > -1) {
        beginningBerPar.append(idx);
        idx = content.indexOf(rxBerPar, idx+1);
    }

    for (int i = 0; i < beginningBerPar.size(); ++i) {
        // we will count a fixed number from here, so check if the stringlist is long enough
        if (content.size() <= beginningBerPar.at(i) + 11) {
            qDebug() << QString("SeifertValImport::load(): Incomplete scan found in file %1").arg(file);
            break;
        }

        int n = beginningBerPar.at(i); // index of BerPar
        int p = n + 13;                // first intensity value
        double tps = -1.0;

        double startAng = content.at(n + 2).toDouble();
        double endAng = content.at(n + 3).toDouble();
        double stepSize = content.at(n + 4).toDouble();
        QString kVorwahl = content.at(n + 5);
        int points = content.at(n + 8).toInt();

        if (kVorwahl == "T") {
            tps = content.at(n + 6).toDouble();
        }

        if (qFuzzyIsNull(startAng * endAng * stepSize * double(points))) {
            qDebug() << QString("SeifertValImport::load(): Invalid values found in file %1").arg(file);
            break;
        }

        if (content.size() < p + points) {
            qDebug() << QString("SeifertValImport::load(): Invalid number of values found in file %1").arg(file);
            break;
        }

        QVector<double> vec_a;       // angle
        QVector<double> vec_i;       // intensity

        for (int j = 0; j < points; ++j) {
            double ang = startAng + (double(j) / double(points - 1)) * (endAng - startAng);
            double its = content.at(j + p).toDouble();

            // if (tps > 0.0) its *= tps;

            vec_a.push_back(ang);
            vec_i.push_back(its);
        }

        Scan scan(content.at(1), QColor(), 1);
        scan.setSourceFileName(file);
        scan.setDataAng(vec_a);
        scan.setDataInt(vec_i);
        scan.setTimePerStep(tps);
        scan.setNamedWaveLength(nWavelength);
        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);
    }

    return beginningBerPar.size();
}

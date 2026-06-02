/***************************************************************************
                          jademdiimport.cpp  -  description
                             -------------------
    begin                : Tue Aug 27, 2013
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


#include "jademdiimport.h"

JadeMdiImport::JadeMdiImport(QObject *parent)
    : GenericImport(parent)
{
    extens << "mdi";
    descr = QLatin1String("MDI Jade ASCII scan");
}

bool JadeMdiImport::isSupported(const QByteArray &ba)
{
    if (ba.mid(16, 3) == "DIF") {
        return true;
    }

    return false;
}

int JadeMdiImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    qDebug() << QString("JadeMdiImport::load(): Reading file %1").arg(file);

    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.size() < 3) {
        qDebug() << QString("JadeMdiImport::load(): No data found in file %1").arg(file);
        return -1;
    }

    QFileInfo fi(file);
    QString comment = content.at(0).mid(20, content.at(0).length()-20);
    int nScans = 0;

    int i = 1;

    while (i < content.size()) {
        QStringList line = content.at(i).split(" ", Qt::SkipEmptyParts);

        if (line.size() < 7) {
            qDebug() << QString("JadeMdiImport::load(): No scan parameters found %1").arg(file);
            return -1;
        }

        Scan scan(fi.completeBaseName(), QColor(), 1);
        scan.setSourceFileName(file);
        scan.setComment(comment);

        bool ok;
        double startAng = line.at(0).toDouble(&ok);
        qDebug() << QString("JadeMdiImport::load(): Start angle = %1").arg(startAng);

        if (!ok) {
            qDebug() << QStringLiteral("JadeMdiImport::load(): Could not read start angle");
            return -1;
        }

        double stepSize = line.at(1).toDouble(&ok);
        qDebug() << QString("JadeMdiImport::load(): Step size = %1").arg(stepSize);

        if (!ok) {
            qDebug() << QStringLiteral("JadeMdiImport::load(): Could not read step size");
            return -1;
        }

        double timePerStep = line.at(2).toDouble(&ok);
        qDebug() << QString("JadeMdiImport::load(): Time per step = %1").arg(timePerStep);

        if (!ok) {
            qDebug() << QStringLiteral("JadeMdiImport::load(): Could not read time per step");
            timePerStep = -1.0;
        }

        scan.setTimePerStep(timePerStep);
        scan.setNamedWaveLength(line.at(3).simplified());

        double wavelength = line.at(4).toDouble(&ok);
        qDebug() << QString("JadeMdiImport::load(): Wavelength = %1").arg(wavelength);

        if (!ok) {
            qDebug() << QStringLiteral("JadeMdiImport::load(): Could not read wave length");
        } else {
            scan.setWaveLength(wavelength);
        }

        double endAng = line.at(5).toDouble(&ok);
        qDebug() << QString("JadeMdiImport::load(): End angle = %1").arg(endAng);

        if (!ok) {
            qDebug() << QStringLiteral("JadeMdiImport::load(): Could not read end angle");
            return -1;
        }

        int nPoints = line.at(6).toInt(&ok);
        qDebug() << QString("JadeMdiImport::load(): Number of data points = %1").arg(nPoints);

        if (!ok) {
            qDebug() << QStringLiteral("JadeMdiImport::load(): Could not read the number of data points");
            return -1;
        }

        QString rangeId;
        if (line.size() > 7) {
            rangeId = line.at(7);
            scan.setName(QString("%1 %2").arg(fi.completeBaseName()).arg(rangeId));
        }

        Q_UNUSED(timePerStep);
        Q_UNUSED(endAng);

        // move to the next line
        i++;

        // calculate the number of lines to read with data points
        int blockLines = nPoints / 8;

        // check if there is a data line with less than 8 points at the end of the block
        if (nPoints % 8) { // nPoints modulo 8
            blockLines++;
        }

        // abort if looping over blockLines would exceed the end of the file
        if (i + blockLines > content.size()) {
            qDebug() << QString("JadeMdiImport::load(): Exceeding the end of file %1").arg(file);
            return -1;
        }

        QVector<double> vec_a;
        QVector<double> vec_i;

        // loop through the data lines
        for (int l = 0; l < blockLines; ++l) {
            QStringList dln = content.at(i+l).split(" ", Qt::SkipEmptyParts);

            for (int k = 0; k < dln.size(); ++k) {
                double its = dln.at(k).toDouble();
                vec_i.push_back(its);
            }
        }

        for (int j = 0; j < vec_i.size(); ++j) {
            double ang = startAng + (double(j) / double(vec_i.size() - 1)) * (endAng - startAng);
            vec_a.push_back(ang);
        }

        scan.setDataAng(vec_a);
        scan.setDataInt(vec_i);
        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);

        // progress the scan counter
        nScans++;

        // set i to the beginning of the next block
        i += blockLines;
    }

    return nScans;
}

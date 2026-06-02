/***************************************************************************
                          rigakudifimport.cpp  -  description
                             -------------------
    begin                : Sun Dec 30, 2012
    copyright            : (C) 2012 by Nicola Doebelin
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


#include "rigakudifimport.h"

RigakuDifImport::RigakuDifImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "dif";
    descr = QLatin1String("Rigaku DIF scan");
}

bool RigakuDifImport::isSupported(const QByteArray &ba)
{
    QRegularExpression rx("^\\s*\\d{1,2}/\\d{1,2}/\\d{1,2}\\s+\\d{1,2}:\\d{1,2}\\s+DIF\\s+\\d{1,2}\\.\\d{1,2}\\.\\d{1,2}\\s*\\n");
    QRegularExpressionMatch rm = rx.match(ba);
    return rm.hasMatch();
}

/* file format:
  Line 0: Date Time etc.
  Line 1: startang stepsize seconds-per-step target wavelength endang number-of-points
  Line 2-n: val val val val val val val val
*/
int RigakuDifImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    qDebug() << QString("RigakuDifImport::load(): Loading file %1").arg(file);
    QStringList content(BgmnFileIO::readTextFileLines(file));

    // do some file checks
    if (content.isEmpty()) {
      qDebug() << QString("RigakuDifImport::load(): Could not open file %1").arg(file);
      return -1;
    }

    // two lines with comments and scan info, and two data lines required
    if (content.size() < 4) {
        qDebug() << QString("RigakuDifImport::load(): File is too short %1").arg(file);
        return -1;
    }

    QRegularExpression sep("\\s+");

    // read second line. It contains measurement parameters
    QString line = content.at(1).trimmed();
    QStringList settings = line.split(sep);

    // some range checks
    if (settings.size() < 7) {
        qDebug() << QString("RigakuDifImport::load(): No valid scan parameters found in file %1").arg(file);
        return -1;
    }

    // compute all necessary parameters
    double startang = settings.at(0).toDouble();
    double timeperstep = settings.at(2).toDouble();
    double endang = settings.at(5).toDouble();
    double numpoints = settings.at(6).toDouble();
    double wl = settings.at(4).toDouble();

    if (wl == 0.0) {
        qDebug() << QString("RigakuDifImport::load(): Could not read wavelength of kAlpha1. Assuming CuKa1 with 1.54056 A");
        wl = 1.54056;
    }

    // scan range checks
    if ((endang <= startang) || (numpoints <= 1.0) || (startang <= 0.0)) {
        qDebug() << QString("RigakuDifImport::load(): Error in scan parameters found in file %1").arg(file);
        return -1;
    }

    double stepsize = (endang - startang) / (numpoints - 1.0);
    Q_UNUSED(stepsize);

    QVector<double> vec_a;       // angle
    QVector<double> vec_i;       // intensity

    // continue reading the values
    for (int j = 2; j < content.size(); ++j) {
        line = content.at(j).trimmed();

        // skip anything that looks like a comment sign
        if (line.left(1) == "!" || line.left(1) == "%" || line.left(1) == "&" || line.left(2) == "//") {
            continue;
        }

        // split the line into values
        QStringList val = line.split(sep);

        // check if we have some content
        if (!val.size()) {
            continue;
        }

        int p = val.size();
        // store the values in the vectors, then advance the angle
        for (int k = 0; k < p; ++k) {
            double ang = startang + (double(k) / double(p - 1)) * (endang - startang);
            double its = val.at(k).toDouble();

            vec_a.push_back(ang);
            vec_i.push_back(its);
        }
    }

    Scan scan(file, QColor(), 1);
    scan.setSourceFileName(file);
    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);
    scan.setWaveLength(wl);
    scan.setTimePerStep(timeperstep);
    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return scanHeap.size();
}

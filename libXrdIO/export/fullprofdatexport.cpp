/***************************************************************************
                          fullprofdatexport.cpp  -  description
                             -------------------
    begin                : Mon Jan 20 14:16:07 CEST 2009
    copyright            : (C) 2005 by Nicola Doebelin
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



#include "fullprofdatexport.h"
#include <limits>

using namespace std;

FullprofDatExport::FullprofDatExport(QObject *parent) :
    GenericExport(parent)
{
    uId = "FPDAT10_DAT";
}

int FullprofDatExport::save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &)
{
    QString ostr;

    int ms = scan.size();
    emit maxSteps(ms);
    QString t = "!" + file + " XYDATA\n!\n";
    t += "!Fullprof INSTRM=10, created with Profex2\n!";
    t += QString("%1").arg(scan.startAngle(), 9, 'f', 5);
    t += QString("%1").arg(scan.stepSize(), 9, 'f', 5);
    t += QString("%1").arg(scan.endAngle(), 9, 'f', 5);
    t += "\n!\n!";
    QTextStream stream( &ostr );
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    stream << t << Qt::endl;
#else
    stream << t << endl;
#endif
    int m = 0;
    for (int i = 0; i < scan.size(); i++) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        stream << QString("%1 %2").arg(scan.angle(i), 9, 'e', 7).arg(scan.intensity(i), 9, 'e', 7) << Qt::endl;
#else
        stream << QString("%1 %2").arg(scan.angle(i), 9, 'e', 7).arg(scan.intensity(i), 9, 'e', 7) << endl;
#endif
        if (i > m) {
            emit progress(i);
            m = m + 60;
        }
    }

    emit progress(scan.size());

    return writeFile(file, ostr);
}

int FullprofDatExport::save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &)
{
    if (scanHeap.size()) {
        return save(file, scanHeap[0]);
    }

    return -1;
}

int FullprofDatExport::saveSum(const QString &file, const QVector<Scan> &scanHeap)
{
    if (!scanHeap.size()) {
        return 0;
    }

    QString ostr;

    // determine the length of the output scan by identifying the shortest vector
    int tlength = numeric_limits<int>::max();
    for (int i = 0; i < scanHeap.size(); ++i) {
        tlength = qMin(tlength, scanHeap[i].size());
    }

    // something went wrong...
    if (tlength == numeric_limits<int>::max()) {
        return 0;
    }

    int ms = (scanHeap.size() + 1) * tlength;
    emit maxSteps(ms);

    // initialize vectors containing output values
    QVector<double> vec_ang(tlength, 0.0);
    QVector<double> vec_val(tlength, 0.0);

    int m = 0;
    int p = 0;

    // loop over the scans and calculate the output vectors
    for (int j = 0; j < scanHeap.size(); ++j) {
        p = 0;
        for (int i = 0; i < tlength; ++i) {
            vec_ang[i] = scanHeap[j].angle(i);
            vec_val[i] += scanHeap[j].intensity(i);

            if (i > p) {
                emit progress(m);
                m += 200;
                p += 200;
            }
        }
    }


    // write the file header
    QString t = "!" + file + " XYDATA\n!\n";
    t += "!Fullprof INSTRM=10, created with Profex2\n!";
    t += QString("%1").arg(vec_ang.at(0), 9, 'f', 5);
    t += QString("%1").arg(vec_ang.at(1) - vec_ang.at(0), 9, 'f', 5);
    t += QString("%1").arg(vec_ang.at(vec_ang.size()-1), 9, 'f', 5);
    t += "\n!\n!";

    QTextStream stream( &ostr );
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    stream << t << Qt::endl;
#else
    stream << t << endl;
#endif

    p = 0;
    for (int i = 0; i < vec_ang.size(); i++) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        stream << QString("%1 %2").arg(vec_ang.at(i), 9, 'e', 7).arg(vec_val.at(i), 9, 'e', 7) << Qt::endl;
#else
        stream << QString("%1 %2").arg(vec_ang.at(i), 9, 'e', 7).arg(vec_val.at(i), 9, 'e', 7) << endl;
#endif

        if (i > p) {
            emit progress(m);
            m += 200;
            p += 200;
        }
    }

    emit progress(ms);

    return writeFile(file, ostr);
}




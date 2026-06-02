/***************************************************************************
                          asciixyimport.cpp  -  description
                             -------------------
    begin                : Sat Oct 03, 2009
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


#include "gnresgimport.h"

GnrEsgImport::GnrEsgImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "esg";
    descr = QLatin1String("GNR ESG File");
}

bool GnrEsgImport::isSupported(const QByteArray &ba)
{
    return ba.trimmed().left(12) == QByteArray("_pd_block_id");
}

int GnrEsgImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.isEmpty()) {
        qDebug() << QString("GnrEsgImport::load(): Could not open file %1").arg(file);
        return -1;
    }

    QFileInfo fi(file);
    double min;
    double max;
    double wl;
    QString label;
    int idx = 0;
    int n = 0;

    while (idx >= 0) {
        idx = parseHeader(content, idx, min, max, wl, label);
        if (idx >= 0) {
            Scan scan = parseScan(content, idx, QString("%1").arg(fi.completeBaseName()), min, max);
            scan.setSourceFileName(file);
            scan.setWaveLength(wl);
            scan.setTypes(Scan::XY | Scan::MEASURED);
            scanHeap.push_back(scan);
            ++idx;
            ++n;
        }
    }

    return n;
}

int GnrEsgImport::parseHeader(const QStringList &ct, int idx, double &min, double &max, double &wl, QString &lbl)
{
    QRegularExpression rxBlck("_pd_block_id\\s+(.*)");
    QRegularExpression rxMin("_pd_meas_2theta_range_min\\s+(-?\\d+\\.?\\d*).*");
    QRegularExpression rxMax("_pd_meas_2theta_range_max\\s+(-?\\d+\\.?\\d*).*");
    QRegularExpression rxWl("_diffrn_radiation_wavelength\\s+(\\d+\\.?\\d*).*");

    int blkStart = ct.indexOf(rxBlck, idx);
    if (blkStart < 0) return -1;

    int idxMin = ct.indexOf(rxMin, blkStart);
    int idxMax = ct.indexOf(rxMax, blkStart);
    int idxWl  = ct.indexOf(rxWl,  blkStart);

    if ((idxMin < 0) || (idxMax < 0) || (idxWl < 0)) return -1;

    QRegularExpressionMatch rm = rxBlck.match(ct.at(blkStart));
    if (rm.hasMatch()) lbl = rm.captured(1);

    rm = rxMin.match(ct.at(idxMin));
    if (rm.hasMatch()) min = rm.captured(1).toDouble();

    rm = rxMax.match(ct.at(idxMax));
    if (rm.hasMatch()) max = rm.captured(1).toDouble();

    rm = rxWl.match(ct.at(idxWl));
    if (rm.hasMatch()) wl = rm.captured(1).toDouble();

    return blkStart;
}

Scan GnrEsgImport::parseScan(const QStringList &ct, int idx, const QString &lbl, double min, double max)
{
    Scan scan(lbl, QColor(), 1);

    int i = ct.indexOf(QRegularExpression("_pd_meas_intensity_total.*"), idx) + 1;
    if (i == 0) return scan;
    if (ct.size() < 2) return scan;

    QRegularExpression rxData("(-?\\d+\\.?\\d*).*");
    QVector<double> vx;
    QVector<double> vy;

    while (i < ct.size()) {
        if (ct.at(i).left(1) != "_") {
            QRegularExpressionMatch rm = rxData.match(ct.at(i));
            if (rm.hasMatch()) {
                vy.append(rm.captured(1).toDouble());
            }
        }
        ++i;
    }

    for (int n = 0; n < vy.size(); ++n) {
        double d = double(n) / double(vy.size() - 1);
        vx.append(min + d * (max - min));
    }

    scan.setDataAng(vx);
    scan.setDataInt(vy);

    return scan;
}

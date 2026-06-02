/***************************************************************************
                          pdcifimport.cpp  -  description
                             -------------------
    begin                : Tue Jul 19, 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#include "pdcifimport.h"

PdCifImport::PdCifImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "cif";
    descr = QLatin1String("pdCIF Powder Diffraction Data");

}

bool PdCifImport::isSupported(const QByteArray &ba)
{
    QString tag("_pd_block_id");
    return ba.contains(tag.toLatin1());
}

int PdCifImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.isEmpty()) {
        return 0;
    }

    static QRegularExpression rxBlockId("_pd_block_id\\s+(\\S+)");
    int idxBlockId = content.indexOf(rxBlockId, 0);

    if (idxBlockId < 0) {
        return 0;
    }

    QRegularExpressionMatch rmBlockId = rxBlockId.match(content.at(idxBlockId));
    QString blockId = rmBlockId.captured(1);
    Q_UNUSED(blockId);

    QStringList loopTags;
    int dataStart = parseLoop(content, loopTags);

    static QRegularExpression rxAngle("_pd_meas_2theta_scan|_pd_proc_2theta_corrected");
    static QRegularExpression rxIntensity("_pd_meas_counts_|_pd_proc_counts_|_pd_calc_intensity_|_pd_meas_intensity_|_pd_proc_intensity_");

    int angleCol = loopTags.indexOf(rxAngle, 0);
    QList<int> intensityCols;

    for (int i = 0; i < loopTags.size(); ++i) {
        if (loopTags.at(i).contains(rxIntensity)) {
            intensityCols.append(i);
        }
    }

    scanHeap.append(parseData(file, content, dataStart, angleCol, intensityCols, loopTags));

    return intensityCols.size();
}

int PdCifImport::parseLoop(const QStringList &content, QStringList &loopTags)
{
    static QRegularExpression rxLoop("^\\s*loop_\\s*$");

    int lBegin = content.indexOf(rxLoop, 0);

    if (lBegin < 0) {
        return 0;
    }

    lBegin++;

    for (int i = lBegin; i < content.size(); ++i) {
        if (content.at(i).trimmed().left(1) != "_") {
            return i;
        }

        loopTags.append(content.at(i).trimmed());
    }

    return 0;
}

QVector<Scan> PdCifImport::parseData(const QString &file, const QStringList &content, int start, int angCol, const QList<int> &intCol, const QStringList &loopTags)
{
    static QRegularExpression rxSep("\\s+");
    QVector<Scan> tempHeap;
    int maxCol = angCol;

    for (int i = 0; i < intCol.size(); ++i) {
        maxCol = qMax(angCol, intCol.at(i));
    }

    for (int i = 0; i < intCol.size(); ++i) {
        Scan scan(loopTags.at(intCol.at(i)), QColor(), 1);
        scan.setTypes(Scan::XY | Scan::MEASURED);
        scan.setSourceFileName(file);
        tempHeap.append(scan);
    }

    for (int i = start; i < content.size(); ++i) {
        if (content.at(i).trimmed().isEmpty()) {
            return tempHeap;
        }

        QStringList line(content.at(i).trimmed().split(rxSep));
        if (line.size() <= maxCol) continue;

        double ang = line.at(angCol).toDouble();

        for (int j = 0; j < intCol.size(); ++j) {
            double intens = line.at(intCol.at(j)).toDouble();
            tempHeap[j].pDataAngle().append(ang);
            tempHeap[j].pDataIntensity().append(intens);
        }
    }

    return tempHeap;
}

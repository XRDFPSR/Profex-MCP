/***************************************************************************
                          philipsudfimport.cpp  -  description
                             -------------------
    begin                : Thu May 21, 2013
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

#include "philipsudfimport.h"

PhilipsUdfImport::PhilipsUdfImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "udf";
    descr = QLatin1String("Philips UDF scan");
}

bool PhilipsUdfImport::isSupported(const QByteArray &ba)
{
    // check if the first line is of format "param,value,/"
    QString line = QString(ba).left(QString(ba).indexOf("\n")).trimmed();

    static QRegularExpression rx("^[^,]+,\\s*[^,]+,\\s*/$");
    QRegularExpressionMatch rm = rx.match(line);
    return rm.hasMatch();
}

int PhilipsUdfImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    qDebug() << QString("PhilipsUdfImport::load(): Loading file %1").arg(file);
    QStringList content(BgmnFileIO::readTextFileLines(file));

    static QRegularExpression rxStartData("^RawScan");
    int dataBlockStartIndex = content.indexOf(rxStartData);

    if (dataBlockStartIndex < 0) {
        qDebug() << QString("PhilipsUdfImport::load(): No valid data found in file %1").arg(file);
        return -1;
    }

    ++dataBlockStartIndex;
    QVector<double> vec_a;       // angle
    QVector<double> vec_i;       // intensity

    Scan scan(file, QColor(), 1);
    scan.setSourceFileName(file);

    static QRegularExpression rxSampleIdent("^SampleIdent,\\s*(.*)\\s*,/$");
    static QRegularExpression rxComment("^Title1,\\s*(.*)\\s*,/$");
    static QRegularExpression rxWaveLength("^Lam?bdaAlpha1,\\s*(.*)\\s*,/$");
    static QRegularExpression rxNamedWaveLength("^Anode,\\s*(.*)\\s*,/$");
    static QRegularExpression rxDataAngleRange("^DataAngleRange,\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*,/$");
    static QRegularExpression rxScanStepTime("^ScanStepTime,\\s*(\\d+\\.?\\d*)\\s*,/$");

    int n = content.indexOf(rxSampleIdent);

    if (n > -1) {
        QRegularExpressionMatch rm = rxSampleIdent.match(content.at(n));
        scan.setName(rm.captured(1));
    }

    n = content.indexOf(rxComment);

    if (content.indexOf(rxComment) > -1) {
        QRegularExpressionMatch rm = rxComment.match(content.at(n));
        scan.setComment(rm.captured(1));
    }

    n = content.indexOf(rxNamedWaveLength);

    if (content.indexOf(rxNamedWaveLength) > -1) {
        QRegularExpressionMatch rm = rxNamedWaveLength.match(content.at(n));
        scan.setNamedWaveLength(rm.captured(1));
    }

    n = content.indexOf(rxWaveLength);

    if (content.indexOf(rxWaveLength) > -1) {
        QRegularExpressionMatch rm = rxWaveLength.match(content.at(n));
        scan.setWaveLength(rm.captured(1).toFloat());
    }

    // extract start angle
    double startAng;
    double endAng;

    n = content.indexOf(rxDataAngleRange);

    if (content.indexOf(rxDataAngleRange) > -1) {
        QRegularExpressionMatch rm = rxDataAngleRange.match(content.at(n));        
        bool ok;

        startAng = rm.captured(1).toDouble(&ok);
        if (ok) {
            qDebug() << QString("PhilipsUdfImport::load(): Scan start angle = %1").arg(startAng, 0, 'f', 6);
        } else {
            qDebug() << QString("PhilipsUdfImport::load(): Could not find start angle in file %1").arg(file);
            return -1;
        }

        endAng   = rm.captured(2).toDouble(&ok);
        if (ok) {
            qDebug() << QString("PhilipsUdfImport::load(): Scan end angle = %1").arg(endAng, 0, 'f', 6);
        } else {
            qDebug() << QString("PhilipsUdfImport::load(): Could not find end angle in file %1").arg(file);
            return -1;
        }
    } else {
        qDebug() << QString("PhilipsUdfImport::load(): Could not find angular range in file %1").arg(file);
        return -1;
    }

    // extract time per step
    double stepTime = -1.0;
    n = content.indexOf(rxScanStepTime);

    if (content.indexOf(rxScanStepTime) > -1) {
        QRegularExpressionMatch rm = rxScanStepTime.match(content.at(n));
        stepTime = rm.captured(1).toDouble();
    }

    for (int k = dataBlockStartIndex; k < content.size(); ++k) {
        QStringList values = content.at(k).split(",");

        for (int j = 0; j < values.size(); ++j) {
            bool ok;
            double f = values.at(j).toDouble(&ok);

            if (ok) vec_i.push_back(f);
        }
    }

    for (int i = 0; i < vec_i.size(); ++i) {
        double ang = startAng + (endAng - startAng) * double(i) / double(vec_i.size());
        vec_a.push_back(ang);
    }

    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);
    scan.setTimePerStep(stepTime);
    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return scanHeap.size();
}

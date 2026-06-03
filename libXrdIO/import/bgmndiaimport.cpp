/***************************************************************************
                          bgmndiaimport.cpp  -  description
                             -------------------
    begin                : Thu Jan 26, 2012
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

#include "bgmndiaimport.h"
#include <sstream>
#include <locale>
#include <QtConcurrent>

BgmnDiaImport::BgmnDiaImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "dia";
    descr = QLatin1String("BGMN refined profile");
}

/************* PUBLIC FUNCTIONS *************/

/* - If the load function returns -1, don't use any of the
     get* functions. They are bound to break!

   - Always make sure that the load function was called at least once
     before using the get* functions. Otherwise you'll get an error message.
*/

bool BgmnDiaImport::isSupported(const QByteArray &ba)
{
    if (ba.contains(QByteArray("TITEL="))) {
        return true;
    }

    return false;
}

int BgmnDiaImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal)

    QList<QByteArray> lines = BgmnFileIO::readBinaryFileLines(file);
    if (lines.size() < 3) return -1;

    _header = lines.constFirst();

    if (!parseBinaryData(_data, lines)) {
        qDebug() << QString("BgmnDiaImport::load(): Could not read file %1").arg(file);
        return -1;
    }

    return createScans(scanHeap, file);
}

int BgmnDiaImport::loadAndCheck( const QString &file, QVector<Scan> &scanHeap, int &ll, int &tl, bool minimal)
{
    Q_UNUSED(minimal)

    QList<QByteArray> lines = BgmnFileIO::readBinaryFileLines(file);
    if (lines.size() < 3) return -1;

    _header = lines.constFirst();

    if (!parseBinaryData(_data, lines)) {
        qDebug() << QString("BgmnDiaImport::load(): Could not read file %1").arg(file);
        ll = 0;
        tl = std::numeric_limits<int>().max();
        return -1;
    }

    if (checkSize(ll, tl) <= 0) return -1;
    return createScans(scanHeap, file);
}

/************* PRIVATE FUNCTIONS *************/

/*
 * ll = lines read
 * tl = lines total
 */
int BgmnDiaImport::checkSize(int &ll, int &tl)
{
    int returnValue = -1;
    int m = 0;

    if (_data.size() > 2) {
        static QRegularExpression rx("\\s+M=(\\d+)\\s+");
        QRegularExpressionMatch rm = rx.match(_header);

        if (rm.hasMatch()) {
            m = rm.captured(1).toInt();
            if (m == _totalDataLines) returnValue = 1;
        }
    }

    if (returnValue < 0) {
        ll = 0;
        tl = std::numeric_limits<int>().max();
        qDebug() << QString("BgmnDiaImport::checkSize(): Incomplete file, found %1 data points, expected %2").arg(_totalDataLines).arg(m);
    } else {
        // we return the size of the resulting dataset, not the number of lines read from the dia file, which may be a multiple in case of multi scans
        ll = _data.size();
        tl = _data.size();
    }

    return returnValue;
}

int BgmnDiaImport::createScans(QVector<Scan> &scanHeap, const QString &sourceFile)
{
        QStringList struc = getStructureNames(_header);
        double wavelength = getWaveLength(_header);
        double diffMax = 0.0;

        int nScans  = _data.first().size() - 1;

        if (nScans < 3) {
            qDebug() << QString("BgmnDiaImport::getScans(): The file only contains %1 scans, not a valid DIA file.").arg(nScans);
            return -1;
        }

        scanHeap.clear();

        // only intensity vectors must be initialized. Angle vectors will be copied later.
        // add one more for the difference curve
        for (int i = 0; i <= nScans; ++i) {
            scanHeap.append(Scan());
            scanHeap.last().pDataIntensity() = QVector<double>(_data.size(), 0.0);
            scanHeap.last().setSourceFileName(sourceFile);
        }

        scanHeap[0].pDataAngle() = QVector<double>(_data.size(), 0.0);

        // layout of the scanHeap:
        // [0] = iObs
        // [1] = iCalc
        // [2] = iDiff
        // [3] = iBkgr
        // [4...n] = strucs

        for (int pos = 0; pos < _data.size(); ++pos) {
            const QVector<double> *line = &(_data.at(pos));

            scanHeap[0].pDataAngle()[pos] = line->at(0);

            for (int i = 0; i < line->size(); ++i) {
                if (i < 2)  scanHeap[i].pDataIntensity()[pos] = line->at(i + 1);
                if (i == 2) scanHeap[i].pDataIntensity()[pos] = line->at(1) - line->at(2);
                if (i == 3) scanHeap[i].pDataIntensity()[pos] = line->at(i);
                if (i > 3)  scanHeap[i].pDataIntensity()[pos] = line->at(i) + line->at(3);
            }

            diffMax = qMax(diffMax, scanHeap.at(2).pDataIntensity().at(pos));
        }

        for (int i = 1; i < scanHeap.size(); ++i) {
            scanHeap[i].pDataAngle() = scanHeap[0].pDataAngle();
        }

        // add additional info to the scans
        for (int i = 0; i < scanHeap.size(); ++i) {
            scanHeap[i].setWaveLength(wavelength);

            // copy the angle vector from the first scan to the others
            if (i > 0) {
                scanHeap[i].pDataAngle() = scanHeap[0].pDataAngle();
            }

            if (i == 0) {
                scanHeap[i].setName("I observed");
                scanHeap[i].setColor(QColor(0, 0, 0));
                scanHeap[i].setTypes(Scan::XY | Scan::MEASURED | Scan::REFINED | Scan::ISABOVEBACKGROUND);
            }

            if (i == 1) {
                scanHeap[i].setName("I calculated");
                scanHeap[i].setColor(QColor(255, 0, 0));
                scanHeap[i].setTypes(Scan::XY | Scan::REFINED | Scan::ISABOVEBACKGROUND);
            }

            if (i == 2) {
                scanHeap[i].setName("I difference");
                scanHeap[i].setColor(QColor(187, 187, 187));
                scanHeap[i].setTypes(Scan::XY | Scan::REFINED | Scan::DIFF);
            }

            if (i == 3) {
                scanHeap[i].setName("Background");
                scanHeap[i].setColor(QColor(0, 0, 255));
                scanHeap[i].setTypes(Scan::XY | Scan::REFINED | Scan::BACKGROUND);
            }

            if (i > 3) {
                if ((i - 4) < struc.size()) {
                    scanHeap[i].setName(struc.at(i - 4));
                    scanHeap[i].setTypes(Scan::XY | Scan::REFINED | Scan::PHASE | Scan::ISABOVEBACKGROUND);
                } else {
                    scanHeap[i].setName("Unknown Phase");
                    scanHeap[i].setTypes(Scan::XY | Scan::REFINED | Scan::PHASE | Scan::ISABOVEBACKGROUND);
                }
                scanHeap[i].setColor(QColor());
            }

        }

        if (scanHeap.size() > 3) {
            scanHeap[2].setYoffset(-1.2 * diffMax);
        }

        return scanHeap.size();
}

QStringList BgmnDiaImport::getStructureNames(const QByteArray &line)
{
    static QRegularExpression rxStruc("STRUC\\[\\d+\\]=(\\S+)");
    QRegularExpressionMatch rm = rxStruc.match(line);
    QStringList struc;

    while (rm.hasMatch()) {
        struc.append(rm.captured(1));
        rm = rxStruc.match(_header, rm.capturedEnd(1) + 1);
    }

    return struc;
}

double BgmnDiaImport::getWaveLength(const QByteArray &line)
{
    double wl = 1.54056;

    static QRegularExpression rx("L0=(-?\\d+\\.?\\d*)");
    QRegularExpressionMatch rm = rx.match(line);

    if (rm.hasMatch()) {
        wl = rm.captured(1).toDouble();
        if (!qFuzzyIsNull(wl)) {
            wl = 10.0 / qAbs(wl);
        } else {
            qDebug() << QString("BgmnDiaImport::getWaveLength(): L0 is not valid, returning fallback value of 1.54056.");
        }
    } else {
        qDebug() << QString("BgmnDiaImport::getWaveLength(): L0 not found, returning fallback value of 1.54056.");
    }

    return wl;
}

bool BgmnDiaImport::parseBinaryData(QList<QVector<double> > &data, const QList<QByteArray> &lines)
{
    _totalDataLines = 0;

    // Containers to hold the unique x values and the corresponding averaged y columns.
    QList<double> uniqueX;         // Each unique x value.
    QList<QList<double>> avgYCols; // Each element is a QList<double> of averaged y columns.

    bool firstGroup = true;
    double currentX = 0.0;
    QList<double> groupSum;  // Column‐wise sum of y values for the current group.
    int groupCount = 0;      // Number of lines in the current group.

    // Process each line.
    for (const QByteArray &line : lines) {
        // parseLineColumns() is assumed to parse a line into:
        //   struct LineDataColumns { bool valid; double x; QVector<double> yValues; };
        LineDataColumns ld = parseLineColumns(line);
        if (!ld.valid) {
            continue;  // Skip empty or comment lines.
        }

        _totalDataLines++;

        if (firstGroup) {
            // Start the first group.
            currentX = ld.x;
            groupSum = ld.yValues.toList(); // Convert QVector<double> to QList<double>
            groupCount = 1;
            firstGroup = false;
        } else if (qFuzzyCompare(ld.x, currentX)) {
            // Same x value: accumulate the y values column‐by‐column.
            if (ld.yValues.size() != groupSum.size()) {
                qWarning() << "Inconsistent number of y columns for x =" << currentX;
                continue;
            }
            for (int i = 0; i < groupSum.size(); ++i) {
                groupSum[i] += ld.yValues[i];
            }
            ++groupCount;
        } else {
            // New x encountered: compute and store averages for the previous group.
            QList<double> avg;
            avg.reserve(groupSum.size());
            for (int i = 0; i < groupSum.size(); ++i) {
                avg.append(groupSum[i] / groupCount);
            }
            uniqueX.append(currentX);
            avgYCols.append(avg);

            // Start a new group.
            currentX = ld.x;
            groupSum = ld.yValues.toList();
            groupCount = 1;
        }
    }
    // Process the final group (if any).
    if (!firstGroup) {
        QList<double> avg;
        avg.reserve(groupSum.size());
        for (int i = 0; i < groupSum.size(); ++i) {
            avg.append(groupSum[i] / groupCount);
        }
        uniqueX.append(currentX);
        avgYCols.append(avg);
    }

    // Build the output data.
    // Each row: first element is the unique x value, followed by the averaged y columns.
    data.clear();
    for (int i = 0; i < uniqueX.size(); ++i) {
        QVector<double> row;
        row.reserve(1 + avgYCols[i].size());
        row.append(uniqueX[i]);
        // Append each averaged y value.
        for (double val : avgYCols[i]) {
            row.append(val);
        }
        data.append(row);
    }

    return true;
}

bool BgmnDiaImport::parseDouble(const char *&ptr, const char *end, double &value) {
    while (ptr < end && std::isspace(static_cast<unsigned char>(*ptr))) {
        ++ptr;
    }

    if (ptr >= end) {
        return false;
    }

    const char* start = ptr;

    // Find the end of the number token
    while (ptr < end && !std::isspace(static_cast<unsigned char>(*ptr))) {
        ++ptr;
    }

    std::string token(start, ptr - start);
    std::istringstream iss(token);
    iss.imbue(std::locale::classic()); // Force period as decimal separator
    iss >> value;

    return true;
}

LineDataColumns BgmnDiaImport::parseLineColumns(const QByteArray &line) {
    const char *data = line.constData();
    const char *end = data + line.size();

    // Skip any leading whitespace.
    while (data < end && std::isspace(static_cast<unsigned char>(*data))) {
        ++data;
    }
    // If the line is empty or the title line, mark it as invalid.
    if (data >= end || *data == 'T') {
        return { false, 0.0, QVector<double>() };
    }

    // Parse the x value.
    double x;
    if (!parseDouble(data, end, x)) {
        return { false, 0.0, QVector<double>() };
    }

    // Parse all remaining numbers as y values.
    QVector<double> yValues;
    double y;
    while (parseDouble(data, end, y)) {
        yValues.append(y);
    }

    return { true, x, yValues };
}

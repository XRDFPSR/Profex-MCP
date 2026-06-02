/***************************************************************************
                          graceexport.cpp  -  description
                             -------------------
    begin                : Tue Feb 18 20:28:07 CEST 2016
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


#include "graceexport.h"
#include "scanops.h"
#include "structs.h"
#include <math.h>

GraceExport::GraceExport(QObject *parent) :
    GenericExport(parent)
{
    uId = "GRACE_AGR";
    settings = SettingsManager::getInstance();
    _xmax = 0.0;
    _xmin = 180.0;
    _ymax = 0.0;
    _ymin = 0.0;
    _fillActiveScans = settings->value("graph/fillActiveScan", true).toBool();
    int scanLw = settings->value("graph/lineWidth", 1).toInt();
    int aScanLw = settings->value("graph/activeLineWidth", 2).toInt();
    _activeScansBold = aScanLw > scanLw;
    _hklSymbol = settings->value("config/hklSymbol", 0).toInt();
}


/*
 * convenience function: saves a single scan.
 */
int GraceExport::save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &)
{
    QVector<Scan> vec;
    vec.push_back(scan);
    return save(file, vec);
}

/*
 * composes the grace output and saves it to file
 */
int GraceExport::save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &)
{
    qDebug() << QString("GraceExport::save(): Writing %1 files to %2").arg(scanHeap.size()).arg(file);

    // check if there is any data to export
    if (!scanHeap.size()) {
        qDebug() << QString("GraceExport::save(): No scans to export");
        return 0;
    }

    // check if the file name makes sense (not checking for write permissions etc.)
    if (file.isEmpty()) {
        qDebug() << QString("GraceExport::save(): No valid file name provided");
        return 0;
    }

    parseScanRanges(scanHeap);

    QString fileHeaderString = getFileHeaderString(scanHeap);
    QString scanHeaderString;
    QString hklTickHeaderString;
    QString hklScanHeaderString;
    QString scanDataString;
    QString hklTickDataString;
    QString hklScanDataString;

    int scanSetNumber = 1;

    for (int i = 0; i < scanHeap.size(); ++i) {
        if (!scanHeap.at(i).isVisible()) continue;

        if (scanHeap.at(i).pDataAngle().size() && scanHeap.at(i).pDataIntensity().size()) {
            scanHeaderString += getScanHeaderString(scanHeap.at(i), scanSetNumber, scanHeap.size());
            scanDataString += getScanDataString(scanHeap.at(i), scanSetNumber);
        } else {
            if (scanHeap.at(i).pDataHkl().size()) {
                hklScanHeaderString += getHklScanHeaderString(scanHeap.at(i), scanSetNumber);
                hklScanDataString += getHklScanDataString(scanHeap.at(i), scanSetNumber);
                qDebug() << hklScanDataString;
            }
        }

        ++scanSetNumber;
    }

    int hklSetNumber = scanSetNumber;

    for (int i = 0; i < scanHeap.size(); ++i) {
        if (!scanHeap.at(i).isVisible()) continue;

        if (scanHeap.at(i).pDataHkl().size()) {
            if (scanHeap.at(i).pDataAngle().size() && scanHeap.at(i).pDataIntensity().size()) {
                hklTickHeaderString += getHklTickHeaderString(scanHeap.at(i), hklSetNumber, i + 1);
                hklTickDataString += getHklTickDataString(scanHeap.at(i), hklSetNumber, scanSetNumber - 1);
                ++hklSetNumber;
            }
        }
    }

    qDebug() << QString("GraceExport::save(): Saving file");

    // write the output string to the file
    return writeFile(file, fileHeaderString
                     + scanHeaderString
                     + hklScanHeaderString
                     + hklTickHeaderString
                     + scanDataString
                     + hklScanDataString
                     + hklTickDataString);
}

void GraceExport::parseScanRanges(const QVector<Scan> &scanHeap)
{
    _colors.clear();
    _fillColors.clear();
    _names.clear();
    _xmax = 0.0;
    _xmin = 180.0;
    _ymax = 0.0;
    _ymin = 0.0;

    for (int i = 0; i < scanHeap.size(); ++i) {
        if (!scanHeap.at(i).isVisible()) continue;

        double xmax = _xmax;
        double xmin = _xmin;
        double ymax = _ymax - scanHeap.at(i).yOffset();
        double ymin = _ymin - scanHeap.at(i).yOffset();

        if (scanHeap.at(i).pDataAngle().size() && scanHeap.at(i).pDataIntensity().size()) {
            ScanOps::scanMetrics(scanHeap.at(i), xmin, xmax, ymin, ymax);
        } else {
            if (scanHeap.at(i).pDataHkl().size()) {
                for (int j = 0; j < scanHeap.at(i).pDataHkl().size(); ++j) {
                    ymax = qMax(scanHeap.at(i).pDataHkl().at(j).intensity(), ymax);
                }
            } else {
                continue;
            }
        }

        _names.append(scanHeap.at(i).name());
        _colors.append(scanHeap.at(i).color());
        _fillColors.append(settings->toFillColor(scanHeap.at(i).color()));
        _xmax = qMax(_xmax, xmax);
        _xmin = qMin(_xmin, xmin);
        _ymax = qMax(_ymax, ymax + scanHeap.at(i).yOffset());
        _ymin = qMin(_ymin, ymin + scanHeap.at(i).yOffset());
    }

    _ymax *= 1.05;
    _ymin *= 1.05;
}

QString GraceExport::getFileHeaderString(const QVector<Scan> &scanHeap)
{
    if (!scanHeap.size()) return QString();

    double yMajorTicks = getMajorTickSpacing(_ymax);

    QString xlabel(scanHeap.first().xAxisLabel());
    xlabel.replace(global::theta, "\\xq\\0");
    xlabel.replace(QRegularExpression("[tT]heta"), "\\xq\\0");
    xlabel.replace(global::degree, "\\c0\\C");

    QFileInfo fi(scanHeap.first().sourceFileName());
    QString fname = settings->value("graph/drawCompleteFileName", false).toBool() ? fi.absoluteFilePath() : fi.fileName();

    bool ok;
    QString sampleId = scanHeap.first().auxInfo("sampleId", ok).toString();
    QString titleString = sampleId.isEmpty() ? fname : QString("%1 (%2)").arg(sampleId).arg(fname);

    bool majorGrid = settings->value("graph/showMajorGridLines", false).toBool();
    bool minorGrid = settings->value("graph/showMinorGridLines", false).toBool();

    QString string = QString("# Grace project file\n");
    string += QString("# Exported by Profex\n#\n");
    string += QString("@version 50125\n");
    string += QString("@page size 841, 594\n");
    string += QString("@page scroll 5%\n");
    string += QString("@page inout 5%\n");
    string += QString("@link page off\n");
    string += QString("@map font 0 to \"Times-Roman\", \"Times-Roman\"\n");
    string += QString("@map font 1 to \"Times-Italic\", \"Times-Italic\"\n");
    string += QString("@map font 2 to \"Times-Bold\", \"Times-Bold\"\n");
    string += QString("@map font 3 to \"Times-BoldItalic\", \"Times-BoldItalic\"\n");
    string += QString("@map font 4 to \"Helvetica\", \"Helvetica\"\n");
    string += QString("@map font 5 to \"Helvetica-Oblique\", \"Helvetica-Oblique\"\n");
    string += QString("@map font 6 to \"Helvetica-Bold\", \"Helvetica-Bold\"\n");
    string += QString("@map font 7 to \"Helvetica-BoldOblique\", \"Helvetica-BoldOblique\"\n");
    string += QString("@map font 8 to \"Courier\", \"Courier\"\n");
    string += QString("@map font 9 to \"Courier-Oblique\", \"Courier-Oblique\"\n");
    string += QString("@map font 10 to \"Courier-Bold\", \"Courier-Bold\"\n");
    string += QString("@map font 11 to \"Courier-BoldOblique\", \"Courier-BoldOblique\"\n");
    string += QString("@map font 12 to \"Symbol\", \"Symbol\"\n");
    string += QString("@map font 13 to \"ZapfDingbats\", \"ZapfDingbats\"\n");
    string += QString("@map color 0 to (255, 255, 255), \"white\"\n");
    string += QString("@map color 1 to (0, 0, 0), \"black\"\n");

    int scanCols = qMin(_names.size(), _colors.size());

    for (int i = 0; i < scanCols; ++i) {
        string += QString("@map color %1 to (%2, %3, %4), \"%5\"\n")
                .arg(i + 2)
                .arg(_colors.at(i).red())
                .arg(_colors.at(i).green())
                .arg(_colors.at(i).blue())
                .arg(_names.at(i));
    }

    int fillCols = qMin(_names.size(), _fillColors.size());

    for (int i = 0; i < fillCols; ++i) {
        string += QString("@map color %1 to (%2, %3, %4), \"%5\"\n")
                .arg(scanCols + i + 2)
                .arg(_fillColors.at(i).red())
                .arg(_fillColors.at(i).green())
                .arg(_fillColors.at(i).blue())
                .arg(QString("%1-fill").arg(_names.at(i)));
    }

    // rgb corresponds to Qt::LightGray
    string += QString("@map color %1 to (174, 173, 172), \"grid lines\"\n").arg(scanCols + fillCols + 2);

    string += QString("@reference date 0\n");
    string += QString("@date wrap off\n");
    string += QString("@date wrap year 1950\n");
    string += QString("@default linewidth 1.0\n");
    string += QString("@default linestyle 1\n");
    string += QString("@default color 1\n");
    string += QString("@default pattern 1\n");
    string += QString("@default font 0\n");
    string += QString("@default char size 1.000000\n");
    string += QString("@default symbol size 1.000000\n");
    string += QString("@default sformat \"%.8g\"\n");
    string += QString("@background color 0\n");
    string += QString("@page background fill off\n");
    string += QString("@timestamp off\n");
    string += QString("@timestamp 0.03, 0.03\n");
    string += QString("@timestamp color 1\n");
    string += QString("@timestamp rot 0\n");
    string += QString("@timestamp font 0\n");
    string += QString("@timestamp char size 1.000000\n");

    QDateTime date(QDateTime::currentDateTime());
    string += QString("@timestamp def \"%1\"\n").arg(date.toString("ddd MMM dd hh:mm:ss yyyy"));

    string += QString("@r0 off\n");
    string += QString("@link r0 to g0\n");
    string += QString("@r0 type above\n");
    string += QString("@r0 linestyle 1\n");
    string += QString("@r0 linewidth 1.0\n");
    string += QString("@r0 color 1\n");
    string += QString("@r0 line 0, 0, 0, 0\n");
    string += QString("@r1 off\n");
    string += QString("@link r1 to g0\n");
    string += QString("@r1 type above\n");
    string += QString("@r1 linestyle 1\n");
    string += QString("@r1 linewidth 1.0\n");
    string += QString("@r1 color 1\n");
    string += QString("@r1 line 0, 0, 0, 0\n");
    string += QString("@r2 off\n");
    string += QString("@link r2 to g0\n");
    string += QString("@r2 type above\n");
    string += QString("@r2 linestyle 1\n");
    string += QString("@r2 linewidth 1.0\n");
    string += QString("@r2 color 1\n");
    string += QString("@r2 line 0, 0, 0, 0\n");
    string += QString("@r3 off\n");
    string += QString("@link r3 to g0\n");
    string += QString("@r3 type above\n");
    string += QString("@r3 linestyle 1\n");
    string += QString("@r3 linewidth 1.0\n");
    string += QString("@r3 color 1\n");
    string += QString("@r3 line 0, 0, 0, 0\n");
    string += QString("@r4 off\n");
    string += QString("@link r4 to g0\n");
    string += QString("@r4 type above\n");
    string += QString("@r4 linestyle 1\n");
    string += QString("@r4 linewidth 1.0\n");
    string += QString("@r4 color 1\n");
    string += QString("@r4 line 0, 0, 0, 0\n");
    string += QString("@g0 on\n");
    string += QString("@g0 hidden false\n");
    string += QString("@g0 type XY\n");
    string += QString("@g0 stacked false\n");
    string += QString("@g0 bar hgap 0.000000\n");
    string += QString("@g0 fixedpoint off\n");
    string += QString("@g0 fixedpoint type 0\n");
    string += QString("@g0 fixedpoint xy 0.000000, 0.000000\n");
    string += QString("@g0 fixedpoint format general general\n");
    string += QString("@g0 fixedpoint prec 6, 6\n");
    string += QString("@with g0\n");
    string += QString("@    world %1, %2, %3, %4\n").arg(_xmin).arg(_ymin).arg(_xmax).arg(_ymax);
    string += QString("@    stack world 0, 0, 0, 0\n");
    string += QString("@    znorm 1\n");
    string += QString("@    view 0.150000, 0.100000, 1.350000, 0.900000\n");
    string += QString("@    title \"\"\n");
    string += QString("@    title font 0\n");
    string += QString("@    title size 1.500000\n");
    string += QString("@    title color 1\n");
    string += QString("@    subtitle \"%1\"\n").arg(titleString);
    string += QString("@    subtitle font 0\n");
    string += QString("@    subtitle size 0.75\n");
    string += QString("@    subtitle color 1\n");
    string += QString("@    xaxes scale Normal\n");
    string += QString("@    yaxes scale Normal\n");
    string += QString("@    xaxes invert off\n");
    string += QString("@    yaxes invert off\n");
    string += QString("@    xaxis  on\n");
    string += QString("@    xaxis  type zero false\n");
    string += QString("@    xaxis  offset 0.000000 , 0.000000\n");
    string += QString("@    xaxis  bar on\n");
    string += QString("@    xaxis  bar color 1\n");
    string += QString("@    xaxis  bar linestyle 1\n");
    string += QString("@    xaxis  bar linewidth 1.0\n");
    string += QString("@    xaxis  label \"%1\"\n").arg(xlabel);
    string += QString("@    xaxis  label layout para\n");
    string += QString("@    xaxis  label place auto\n");
    string += QString("@    xaxis  label char size 1.000000\n");
    string += QString("@    xaxis  label font 0\n");
    string += QString("@    xaxis  label color 1\n");
    string += QString("@    xaxis  label place normal\n");
    string += QString("@    xaxis  tick on\n");
    string += QString("@    xaxis  tick major 10\n");
    string += QString("@    xaxis  tick minor ticks 9\n");
    string += QString("@    xaxis  tick default 10\n");
    string += QString("@    xaxis  tick place rounded true\n");
    string += QString("@    xaxis  tick in\n");
    string += QString("@    xaxis  tick major size 1.050000\n");
    string += QString("@    xaxis  tick major color %1\n").arg(majorGrid ? scanCols + 2 : 1);
    string += QString("@    xaxis  tick major linewidth 1.0\n");
    string += QString("@    xaxis  tick major linestyle %1\n").arg(majorGrid ? 3 : 1);
    string += QString("@    xaxis  tick major grid %1\n").arg(majorGrid ? "on" : "off");
    string += QString("@    xaxis  tick minor color %1\n").arg(minorGrid ? scanCols + 2 : 1);
    string += QString("@    xaxis  tick minor linewidth 1.0\n");
    string += QString("@    xaxis  tick minor linestyle %1\n").arg(minorGrid ? 2 : 1);
    string += QString("@    xaxis  tick minor grid %1\n").arg(minorGrid ? "on" : "off");
    string += QString("@    xaxis  tick minor size 0.500000\n");
    string += QString("@    xaxis  ticklabel on\n");
    string += QString("@    xaxis  ticklabel format general\n");
    string += QString("@    xaxis  ticklabel prec 5\n");
    string += QString("@    xaxis  ticklabel formula \"\"\n");
    string += QString("@    xaxis  ticklabel append \"\"\n");
    string += QString("@    xaxis  ticklabel prepend \"\"\n");
    string += QString("@    xaxis  ticklabel angle 0\n");
    string += QString("@    xaxis  ticklabel skip 0\n");
    string += QString("@    xaxis  ticklabel stagger 0\n");
    string += QString("@    xaxis  ticklabel place normal\n");
    string += QString("@    xaxis  ticklabel offset auto\n");
    string += QString("@    xaxis  ticklabel offset 0.000000 , 0.010000\n");
    string += QString("@    xaxis  ticklabel start type auto\n");
    string += QString("@    xaxis  ticklabel start 0.000000\n");
    string += QString("@    xaxis  ticklabel stop type auto\n");
    string += QString("@    xaxis  ticklabel stop 0.000000\n");
    string += QString("@    xaxis  ticklabel char size 0.800000\n");
    string += QString("@    xaxis  ticklabel font 0\n");
    string += QString("@    xaxis  ticklabel color 1\n");
    string += QString("@    xaxis  tick place both\n");
    string += QString("@    xaxis  tick spec type none\n");
    string += QString("@    yaxis  on\n");
    string += QString("@    yaxis  type zero false\n");
    string += QString("@    yaxis  offset 0.000000 , 0.000000\n");
    string += QString("@    yaxis  bar on\n");
    string += QString("@    yaxis  bar color 1\n");
    string += QString("@    yaxis  bar linestyle 1\n");
    string += QString("@    yaxis  bar linewidth 1.0\n");
    string += QString("@    yaxis  label \"%1\"\n").arg(scanHeap.first().yAxisLabel());
    string += QString("@    yaxis  label layout para\n");
    string += QString("@    yaxis  label place auto\n");
    string += QString("@    yaxis  label char size 1.000000\n");
    string += QString("@    yaxis  label font 0\n");
    string += QString("@    yaxis  label color 1\n");
    string += QString("@    yaxis  label place normal\n");
    string += QString("@    yaxis  tick on\n");
    string += QString("@    yaxis  tick major %2\n").arg(yMajorTicks, 0, 'f', 2);
    string += QString("@    yaxis  tick minor ticks 4\n");
    string += QString("@    yaxis  tick default 6\n");
    string += QString("@    yaxis  tick place rounded true\n");
    string += QString("@    yaxis  tick in\n");
    string += QString("@    yaxis  tick major size 1.000000\n");
    string += QString("@    yaxis  tick major color %1\n").arg(majorGrid ? scanCols + 2 : 1);
    string += QString("@    yaxis  tick major linewidth 1.0\n");
    string += QString("@    yaxis  tick major linestyle %1\n").arg(majorGrid ? 3 : 1);
    string += QString("@    yaxis  tick major grid %1\n").arg(majorGrid ? "on" : "off");
    string += QString("@    yaxis  tick minor color %1\n").arg(minorGrid ? scanCols + 2 : 1);
    string += QString("@    yaxis  tick minor linewidth 1.0\n");
    string += QString("@    yaxis  tick minor linestyle %1\n").arg(minorGrid ? 2 : 1);
    string += QString("@    yaxis  tick minor grid %1\n").arg(minorGrid ? "on" : "off");
    string += QString("@    yaxis  tick minor size 0.500000\n");
    string += QString("@    yaxis  ticklabel on\n");
    string += QString("@    yaxis  ticklabel format general\n");
    string += QString("@    yaxis  ticklabel prec 5\n");
    string += QString("@    yaxis  ticklabel formula \"\"\n");
    string += QString("@    yaxis  ticklabel append \"\"\n");
    string += QString("@    yaxis  ticklabel prepend \"\"\n");
    string += QString("@    yaxis  ticklabel angle 0\n");
    string += QString("@    yaxis  ticklabel skip 0\n");
    string += QString("@    yaxis  ticklabel stagger 0\n");
    string += QString("@    yaxis  ticklabel place normal\n");
    string += QString("@    yaxis  ticklabel offset auto\n");
    string += QString("@    yaxis  ticklabel offset 0.000000 , 0.010000\n");
    string += QString("@    yaxis  ticklabel start type auto\n");
    string += QString("@    yaxis  ticklabel start 0.000000\n");
    string += QString("@    yaxis  ticklabel stop type auto\n");
    string += QString("@    yaxis  ticklabel stop 0.000000\n");
    string += QString("@    yaxis  ticklabel char size 0.800000\n");
    string += QString("@    yaxis  ticklabel font 0\n");
    string += QString("@    yaxis  ticklabel color 1\n");
    string += QString("@    yaxis  tick place both\n");
    string += QString("@    yaxis  tick spec type none\n");
    string += QString("@    altxaxis  off\n");
    string += QString("@    altyaxis  off\n");
    string += QString("@    legend on\n");
    string += QString("@    legend loctype view\n");
    string += QString("@    legend 1.0, 0.85\n");
    string += QString("@    legend box color 1\n");
    string += QString("@    legend box pattern 1\n");
    string += QString("@    legend box linewidth 1.0\n");
    string += QString("@    legend box linestyle 1\n");
    string += QString("@    legend box fill color 0\n");
    string += QString("@    legend box fill pattern 1\n");
    string += QString("@    legend font 0\n");
    string += QString("@    legend char size 0.75\n");
    string += QString("@    legend color 1\n");
    string += QString("@    legend length 4\n");
    string += QString("@    legend vgap 1\n");
    string += QString("@    legend hgap 1\n");
    string += QString("@    legend invert false\n");
    string += QString("@    frame type 0\n");
    string += QString("@    frame linestyle 1\n");
    string += QString("@    frame linewidth 1.0\n");
    string += QString("@    frame color 1\n");
    string += QString("@    frame pattern 1\n");
    string += QString("@    frame background color 0\n");
    string += QString("@    frame background pattern 0\n");

    return string;
}

QString GraceExport::getScanHeaderString(const Scan &scan, int currentScanNumber, int totalScanNumber)
{
    QString string;
    int symbol = 0;
    if (scan.pointSymbol() == 1) symbol = 1;
    if (scan.pointSymbol() == 2) symbol = 9;

    double symbolSize = 1.0;
    if (scan.pointSymbol() == 1) symbolSize = 0.07;
    if (scan.pointSymbol() == 2) symbolSize = 0.50;

    int lineType = 1;
    if (scan.pointSymbol() > 0) lineType = 0;

    string += QString("@    s%1 hidden false\n").arg(currentScanNumber);
    string += QString("@    s%1 type xy\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol %2\n").arg(currentScanNumber).arg(symbol);
    string += QString("@    s%1 symbol size %2\n").arg(currentScanNumber).arg(symbolSize);
    string += QString("@    s%1 symbol color %2\n").arg(currentScanNumber).arg(currentScanNumber + 1);
    string += QString("@    s%1 symbol pattern 1\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol fill color %2\n").arg(currentScanNumber).arg(currentScanNumber + 1);
    string += QString("@    s%1 symbol fill pattern 0\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol linewidth 1.0\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol linestyle 1\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol char 65\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol char font 0\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol skip 0\n").arg(currentScanNumber);
    string += QString("@    s%1 line type %2\n").arg(currentScanNumber).arg(lineType);
    string += QString("@    s%1 line linestyle 1\n").arg(currentScanNumber);
    string += QString("@    s%1 line linewidth %2\n").arg(currentScanNumber).arg((scan.isActive() && _activeScansBold) ? "2.0" : "1.0");
    string += QString("@    s%1 line color %2\n").arg(currentScanNumber).arg(currentScanNumber + 1);
    string += QString("@    s%1 line pattern 1\n").arg(currentScanNumber);
    string += QString("@    s%1 baseline type 0\n").arg(currentScanNumber);
    string += QString("@    s%1 baseline %2\n").arg(currentScanNumber).arg(currentScanNumber == 1 ? "on" : "off");
    string += QString("@    s%1 dropline off\n").arg(currentScanNumber);
    string += QString("@    s%1 fill type %2\n").arg(currentScanNumber).arg((scan.isActive() && _fillActiveScans) ? "2" : "0");
    string += QString("@    s%1 fill rule 0\n").arg(currentScanNumber);
    string += QString("@    s%1 fill color %2\n").arg(currentScanNumber).arg(totalScanNumber + currentScanNumber + 1);
    string += QString("@    s%1 fill pattern 1\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue off\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue type 2\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue char size 1.000000\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue font 0\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue color %2\n").arg(currentScanNumber).arg(currentScanNumber + 1);
    string += QString("@    s%1 avalue rot 0\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue format general\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue prec 3\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue prepend \"\"\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue append \"\"\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue offset 0.000000 , 0.000000\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar off\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar place both\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar color %2\n").arg(currentScanNumber).arg(currentScanNumber + 1);
    string += QString("@    s%1 errorbar pattern 1\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar size 1.000000\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar linewidth 1.0\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar linestyle 1\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar riser linewidth 1.0\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar riser linestyle 1\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar riser clip off\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar riser clip length 0.100000\n").arg(currentScanNumber);
    string += QString("@    s%1 comment \"%2\"\n").arg(currentScanNumber).arg(scan.name());
    string += QString("@    s%1 legend \"%2\"\n").arg(currentScanNumber).arg(scan.name());

    return string;
}

QString GraceExport::getHklTickHeaderString(const Scan &scan, int currentScanNumber, int colorNumber)
{
    QString string;

    string += QString("@    s%1 hidden false\n").arg(currentScanNumber);
    string += QString("@    s%1 type xy\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol 11\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol size 0.75\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol color %2\n").arg(currentScanNumber).arg(colorNumber + 1);
    string += QString("@    s%1 symbol pattern 1\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol fill color %2\n").arg(currentScanNumber).arg(colorNumber + 1);
    string += QString("@    s%1 symbol fill pattern 0\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol linewidth 1.0\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol linestyle 1\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol char 124\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol char font 0\n").arg(currentScanNumber);
    string += QString("@    s%1 symbol skip 0\n").arg(currentScanNumber);
    string += QString("@    s%1 line type 1\n").arg(currentScanNumber);
    string += QString("@    s%1 line linestyle 0\n").arg(currentScanNumber);
    string += QString("@    s%1 line linewidth 1.0\n").arg(currentScanNumber);
    string += QString("@    s%1 line color %2\n").arg(currentScanNumber).arg(colorNumber + 1);
    string += QString("@    s%1 line pattern 1\n").arg(currentScanNumber);
    string += QString("@    s%1 baseline type 0\n").arg(currentScanNumber);
    string += QString("@    s%1 baseline off\n").arg(currentScanNumber);
    string += QString("@    s%1 dropline off\n").arg(currentScanNumber);
    string += QString("@    s%1 fill type 0\n").arg(currentScanNumber);
    string += QString("@    s%1 fill rule 0\n").arg(currentScanNumber);
    string += QString("@    s%1 fill color %2\n").arg(currentScanNumber).arg(colorNumber + 1);
    string += QString("@    s%1 fill pattern 1\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue off\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue type 2\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue char size 1.000000\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue font 0\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue color %2\n").arg(currentScanNumber).arg(colorNumber + 1);
    string += QString("@    s%1 avalue rot 0\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue format general\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue prec 3\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue prepend \"\"\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue append \"\"\n").arg(currentScanNumber);
    string += QString("@    s%1 avalue offset 0.000000 , 0.000000\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar on\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar place both\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar color %2\n").arg(currentScanNumber).arg(colorNumber + 1);
    string += QString("@    s%1 errorbar pattern 1\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar size 1.000000\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar linewidth 1.0\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar linestyle 1\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar riser linewidth 1.0\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar riser linestyle 1\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar riser clip off\n").arg(currentScanNumber);
    string += QString("@    s%1 errorbar riser clip length 0.100000\n").arg(currentScanNumber);
    string += QString("@    s%1 comment \"%2 hkl\"\n").arg(currentScanNumber).arg(scan.name());
    string += QString("@    s%1 legend  \"\"\n").arg(currentScanNumber);

    return string;
}


QString GraceExport::getHklScanHeaderString(const Scan &scan, int scanNumber)
{
    QString string;
    double symbolSize = 0.25;

    string += QString("@    s%1 hidden false\n").arg(scanNumber);
    string += QString("@    s%1 type bardy\n").arg(scanNumber);
    string += QString("@    s%1 symbol 0\n").arg(scanNumber);
    string += QString("@    s%1 symbol size %2\n").arg(scanNumber).arg(0.0);
    string += QString("@    s%1 symbol color %2\n").arg(scanNumber).arg(scanNumber + 1);
    string += QString("@    s%1 symbol pattern 1\n").arg(scanNumber);
    string += QString("@    s%1 symbol fill color %2\n").arg(scanNumber).arg(scanNumber + 1);
    string += QString("@    s%1 symbol fill pattern 0\n").arg(scanNumber);
    string += QString("@    s%1 symbol linewidth 1.0\n").arg(scanNumber);
    string += QString("@    s%1 symbol linestyle 1\n").arg(scanNumber);
    string += QString("@    s%1 symbol char 124\n").arg(scanNumber);
    string += QString("@    s%1 symbol char font 0\n").arg(scanNumber);
    string += QString("@    s%1 symbol skip 0\n").arg(scanNumber);
    string += QString("@    s%1 line type 0\n").arg(scanNumber);
    string += QString("@    s%1 line linestyle 1\n").arg(scanNumber);
    string += QString("@    s%1 line linewidth 1.0\n").arg(scanNumber);
    string += QString("@    s%1 line color %2\n").arg(scanNumber).arg(scanNumber + 1);
    string += QString("@    s%1 line pattern 1\n").arg(scanNumber);
    string += QString("@    s%1 baseline type 1\n").arg(scanNumber);
    string += QString("@    s%1 baseline %2\n").arg(scanNumber).arg(scanNumber == 0 ? "on" : "off");
    string += QString("@    s%1 dropline on\n").arg(scanNumber);
    string += QString("@    s%1 fill type 0\n").arg(scanNumber);
    string += QString("@    s%1 fill rule 0\n").arg(scanNumber);
    string += QString("@    s%1 fill color %2\n").arg(scanNumber).arg(scanNumber + 1);
    string += QString("@    s%1 fill pattern 1\n").arg(scanNumber);
    string += QString("@    s%1 avalue off\n").arg(scanNumber);
    string += QString("@    s%1 avalue type 2\n").arg(scanNumber);
    string += QString("@    s%1 avalue char size 1.000000\n").arg(scanNumber);
    string += QString("@    s%1 avalue font 0\n").arg(scanNumber);
    string += QString("@    s%1 avalue color %2\n").arg(scanNumber).arg(scanNumber + 1);
    string += QString("@    s%1 avalue rot 0\n").arg(scanNumber);
    string += QString("@    s%1 avalue format general\n").arg(scanNumber);
    string += QString("@    s%1 avalue prec 3\n").arg(scanNumber);
    string += QString("@    s%1 avalue prepend \"\"\n").arg(scanNumber);
    string += QString("@    s%1 avalue append \"\"\n").arg(scanNumber);
    string += QString("@    s%1 avalue offset 0.000000 , 0.000000\n").arg(scanNumber);
    string += QString("@    s%1 errorbar %2\n").arg(scanNumber).arg(_hklSymbol > 0 ? "on" : "off");
    string += QString("@    s%1 errorbar place normal\n").arg(scanNumber);
    string += QString("@    s%1 errorbar color %2\n").arg(scanNumber).arg(scanNumber + 1);
    string += QString("@    s%1 errorbar pattern 1\n").arg(scanNumber);
    string += QString("@    s%1 errorbar size %2\n").arg(scanNumber).arg(symbolSize, 0, 'f', 5);
    string += QString("@    s%1 errorbar linewidth 1.0\n").arg(scanNumber);
    string += QString("@    s%1 errorbar linestyle 1\n").arg(scanNumber);
    string += QString("@    s%1 errorbar riser linewidth 1.0\n").arg(scanNumber);
    string += QString("@    s%1 errorbar riser linestyle 1\n").arg(scanNumber);
    string += QString("@    s%1 errorbar riser clip off\n").arg(scanNumber);
    string += QString("@    s%1 errorbar riser clip length 0.100000\n").arg(scanNumber);
    string += QString("@    s%1 comment \"\"\n").arg(scanNumber);
    string += QString("@    s%1 legend \"%2\"\n").arg(scanNumber).arg(scan.name());

    return string;
}

QString GraceExport::getScanDataString(const Scan &scan, int scanNumber)
{
    double offset = scan.yOffset();

    QString string = QString("@target G0.S%1\n").arg(scanNumber);
    string += QString("@type xy\n");

    for (int i = 0; i < qMin(scan.pDataAngle().size(), scan.pDataIntensity().size()); ++i) {
        string += QString("%1 %2\n").arg(scan.angle(i), 0, 'f', 6).arg(scan.intensity(i) + offset, 0, 'f', 6);
    }

    string += QString("&\n");
    return string;
}

QString GraceExport::getHklTickDataString(const Scan &scan, int setNumber, int phaseNumber)
{
    QString string = QString("@target G0.S%1\n").arg(setNumber);
    string += QString("@type xy\n");
    bool d2tt = scan.getHklXunit() == "dnm" ? true : false;
    double wl = scan.waveLength();

    for (int i = 0; i < scan.pDataHkl().size(); ++i) {
        double xpos = scan.pDataHkl().at(i).position();
        double ypos = (1.0 - double(setNumber - phaseNumber) * 0.05) * _ymax;

        if (d2tt) xpos = asin(wl / (20.0 * xpos)) * 360.0 / M_PI;

        string += QString("%1 %2 \"%3\"\n").arg(xpos, 0, 'f', 6)
                                           .arg(ypos, 0, 'f', 6)
                                           .arg(scan.pDataHkl().at(i).hkl());
    }

    return string;
}

QString GraceExport::getHklScanDataString(const Scan &scan, int scanNumber)
{
    double offset = scan.yOffset();
    bool d2tt = scan.getHklXunit() == "dnm" ? true : false;
    double wl = scan.waveLength();

    QString string = QString("@target G0.S%1\n").arg(scanNumber);
    string += QString("@type bardy\n");

    for (int i = 0; i < scan.pDataHkl().size(); ++i) {
        double xpos = scan.pDataHkl().at(i).position();
        double ypos = scan.pDataHkl().at(i).intensity() + offset;

        if (d2tt) xpos = asin(wl / (20.0 * xpos)) * 360.0 / M_PI;

        string += QString("%1 %2 %3\n").arg(xpos, 0, 'f', 6).arg(0.99*ypos, 0, 'f', 6).arg(0.01*ypos, 0, 'f', 6);
    }

    string += QString("&\n");
    return string;
}

double GraceExport::getMajorTickSpacing(double m)
{
    double l = log10(m);
    double r = l - floor(l);

    if (r < 0.3) return (pow(10.0, floor(l))) / 5.0;
    if (r < 0.7) return (pow(10.0, floor(l))) / 2.0;
    return pow(10.0, floor(l));
}

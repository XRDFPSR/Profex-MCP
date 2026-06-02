/***************************************************************************
                          gnuplotexport.cpp  -  description
                             -------------------
    begin                : Tue Apr 16 20:16:07 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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


#include "gnuplotexport.h"
#include "functions.h"

GnuPlotExport::GnuPlotExport(QObject *parent) :
    GenericExport(parent)
{
    uId = "GNUPLOT_GPL";
    settings = SettingsManager::getInstance();
}

int GnuPlotExport::save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &)
{
    QVector<Scan> vec;
    vec.push_back(scan);
    return save(file, vec);
}

int GnuPlotExport::save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &)
{
    qDebug() << QString("GnuPlotExport::save(): Writing %1 files to %2").arg(scanHeap.size()).arg(file);

    // check if there is any data to export
    if (!scanHeap.size()) {
        qDebug() << QString("GnuPlotExport::save(): No scans to export");
        return 0;
    }

    // check if the file name makes sense (not checking for write permissions etc.)
    if (file.isEmpty()) {
        qDebug() << QString("GnuPlotExport::save(): No valid file name provided");
        return 0;
    }

    QMap<uint, QString> symbols;
    symbols[0] = QString("lines");
    symbols[1] = QString("dots");
    symbols[2] = QString("points");

    // initialize angle boundaries
    double start = scanHeap.first().startAngle();
    double end = scanHeap.first().endAngle();

    // lists to store hkl and line styles
    QStringList hklStyles;
    QStringList scanStyles;

    // determine the y axis scaling
    // 0 = linear
    // 1 = sqrt
    // 2 = log10
    int yAxScaling = settings->value("graph/yAxisScale", 0).toInt();

    // determine hkl and line styles and angular boundaries
    for (int i = 0; i < (int)scanHeap.size(); ++i) {
        start = qMin(start, scanHeap[i].startAngle());
        end = qMax(end, scanHeap[i].endAngle());

        hklStyles.append(QString("set style arrow %1 lc rgb '%2' lw 1 nohead").arg(i+1).arg(scanHeap[i].colorName()));
        scanStyles.append(QString("set style line %1 lc rgb '%2' lw 1 lt 1 pt 2").arg(i+1).arg(scanHeap[i].colorName()));
    }

    QString output;
    output.append(QString("# Exported by Profex\n"));
    output.append(QString("_xStart=%1\n").arg(start, 0, 'f', 4));
    output.append(QString("_xEnd=%1\n").arg(end, 0, 'f', 4));
    output.append(getFileHeaderString(scanHeap));

    if (yAxScaling == 0) output.append(QString("set ylabel \"Intensity [cts]\"\n"));
    if (yAxScaling == 1) output.append(QString("set ylabel \"Intensity [SQRT(cts)]\"\n"));
    if (yAxScaling == 2) output.append(QString("set ylabel \"Intensity [Log10(cts)]\"\n"));

    output.append("set xrange[_xStart:_xEnd]\n");

    output.append(QString("\n# grid lines\n"));
    if (settings->value("graph/showMajorGridLines", false).toBool()) {
        output.append(QString("set grid xtics lt 2 lw 1 lc rgb '#aeadac'\n"));
        output.append(QString("set grid ytics lt 2 lw 1 lc rgb '#aeadac'\n"));
    }

    if (settings->value("graph/showMinorGridLines", false).toBool()) {
        output.append(QString("set grid mxtics lt 3 lw 1 lc rgb '#aeadac'\n"));
        output.append(QString("set grid mytics lt 3 lw 1 lc rgb '#aeadac'\n"));
    }

    output.append(QString("\n# line styles\n"));
    output.append(QString("set style arrow 99 lc rgb '#000000' lt 3 lw 1 nohead\n\n"));

    output.append(hklStyles.join("\n"));
    output.append(QString("\n"));
    output.append(scanStyles.join("\n"));
    output.append("\nset style fill transparent solid 0.5 \n");
    // output.append("\nset boxwidth 0\n");  // if set to 0, whiskerbars for hkl lines don't work anymore

    output.append(QString("\n# horizontal line at 0\n"));
    output.append(QString("set arrow from _xStart,0 to _xEnd,0 as 99\n\n"));

    output.append(QString("top = 0.96\n"));
    output.append(QString("length = 0.03\n"));
    output.append(QString("spacing = 0.01\n\n"));

    output.append(QString("# hkl tick marks\n"));

    // insert hkl tick marks at the top (not hkl scans with lines from the bottom)
    for (int i = 0; i < scanHeap.size(); ++i) {
        output.append(getHklTickInlineString(scanHeap.at(i), start, end, i));
    }

    // compose the plot line
    output.append(QString("# Plot\n"));

    QString scanBlock;
    QString hklBlock;
    QString prefix("plot '-' using ");

    for (int i = 0; i < scanHeap.size(); ++i) {
        if (!scanHeap[i].isVisible()) continue;

        double doffset = scanHeap[i].yOffset();
        if (yAxScaling == 1) doffset = doffset < 0.0 ? -sqrt(-doffset) : sqrt(doffset);
        if (yAxScaling == 2) doffset = doffset < 1.0 ? doffset < -1.0 ? -log10(-doffset) : 0.0 : log10(doffset);

        QString symbol = symbols.value(scanHeap.at(i).pointSymbol(), "lines");
        if (scanHeap.at(i).isActive()) symbol = "filledcurves y1=0";

        if (scanHeap.at(i).hasScanData()) {
            scanBlock.append(prefix + getScanPlotLine(scanHeap[i].name(), symbol, doffset, i+1));
            prefix = ", \\\n '' using ";
        } else if (scanHeap.at(i).hasHklData()) {
            hklBlock.append(prefix + getHklPlotLine(scanHeap[i].name(), doffset, i+1));
            prefix = ", \\\n '' using ";
        }
    }

    output.append(scanBlock);
    output.append(hklBlock);
    output.append(QString("\n\n#Inline data\n"));

    scanBlock.clear();
    hklBlock.clear();

    // loop over scans and append inlined data
    // scans are separated by empty lines
    for (int i = 0; i < (int)scanHeap.size(); ++i) {
        if (!scanHeap[i].isVisible()) continue;

        if (scanHeap.at(i).hasScanData()) {
            scanBlock.append(QString("# %1\n").arg(scanHeap[i].name()));
            scanBlock.append(getScanDataString(scanHeap[i], yAxScaling) + "EOF\n");
        } else if (scanHeap.at(i).hasHklData()) {
            hklBlock.append(QString("# %1\n").arg(scanHeap[i].name()));
            hklBlock.append(getHklDataString(scanHeap[i], yAxScaling) + "EOF\n");
        }
    }

    output.append(scanBlock);
    output.append(hklBlock);

    // write the output string to the file
    return writeFile(file, output);
}

int GnuPlotExport::getBackgroundScanIndex(const QVector<Scan> &scanHeap)
{
    for (int i = 0; i < scanHeap.size(); ++i) {
        if (scanHeap.at(i).scanTypes().testFlag(Scan::BACKGROUND)) return i;
    }

    return -1;
}

QString GnuPlotExport::getFileHeaderString(const QVector<Scan> &s)
{
    QString output;
    QFileInfo fi(s.first().sourceFileName());
    QString fname = settings->value("graph/drawCompleteFileName", false).toBool() ? fi.absoluteFilePath() : fi.fileName();

    bool ok;
    QString sampleId = s.first().auxInfo("sampleId", ok).toString();
    QString titleString = sampleId.isEmpty() ? fname : QString("%1 (%2)").arg(sampleId).arg(fname);

    // Composing the file header
    output.append(QString("reset\n"));
    output.append(QString("set termoption dashed\n"));
    output.append(QString("set key opaque\n"));

    output.append(QString("set autoscale                          # scale axes automatically\n"));
    output.append(QString("set xtic auto                          # set xtics automatically\n"));
    output.append(QString("set ytic auto                          # set ytics automatically\n"));
    output.append(QString("set mxtic 10                           # set minor xtics\n"));
    output.append(QString("set mytic 5                            # set minor ytics\n"));
    output.append(QString("set title \"%1\"\n").arg(titleString));
    output.append(QString("set xlabel \"Diffraction Angle [°2theta]\"\n"));
    return output;
}

QString GnuPlotExport::getHklTickInlineString(const Scan &s, double _xStart, double _xEnd, int i)
{
    if (!s.hasHklData())  return QString();
    if (!s.hasScanData()) return QString();
    if (!s.isVisible())   return QString();

    double wl = 0.1 * s.waveLength();
    qDebug() << QString("GnuPlotExport::getHklTickInlineString(): Using wavelength = %1 "
                        "to convert hkl tick marks from d to 2theta").arg(wl);

    QString output;

    for (int j = 0; j < s.pDataHkl().size(); ++j) {
        double pos = global::Functions::dToTwoTheta(s.pDataHkl().at(j).position(), wl);

        // careful with using 'break' in the following checks. We might be iterating from end to start in 2theta
        if (pos < _xStart)    continue;
        else if (pos > _xEnd) continue;

        output.append(QString("if (%1 > _xStart && %1 < _xEnd) "
                              "{set arrow from first %1,graph top "
                              "to first %1,graph top - length as %2}\n")
                      .arg(pos)
                      .arg(i+1));
    }

    // increment the vertical position for the next scan
    output.append(QString("\ntop = top - (length + spacing)\n\n"));
    return output;
}

QString GnuPlotExport::getScanDataString(const Scan &s, int scale)
{
    QString output;
    double xOffset = s.xOffset();
    double yScale  = s.scaleFactor();
    int size = qMin(s.pDataAngle().size(), s.pDataIntensity().size());

    for (int i = 0; i < size; ++i) {
        double x = s.pDataAngle().at(i) + xOffset;
        double y = s.pDataIntensity().at(i) * yScale;

        if (scale == 1) y = y < 0.0 ? -sqrt(-y) : sqrt(y);
        if (scale == 2) y = y < 1.0 ? y < -1.0 ? -log10(-y) : 0.0 : log10(y);

        output.append(QString("%1 %2\n").arg(x).arg(y));
    }

    return output;
}

QString GnuPlotExport::getHklDataString(const Scan &s, int scale)
{
    QString output;
    double xOffset = s.xOffset();
    double yScale  = s.scaleFactor();
    bool d2tt = s.getHklXunit() == "dnm" ? true : false;
    double wl = s.waveLength();

    for (int i = 0; i < s.pDataHkl().size(); ++i) {
        double x = s.pDataHkl().at(i).position() + xOffset;
        double y = s.pDataHkl().at(i).intensity() * yScale;

        if (d2tt) x = asin(wl / (20.0 * x)) * 360.0 / M_PI;

        if (scale == 1) y = y < 0.0 ? -sqrt(-y) : sqrt(y);
        if (scale == 2) y = y < 1.0 ? y < -1.0 ? -log10(-y) : 0.0 : log10(y);

        output.append(QString("%1 %2\n").arg(x).arg(y));
    }

    return output;
}

QString GnuPlotExport::getScanPlotLine(const Scan &scan, const QString &symbol, double offset, int i)
{
    QString output;
    QString op = offset < 0.0 ? QString("%1").arg(offset, 0, 'f', 4) : QString("+%1").arg(offset, 0, 'f', 4);
    if (qFuzzyIsNull(offset)) op = QString();

    QString columns = QString("1:($2%1)").arg(op);

    QString whiskerbars;

    output.append(QString("%1 title '%2' with %3 ls %4")
                  .arg(columns)
                  .arg(scan.name())
                  .arg(symbol)
                  .arg(i));

    return output;
}

QString GnuPlotExport::getHklPlotLine(const Scan &scan, double offset, int i)
{
    QString output;
    QString op;

    if (!qFuzzyIsNull(offset)) {
        op = QString("%1").arg(offset, 0, 'f', 4);
        if (offset > 0.0) op = "+" + op;
    }

    bool hasTipSymbols = settings->value("config/hklSymbol", 0).toInt() > 0;
    QString columns = QString("1:(%1):(%1):($2%2):(%1)").arg(offset, 0, 'f', 4).arg(op);

    output.append(QString("%1 title '%2' with %3 %4 ls %5")
                  .arg(columns)
                  .arg(scan.name())
                  .arg("candlesticks")
                  .arg(hasTipSymbols ? "whiskerbars" : QString())
                  .arg(i));

    return output;
}

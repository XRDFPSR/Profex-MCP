/***************************************************************************
                          graphtosvg.cpp  -  description
                             -------------------
    begin                : Tue Sep 04 19:42:15 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#include "graphtosvg.h"
#include "structs.h"
#include <QtSvg/QSvgGenerator>
#include <QFontInfo>
#include <QPainter>
#include <QString>
#include <QDebug>
#include <QtMath>

GraphToSvg::GraphToSvg(const QVector<Scan> &_sheap, double _eps1, double _eps2, double _eps3)
    : scanHeap(_sheap), eps1(_eps1), eps2(_eps2), eps3(_eps3)
{
    settings = SettingsManager::getInstance();
    yScaling = YSCALELIN;
    minAng = std::numeric_limits<double>::max();
    maxAng = 0.0;
    minIntens = std::numeric_limits<double>::max();
    maxIntens = 0.0;
    wavelength = 0.0;

    for (int i = 0; i < scanHeap.size(); ++i) {
        minAng = qMin(minAng, scanHeap.at(i).minAngle());
        maxAng = qMax(maxAng, scanHeap.at(i).maxAngle());
        minIntens = qMin(minIntens, scanHeap.at(i).minIntensity() + scanHeap.at(i).yOffset());
        maxIntens = qMax(maxIntens, scanHeap.at(i).maxIntensity() + scanHeap.at(i).yOffset());
    }

    if (minIntens < 0.0) minIntens *= 1.05;
    maxIntens *= 1.05;

    useCountsPerSecond = settings->value("graph/countsPerSecond", false).toBool();
    showGridMinorX = settings->value("graph/showMinorGridLinesX", false).toBool();
    showGridMajorX = settings->value("graph/showMajorGridLinesX", false).toBool();
    showGridMinorY = settings->value("graph/showMinorGridLinesY", false).toBool();
    showGridMajorY = settings->value("graph/showMajorGridLinesY", false).toBool();

    tickDensityX = settings->value("graph/tickMarkDensityX", 10).toInt();
    tickDensityY = settings->value("graph/tickMarkDensityY", 5).toInt();
    tickDensityD = settings->value("graph/tickMarkDensityD", 2).toInt();

    drawDifferenceTickLabels = settings->value("graph/differenceTickLabels", false).toBool();
    updateScanColors(false);
    updateScanStyles(false);
}

QByteArray GraphToSvg::getSvg(double ar)
{
    QBuffer buf;
    buf.open(QBuffer::ReadWrite);
    renderSvg(&buf, ar);

    buf.seek(0);
    QByteArray data = buf.readAll();

    buf.close();
    return data;
}

double GraphToSvg::getWavelength()
{
    return -1.0;
}

void GraphToSvg::renderSvg(QBuffer *buf, double a)
{
    QSvgGenerator svg;
    svg.setOutputDevice(buf);
    svg.setTitle("Sample ID");

    double scaleFac = settings->value("graph/svgScaleFactor", 1.0).toDouble();
    scaleFac = 1.25;

    // using an aspect ratio of sqrt(2) : 1
    double w = scaleFac * 27.0 * double(svg.resolution()) / 2.54;
    double h = w / (qFuzzyIsNull(a) ? sqrt(float(2.0)) : a);
    QRectF pageRect(0.0, 0.0, w, h);

    svg.setSize(QSize(int(w), int(h)));
    svg.setViewBox(pageRect);

    QFont defaultFont("Sans Serif", 10);

    pScale = settings->value("graph/svgExportFontSize", 1.0).toDouble(); // smaller values reduce font size
    fontTitle  = QFont(settings->value("graph/fontTitle", defaultFont.toString()).toString());
    fontAxis   = QFont(settings->value("graph/fontAxis", defaultFont.toString()).toString());
    fontTicks  = QFont(settings->value("graph/fontTicks", defaultFont.toString()).toString());
    fontLegend = QFont(settings->value("graph/fontLegend", defaultFont.toString()).toString());
    lineWidth = settings->value("graph/lineWidth", 1).toInt();
    symbolSize = settings->value("graph/crossSize", 1).toInt();

    QFontInfo fiTitle(fontTitle);
    QFontInfo fiAxis(fontAxis);
    QFontInfo fiTicks(fontTicks);
    QFontInfo fiLegend(fontLegend);

    fontTitle.setPixelSize(int(pScale * fiTitle.pixelSize()));
    fontAxis.setPixelSize(int(pScale * fiAxis.pixelSize()));
    fontTicks.setPixelSize(int(pScale * fiTicks.pixelSize()));
    fontLegend.setPixelSize(int(pScale * fiLegend.pixelSize()));

    QPainter painter;
    painter.begin(&svg);
    calcMargins(painter, false);

    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, true);
    initPlot(pageRect.toRect());
    drawWindow(painter, true);
    drawPlot(painter);
    drawHklTicks(painter);
    drawTicks(painter);
    drawLabels(painter);
    drawLegend(painter);
    painter.end();
}

void GraphToSvg::calcMargins(const QPainter &, bool border)
{
    QFontMetrics fmTitle(fontTitle);
    QFontMetrics fmAxis(fontAxis);
    QFontMetrics fmTicks(fontTicks);

    // add an outer margin (e.g. on screen, but not on printing or rendering)
    if (border) {
        margin_o = fmAxis.height() / 2;
    } else {
        margin_o = 0;
    }

    margin_m = fmAxis.height() / 2;
    margin_i = fmTicks.height() / 2;

    QString stra = QString("%1").arg(maxIntens, 0, 'f', 0);
    int spc = margin_o + margin_i;
    int vTickLabelLength = qMax(fmTicks.horizontalAdvance(stra), fmTicks.horizontalAdvance("000000"));
    int spr = int(fmTicks.horizontalAdvance("MMM.MM") / 2.0);

    margin_l = fmAxis.height() + vTickLabelLength + margin_m + spc;
    margin_r = spr + margin_o;
    margin_t = fmTitle.height() + spc;
    margin_b = fmAxis.height() + fmTicks.height() + margin_m + spc;
}

void GraphToSvg::initPlot(const QRect &rc)
{
    canvas = rc;
    plot.setRect(margin_l, margin_t, rc.width() - (margin_r + margin_l), rc.height() - (margin_t + margin_b));
    qDebug() << QString("GraphToSvg::calcMargins(): Plot rect (x, y, w, h): %1, %2, %3, %4").arg(plot.x()).arg(plot.y()).arg(plot.width()).arg(plot.height());
}

void GraphToSvg::drawWindow(QPainter &p, bool replaceChars)
{
    if (!p.isActive()) return;
    p.save();

    QPen pen;
    QString xa;
    QString ya;

    if (xScaling == XSCALED) {
        if (replaceChars) {
            xa = "d Spacing [Angstrom]";
        } else {
            xa = "d Spacing [" + QString(global::angstrom) + "]";
        }
    } else if (xScaling == XSCALEQ) {
        if (replaceChars) {
            xa = "Q [1/Angstrom]";
        } else {
            xa = "Q [" + QString(global::angstrom) + QString(global::superMinus) + QString(global::superOne) + "]";
        }
    } else {
        if (replaceChars) {
            xa = "Diffraction Angle [degrees 2theta]";
        } else {
            xa = "Diffraction Angle [" + QString(global::degree) + "2" + QString(global::theta) + "]";
        }
    }

    if (scanHeap.first().timePerStep() > 0.0) {
        ya = QString("Intensity [cps]");
    } else {
        ya = QString("Intensity [counts]");
    }

    pen.setWidth(lineWidth);
    pen.setColor(Qt::black);

    QFontMetrics fma(fontAxis);

    p.setPen(pen);
    p.setFont(fontTitle);
    /*
    p.drawText(margin_o, fmt.ascent() + margin_o, scanControl->getSampleId());

    QString displayName = settings->value("graph/drawCompleteFileName", false).toBool()
                              ? QDir::toNativeSeparators(scanControl->fileInfo().absoluteFilePath())
                              : scanControl->fileInfo().fileName();

    p.drawText(canvas.width() - margin_o - fmt.horizontalAdvance(displayName), fmt.ascent() + margin_o, displayName);
    */

    // x-axis
    p.setFont(fontAxis);
    p.drawText(plot.center().x() - fma.horizontalAdvance(xa) / 2, canvas.height() - margin_o - (fma.descent() + 2), xa);

    // y-axis
    int dx = fma.ascent() + margin_o;
    int dy = plot.center().y() + fma.horizontalAdvance(ya) / 2;

    p.translate(dx, dy);
    p.rotate(-90);
    p.drawText(0, 0, ya);
    p.restore();
}

void GraphToSvg::drawPlot(QPainter &p)
{
    if (!p.isActive()) return;
    p.save();

    int aLineWidth = settings->value("graph/useActiveLineWidth", true).toBool()
                         ? settings->value("graph/activeLineWidth", 2).toInt()
                         : lineWidth;

    bool drawPhaseScans = settings->value("graph/phaseVisibilityPattern", true).toBool();

    QPolygon bgScreenPolygon;

    // reset the offset value for the difference curve
    difOffset = 0.0;

    QPen pen;
    pen.setWidth(lineWidth);
    pen.setColor(Qt::black);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::RoundJoin);

    QBrush brush(QColor(255, 255, 255));

    p.setPen(pen);
    p.setBrush(brush);
    p.translate(plot.x(), plot.y());

    if (qFuzzyIsNull(wavelength)) {
        wavelength = settings->defaultWavelength();
    }

    // now clip the drawing rect, so as not to print over the axes
    p.setClipRect(0, 0, plot.width(), plot.height());

    // loop over all scans
    for (int ix = 0; ix < scanHeap.count(); ++ix) {
        Scan scan = scanHeap.at(ix);

        // skip invisible scans
        if (!scan.isVisible()) continue;

        // skip scans of size 0
        if (!scan.size()) continue;

        pen.setColor(scan.color());

        bool isPhase = scan.scanTypes().testFlag(Scan::PHASE);

        // drawing of phase scans not requested
        if (isPhase && !drawPhaseScans) continue;

        // don't convert the background scan to screen again if we already have done it
        QPolygon polygon = getScanScreenPolygon(scan, true);

        // only draw the scan if the polygon contains at least 2 points
        if (polygon.size() > 2) {
            // check if this is the active scan. If yes, use double line width
            int cLineWidth = scan.isActive() ? aLineWidth : lineWidth;
            int sstyle = (int)scan.pointSymbol();

            // draw a solid line
            if (sstyle <= 0 || sstyle == 4 || sstyle == 5) {
                pen.setWidth(cLineWidth);
                pen.setStyle(Qt::SolidLine);
                p.setPen(pen);
                p.drawPolyline(polygon);
            }

            // draw points
            if (sstyle == 1 || sstyle == 4 || sstyle == 6) {
                pen.setWidth(cLineWidth * symbolSize);
                pen.setStyle(Qt::SolidLine);
                p.setPen(pen);
                p.drawPoints(polygon);
            }

            // draw crosses
            if (sstyle == 2 || sstyle == 5 || sstyle == 7) {
                pen.setWidth(cLineWidth);
                pen.setStyle(Qt::SolidLine);
                p.setPen(pen);
                double sz = symbolSize * lineWidth + 1.0;
                for (int pp = 0; pp < polygon.size(); ++pp) {
                    p.drawLine(polygon.at(pp).x() - sz, polygon.at(pp).y() - sz,
                               polygon.at(pp).x() + sz, polygon.at(pp).y() + sz);
                    p.drawLine(polygon.at(pp).x() - sz, polygon.at(pp).y() + sz,
                               polygon.at(pp).x() + sz, polygon.at(pp).y() - sz);
                }
            }

            // draw a dash line
            if (sstyle == 3 || sstyle == 6 || sstyle == 7) {
                pen.setWidth(cLineWidth);
                pen.setStyle(Qt::DashLine);
                p.setPen(pen);
                p.drawPolyline(polygon);
            }
        }

        difOffset = qMin(scan.yOffset(), difOffset);
    }

    // draw a horizontal line at yOffset if yOffset is < 0. This is only used to draw a line
    // at the center of the difference curve
    if (difOffset < 0.0) {
        QPen hlinePen(Qt::black, lineWidth, Qt::DashLine);
        p.setPen(hlinePen);

        double yCenterLine = getYcoord(yValueToScale(difOffset));
        p.drawLine(0, yCenterLine, plot.width(), yCenterLine);
    }

    // draw axes
    pen.setColor(Qt::black);
    pen.setWidth(lineWidth);
    p.setPen(pen);
    p.setBrush(QBrush());
    // increase the clipped rect
    p.setClipRect(-lineWidth, -lineWidth, plot.width() + 2*lineWidth, plot.height() + 2*lineWidth);
    p.drawRect(0, 0, plot.width() , plot.height());

    p.restore();
}

QPolygon GraphToSvg::getScanScreenPolygon(const Scan &scan, bool clipManually)
{
    QPolygon polygon;

    double yoff = scan.yOffset();
    double tps = 1.0;

    if (useCountsPerSecond) {
        tps = scan.timePerStep() > 1e-7 ? scan.timePerStep() : 1.0;
    }

    if (yScaling == YSCALELOG10) yoff = getLog10(yoff);
    if (yScaling == YSCALESQRT)  yoff = getSqrt(yoff);

    int firstIdx = 0;
    int lastIdx = scan.size() - 1;

    double leftAng = getRealX(plot.left());
    double rightAng = getRealX(plot.right());

    // loop from the left to determine the start of the visible range
    for (int i = 0; i < scan.size(); ++i) {
        if (scan.angle(i) - angularCorrection(scan.angle(i)) + scan.xOffset() >= leftAng) {
            break;
        }

        firstIdx = i;
    }

    // loop from the right to determine the end of the visible range
    for (int i = scan.size() - 1; i > firstIdx; --i) {
        if (scan.angle(i) - angularCorrection(scan.angle(i)) + scan.xOffset() <= rightAng) {
            break;
        }

        lastIdx = i;
    }

    // loop over the visible range and calculate screen coordinates
    for (int i = firstIdx; i <= lastIdx; ++i) {
        // calculate intensity
        double scint = scan.intensity(i) * scan.scaleFactor() / tps;

        if (yScaling == YSCALESQRT)  scint = getSqrt(scint);
        if (yScaling == YSCALELOG10) scint = getLog10(scint);

        // calculate screen coordinates
        int x = getXcoord(scan.angle(i) + scan.xOffset() - angularCorrection(scan.angle(i)));
        int y = getYcoord(scint + yoff);

        if (clipManually) {
            if (y < 0)                  y = 0;
            else if (y > plot.height()) y = plot.height();
        }

        polygon << QPoint(x, y);
    }

    if (polygon.size() >= 2) {
        if (polygon.at(0).x() < 0) {
            polygon[0] = interpolate(polygon.at(0), polygon.at(1), 0);
        }

        if (polygon.last().x() > plot.width()) {
            polygon[polygon.size() - 1] = interpolate(polygon.at(polygon.size() - 2), polygon.last(), plot.width());
        }
    }

    return polygon;
}

double GraphToSvg::yValueToScale(double d) const
{
    double s;

    switch (yScaling) {
    case YSCALESQRT:
        s = getSqrt(d);
        break;
    case YSCALELOG10:
        s = getLog10(d);
        break;
    default:
        s = d;
    }

    return s;
}

int GraphToSvg::getXcoord(double i) const
{
    return int(plot.width() * (i - minAng) / (maxAng - minAng));
}

int GraphToSvg::getYcoord(double i) const
{
    return int(plot.height() - plot.height() * (i - minIntens) / (maxIntens - minIntens));
}

double GraphToSvg::getRealX(int i) const
{
    return minAng + (maxAng - minAng) * (i - margin_l) / plot.width();
}

/*
 * conversion from *widget* Y coordinate to intensity
 * if dif = false: returns negative coordinate below the zero line
 * if dif = true:  returns coordinate relative to the zero line of the difference curve
 */
double GraphToSvg::getRealY(int i, bool dif) const
{
    double y = minIntens + double(plot.height() - i + margin_t) * (maxIntens - minIntens) / plot.height();

    // for the difference curve, add the vertical offset from the base line
    if ((y < 0.0) && (difOffset < 0.0) && dif) {
        switch (yScaling) {
        case YSCALESQRT:
            y -= getSqrt(difOffset);
            break;
        case YSCALELOG10:
            y -= getLog10(difOffset);
            break;
        default:
            y -= difOffset;
        }
    }

    return yScaleToValue(y);
}

/*
 * converts a value on the y scale to the true value
 * e.g. sqrt(y) -> y
 */
double GraphToSvg::yScaleToValue(double d) const
{
    double v;

    switch (yScaling) {
    case YSCALESQRT:
        v = getInvSqrt(d);
        break;
    case YSCALELOG10:
        v = getInvLog10(d);
        break;
    default:
        v = d;
    }

    return v;
}

QPoint GraphToSvg::interpolate(const QPoint &a, const QPoint &b, int x)
{
    if (a.x() == b.x()) return a;

    int dy = int(0.5 + a.y() + (b.y() - a.y()) * (double(x) - a.x()) / (b.x() - a.x()));

    return QPoint(x, dy);
}

void GraphToSvg::drawHklTicks(QPainter &p)
{
    // painter not active: exit
    if (!p.isActive()) return;

    // no reflection available: exit
    if (!scanHeap.count()) return;

    bool drawPhaseHkl = settings->value("graph/phaseVisibilityHkl", true).toBool();

    p.save();

    QFontMetrics fm(fontLegend);
    int markerSize = fm.ascent()/2;

    QRect r;

    // vertical length of the lines, use font size as maximum length,
    // but allow shorter lines
    int len = qMin(fm.ascent(), int(plot.height()/40.0));

    // spacing between tops of the lines
    int vspacing = int(1.5 * len);

    int top = int(pScale * 6);
    int bot = top + len;

    QPen pen;
    pen.setWidth(lineWidth);
    p.translate(plot.x(), plot.y());
    p.setClipRect(lineWidth, lineWidth, plot.width() - lineWidth, plot.height() - lineWidth);

    int vpos = 0;

    // loop over all scans and draw their hkl lines
    for (int n = 0; n < scanHeap.count(); ++n) {
        int idx = settings->value("graph/legendSortBottomUp", false).toBool() ? scanHeap.count() - n - 1 : n;

        Scan scan = scanHeap.at(idx);

        bool isPhase = scan.scanTypes().testFlag(Scan::PHASE);

        // skip scans that don't contain hkl data or scan data
        if (scan.pDataAngle().isEmpty() || scan.pDataIntensity().isEmpty() || scan.pDataHkl().isEmpty()) {
            continue;
        }

        // skip invisible scans
        if (!scan.isVisible()) {
            continue;
        }

        if (isPhase && !drawPhaseHkl) continue;

        const QVector<Hkl> &vec = scan.pDataHkl();
        int of = vpos * vspacing + top;
        pen.setColor(scan.color());

        bool convertToTT = scan.getHklXunit() == "dnm";

        p.setPen(pen);

        // draw line for reflections
        for (int i = 0; i < vec.size(); ++i) {
            Hkl hkl = vec.at(i);

            if (hkl.status() == 0) {
                continue;
            }

            int x = getXcoord(convertToTT ? global::Functions::dToTwoTheta(hkl.position(), 0.1*wavelength) : hkl.position());

            // ignore x-positions outside the displayed range
            if ((x < 0) || x > plot.width()) {
                continue;
            }

            if (hkl.status() == 2) {
                // hkl lines can be highlighted individually. trying to
                // keep the code as fast as possible for all non-highlighted ones
                pen.setWidth(2 * lineWidth);
                p.setPen(pen);
                p.drawLine(x, top + of, x, bot + of);

                p.drawLine(x - markerSize/2, top + of - markerSize, x, top + of);
                p.drawLine(x + markerSize/2, top + of - markerSize, x, top + of);
                p.drawLine(x - markerSize/2, bot + of + markerSize, x, bot + of);
                p.drawLine(x + markerSize/2, bot + of + markerSize, x, bot + of);

                pen.setWidth(lineWidth);
                p.setPen(pen);
            } else {
                p.drawLine(x, top + of, x, bot + of);
            }

            r.setLeft(x - 2 + plot.x());
            r.setRight(x + 2 + plot.x());
            r.setTop(top + plot.y() + of);
            r.setBottom(bot + plot.y() + of + 1);

            QString str;

            if (hkl.phase().isEmpty()) {
                str = QString("(%1) Texture=%2")
                .arg(hkl.hkl())
                    .arg(hkl.texture(), 0, 'f', 4);
            } else {
                str = QString("%1 (%2) Texture=%3")
                .arg(hkl.phase(),
                     hkl.hkl())
                    .arg(hkl.texture(), 0, 'f', 4);
            }
        }

        ++vpos;
    }

    p.restore();
}

void GraphToSvg::drawTicks(QPainter &p)
{
    if (!p.isActive()) return;

    // check if valid axes are defined
    if (minIntens >= maxIntens) return;
    if (minAng    >= maxAng)    return;

    updateTickDensity();

    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(lineWidth);

    p.save();
    p.setPen(pen);
    p.setClipRect(plot);
    p.translate(plot.x(), plot.y());

    int tLength = int(pScale * 5);

    QList<int> hPos = drawTicksXaxis(p, tLength);
    QList<int> vPos = drawTicksYaxis(p, tLength);
    vPos.append(drawTicksDiff(p, tLength));

    if (showGridMinorX || showGridMinorY) {
        pen.setColor(Qt::lightGray);
        pen.setStyle(Qt::DotLine);
        p.setPen(pen);

        // if (showGridMinorX) drawGridXaxis(p, hPos);
        // if (showGridMinorY) drawGridYaxis(p, vPos);
    }

    p.restore();
}

void GraphToSvg::updateTickDensity()
{
    QFontMetrics fm(fontTicks);
    int scanBottom = qMin(getYcoord(minIntens), getYcoord(0.0));
    int scanTop    = getYcoord(maxIntens);
    int diffZero   = qMin(getYcoord(minIntens), getYcoord(yValueToScale(difOffset)));
    int diffTop    = qMax(getYcoord(0.0), getYcoord(maxIntens));
    int diffBottom = getYcoord(minIntens);

    int rangeGraphHeight  = scanBottom - scanTop;
    int rangeDiffHeight   = qMax(diffZero - diffTop, diffBottom - diffZero);

    if (rangeGraphHeight > 0) { // else the graph is off screen
        tickDensityY = int(qMin(6.0, rangeGraphHeight / (3.0 * double(fm.height()))));
    }

    if (rangeDiffHeight > 0) { // else the difference curve is off screen
        tickDensityD = int(qMin(6.0, rangeDiffHeight / (3.0 * double(fm.height()))));
    }
}

QList<int> GraphToSvg::drawTicksXaxis(QPainter &p, int tLength)
{
    int td;

    switch (xScaling) {
    case XSCALED:
        td = 1;
        break;
    case XSCALEQ:
        td = 5;
        break;
    default:
        td = 6;
    }

    int ph = plot.height();
    QList<Tick> ticks = getTickPositionsX(xScaling, td * tickDensityX, 1);
    QList<int> pos;

    for (int i = 0; i < ticks.size(); ++i) {
        p.drawLine(ticks.at(i).position, 0.0, ticks.at(i).position, tLength);
        p.drawLine(ticks.at(i).position, ph,  ticks.at(i).position, ph - tLength);
        pos.append(ticks.at(i).position);
    }

    return pos;
}

QList<int> GraphToSvg::drawTicksYaxis(QPainter &p, int tLength)
{
    int pw = plot.width();
    QList<Tick> ticks = getTickPositionsY(yScaling, 6 * tickDensityY);
    QList<int> pos;

    // zero line if difference curve is visible
    if (minIntens <= 0) {
        int zero = getYcoord(0.0);
        p.drawLine(0.0, zero, pw, zero);
    }

    for (int i = 0; i < ticks.size(); ++i) {
        p.drawLine(0.0, ticks.at(i).position, tLength,      ticks.at(i).position);
        p.drawLine(pw,  ticks.at(i).position, pw - tLength, ticks.at(i).position);
        pos.append(ticks.at(i).position);
    }

    return pos;
}

QList<int> GraphToSvg::drawTicksDiff(QPainter &p, int tLength)
{
    if (minIntens > 0) return QList<int>();

    int pw = plot.width();
    int ph = plot.height();
    int uLim = getYcoord(yValueToScale(0.0));
    QList<Tick> ticks(getTickPositionsD(yScaling, 6 * tickDensityD));
    QList<int> pos;

    for (int i = 0; i < ticks.size(); ++i) {
        if (ticks.at(i).position > ph) continue;
        if (ticks.at(i).position <= uLim) continue;
        p.drawLine(0.0, ticks.at(i).position, tLength,      ticks.at(i).position);
        p.drawLine(pw,  ticks.at(i).position, pw - tLength, ticks.at(i).position);
        pos.append(ticks.at(i).position);
    }

    return pos;
}

/*
 * tType = 0: major ticks
 * tType = 1: minor ticks
 */
QList<Tick> GraphToSvg::getTickPositionsX(Scale xscale, int tDensity, int tType)
{
    QList<Tick> ticks;

    if (xscale == XSCALED)  {
        // tVec contains evenly spaced d values, must be converted to 2theta for drawing
        QList<double> tVecX = global::Functions::scaleAxisD(minAng, maxAng, wavelength, tType, tDensity);

        for (int i = tVecX.size() - 1; i >= 0; --i) {
            int pos = getXcoord(global::Functions::dToTwoTheta(tVecX.at(i), wavelength));
            Tick t(tVecX.at(i), tVecX.at(i), pos);
            ticks.append(t);
        }
    } else if (xscale == XSCALEQ) {
        // tVec contains evenly spaced Q values, must be converted to 2theta for drawing
        QList<double> tVecX = global::Functions::scaleAxisQ(minAng, maxAng, wavelength, tDensity);

        for (int i = tVecX.size() - 1; i >= 0; --i) {
            int pos = getXcoord(global::Functions::qToTwoTheta(tVecX.at(i), wavelength));
            Tick t(tVecX.at(i), tVecX.at(i), pos);
            ticks.append(t);
        }
    } else /* XSCALETWOTHETA */ {
        // tVec contains evenly spaced 2theta values
        QList<double> tVecX = global::Functions::scaleAxis1(minAng, maxAng, tDensity);

        for (int i = 0; i < tVecX.size(); ++i) {
            int pos = getXcoord(tVecX.at(i));
            Tick t(tVecX.at(i), tVecX.at(i), pos);
            ticks.append(t);
        }
    }

    return ticks;
}

QList<Tick> GraphToSvg::getTickPositionsY(Scale yscale, int tDensity)
{
    QList<Tick> ticks;

    double yMin = yScaleToValue(minIntens);
    double yMax = yScaleToValue(maxIntens);
    double yZero;

    switch (yscale) {
    case YSCALESQRT:
        yZero = 0.0;
        break;
    case YSCALELOG10:
        yZero = 1.0;
        break;
    default:
        yZero = 0.0;
    }

    QList<double> tVecY = global::Functions::scaleAxis1(qMax(yZero, yMin), yMax, tDensity);

    for (int i = 0; i < tVecY.size(); ++i) {
        double valScaled = yValueToScale(tVecY.at(i));
        int pos = getYcoord(valScaled);
        Tick tick(tVecY.at(i), valScaled, pos);
        ticks.append(tick);
    }

    return ticks;
}

QList<Tick> GraphToSvg::getTickPositionsD(Scale yscale, int tDensity)
{
    QList<Tick> ticks;

    double doffScale = yValueToScale(difOffset);
    double difZeroPos = getYcoord(doffScale);
    double difScreenOffset = difZeroPos - getYcoord(yValueToScale(0.0));

    // Visible range above and below the difference zero line. If the range is visible, the value is positive.
    // If the range is off screen, the value is negative.
    double posLim = qMin(-difOffset, yScaleToValue(maxIntens - doffScale));
    double negLim = -yScaleToValue(minIntens - doffScale);

    double dStart, dEnd;

    if ((posLim > 0.0) && (negLim > 0.0)) {
        //both ranges are visible, the diff zero line is on screen
        dStart = 0.0;
        dEnd = qMax(posLim, negLim);
    } else if (negLim <= 0.0) {
        // centered on the positive range, the diff zero line is below the screen
        dStart = -negLim;
        dEnd = posLim;
    } else {
        // centered on the negative range, the diff zero line is above the screen
        dStart = negLim;
        dEnd = -posLim;
    }

    if (yscale == YSCALELOG10) {
        if (qFuzzyIsNull(dStart)) dStart = 1.0;
    }

    QList<double> tVecY = global::Functions::scaleAxis1(dStart, dEnd, tDensity);

    for (int i = 0; i < tVecY.size(); ++i) {
        double valScaled = yValueToScale(tVecY.at(i));
        int posPos = int(getYcoord(valScaled) + difScreenOffset);
        int posNeg = difZeroPos + (difZeroPos - posPos);
        ticks.append(Tick(tVecY.at(i), valScaled, posPos));
        if (posNeg != posPos) ticks.append(Tick(-tVecY.at(i), valScaled, posNeg));
    }

    return ticks;
}

void GraphToSvg::drawLabels(QPainter &p)
{
    if (!p.isActive()) return;

    // check if valid axes are defined
    if (minIntens >= maxIntens) return;
    if (minAng >= maxAng) return;

    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(lineWidth);
    pen.setCapStyle(Qt::FlatCap);

    p.save();
    p.setFont(fontTicks);
    p.setPen(pen);
    p.translate(plot.x(), plot.y());

    int tLength = int(pScale * 10);
    int lSpacing = margin_i;
    QFontMetrics fm(fontTicks);

    QList<int> hPos = drawLabelsXaxis(p, fm, tLength, lSpacing);
    QList<int> vPos = drawLabelsYaxis(p, fm, tLength, lSpacing);
    vPos.append(drawLabelsDiff(p, fm, tLength, lSpacing));

    if (showGridMajorX || showGridMajorY) {
        pen.setColor(Qt::gray);
        pen.setStyle(Qt::DashLine);
        p.setPen(pen);

        // if (showGridMajorX) drawGridXaxis(p, hPos);
        // if (showGridMajorY) drawGridYaxis(p, vPos);
    }

    p.restore();
}

QList<int> GraphToSvg::drawLabelsXaxis(QPainter &p, const QFontMetrics &fm, int tLength, int lSpacing)
{
    QList<Tick> ticks = getTickPositionsX(xScaling, tickDensityX, 0);
    QList<int> pos;

    if (ticks.size() < 2) return pos;

    int ph = plot.height();
    int dig = global::Functions::digitsForValues(ticks.at(0).value, ticks.at(1).value, 2);

    for (int i = 0; i < ticks.size(); ++i)
    {
        QString s = QString("%1").arg(ticks.at(i).value, 0, 'f', dig);
        p.drawText(ticks.at(i).position - int(fm.horizontalAdvance(s) / 2.0), ph + fm.height() + lSpacing, s);
        p.drawLine(ticks.at(i).position, 0.0, ticks.at(i).position, tLength);
        p.drawLine(ticks.at(i).position, ph, ticks.at(i).position, ph - tLength);
        pos.append(ticks.at(i).position);
    }

    return pos;
}

QList<int> GraphToSvg::drawLabelsYaxis(QPainter &p, const QFontMetrics &fm, int tLength, int lSpacing)
{
    QList<Tick> ticks = getTickPositionsY(yScaling, tickDensityY);
    QList<int> pos;

    int pw = plot.width();
    int prevPos = std::numeric_limits<int>::min();
    int asc = fm.ascent();
    int lSpace = fm.lineSpacing();

    int dig = ticks.size() < 2 ? 0 : global::Functions::digitsForValues(ticks.at(0).value, ticks.at(1).value, 0);

    for (int i = 0; i < ticks.size(); ++i) {
        int tp = ticks.at(i).position;

        if (qAbs(tp - prevPos) >= lSpace) { // skip labels that would overlap the previous one
            QString s = QString("%1").arg(ticks.at(i).value, 0, 'f', dig);
            p.drawText(-(fm.horizontalAdvance(s) + lSpacing), tp + int(asc / 2.0), s);
            prevPos = tp;
        }

        p.drawLine(0.0, tp, tLength,      tp);
        p.drawLine(pw,  tp, pw - tLength, tp);
        pos.append(tp);
    }

    return pos;
}

QList<int> GraphToSvg::drawLabelsDiff(QPainter &p, const QFontMetrics &fm, int tLength, int lSpacing)
{
    if (minIntens > 0) return QList<int>();

    QList<Tick> ticks(getTickPositionsD(yScaling, tickDensityD));
    QList<int> pos;

    int pw = plot.width();
    int ph = plot.height();
    int uLimTick = getYcoord(yValueToScale(0.0));
    int uLimLab  = uLimTick + fm.lineSpacing();
    int prevPos = std::numeric_limits<int>::min();
    int lSpace = fm.lineSpacing();

    int dig = ticks.size() < 2 ? 0 : global::Functions::digitsForValues(ticks.at(0).value, ticks.at(1).value, 0);

    for (int i = 0; i < ticks.size(); ++i) {
        int tp = ticks.at(i).position;
        if (tp > ph) continue;
        if (tp <= uLimTick) continue;

        if (drawDifferenceTickLabels) {
            if (qAbs(tp - prevPos) >= lSpace) { // skip labels that would overlap the previous one
                if (tp > uLimLab) {             // do not draw labels above uLimLab because they overlap with the label of the main y-axis
                    QString s = QString("%1").arg(ticks.at(i).value, 0, 'f', dig);
                    p.drawText(-(fm.horizontalAdvance(s) + lSpacing), tp + int(fm.ascent() / 2.0), s);
                }
                prevPos = tp;
            }
        }

        p.drawLine(0.0, tp, tLength,      tp);
        p.drawLine(pw,  tp, pw - tLength, tp);
        pos.append(tp);
    }

    return pos;
}

void GraphToSvg::drawLegend(QPainter &p)
{
    if (!p.isActive()) return;

    QList<QColor> colors;
    QStringList labels;
    QList<int> lineWidths;

    for (int i = 0; i < scanHeap.count(); ++i) {
        Scan scan = scanHeap.at(i);

        if (!scan.isVisible()) continue;

        labels.append(scan.name(false));
        colors.append(scan.color());
        lineWidths.append(scan.lineWidth() < lineWidth ? lineWidth : scan.lineWidth());
    }

    QFont fontLegend("Sans Serif", 10);
    QFontInfo fiLegend(fontLegend);
    fontLegend.setPixelSize(int(pScale * fiLegend.pixelSize()));

    int lTextWidth = 0;
    QFontMetrics fm(fontLegend);

    for (int i = 0; i < labels.size(); ++i) {
        lTextWidth = qMax(fm.boundingRect(labels.at(i)).width(), lTextWidth);
    }

    int lLineSpacing = fm.lineSpacing();
    int lTopBoundingBox = plot.top() + fm.lineSpacing() + fm.ascent();
    int lRightBoundingBox = lTextWidth + fm.boundingRect("MM").width();
    int lLineLength = fm.boundingRect("MMMM").width();
    int lHorizontalSpacing = fm.boundingRect("M").width();

    int lLeftBoundingBox = plot.right() - lRightBoundingBox - 2*lHorizontalSpacing - lLineLength;
    int lLeftText = plot.right() - lRightBoundingBox - lHorizontalSpacing;
    int lTopText = lTopBoundingBox;

    QRect bRect = QRect(lLeftBoundingBox - lHorizontalSpacing,
                  lTopBoundingBox - int(fm.height() / 2.0),
                  lTextWidth + 2 * lHorizontalSpacing + lLeftText - lLeftBoundingBox,
                  labels.size() * lLineSpacing + int(fm.descent()));

    p.save();

    QColor colBox(Qt::black);

    p.setPen(QPen(colBox));
    p.setBrush(Qt::white);
    p.drawRect(bRect);

    int v = lTopText;
    p.setFont(fontLegend);

    int lastIdx = qMin(labels.size(), qMin(colors.size(), lineWidths.size()));

    for (int i = 0; i < lastIdx; i++) {
        QPen pen(colors.at(i));
        pen.setWidth(lineWidths.at(i));

        p.setPen(pen);
        p.drawLine(lLeftBoundingBox, v, lLeftBoundingBox + lLineLength, v);
        p.drawText(lLeftText, v + int(fm.ascent() / 2.0), labels.at(i));
        v += lLineSpacing;
    }

    p.restore();
}

void GraphToSvg::updateScanColors(bool force)
{
    QList<QColor> colorList = settings->getColorList(scanHeap.size());

    for (int i = 0; i < scanHeap.size(); ++i) {
        Scan * scan = &(scanHeap[i]);
        if (!scan) continue;

        if (scan->color().isValid() && !force) continue;

        if (scan->scanTypes().testFlag(Scan::REFINED)) {
            switch (i) {
            case 0: scan->setColor(QColor(settings->value("graph/iobsColor", "#000000").toString()));
                break;
            case 1: scan->setColor(QColor(settings->value("graph/icalcColor", "#ff0000").toString()));
                break;
            case 2: scan->setColor(QColor(settings->value("graph/idiffColor", "#bbbbbb").toString()));
                break;
            case 3: scan->setColor(QColor(settings->value("graph/ibkgrColor", "#0000ff").toString()));
                break;
            default: scan->setColor(colorList.at(i));
            }
        } else {
            scan->setColor(colorList.at(i));
        }
    }
}

void GraphToSvg::updateScanStyles(bool force)
{
    QList<int> styleList = settings->getScanStyleList(scanHeap.size());

    for (int i = 0; i < scanHeap.size(); ++i) {
        Scan * scan = &(scanHeap[i]);
        if (!scan) continue;

        if (scan->pointSymbol() < 0 || force) {
            scan->setPointSymbol(styleList.at(i));
        }
    }
}

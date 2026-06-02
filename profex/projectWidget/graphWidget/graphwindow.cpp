/***************************************************************************
                          graphwindow.cpp  -  description
                             -------------------
    begin                : Die Jun 17 2003
    copyright            : (C) 2003 by Nicola Doebelin
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

#include "graphwindow.h"

#include "graphdatacontroller.h"
#include "../../../libXrdIO/import/importhandler.h"
#include "../../../libXrdIO/scanops.h"

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QPrinter>
#include <QTime>
#include <QSvgGenerator>
#include <QPolygon>
#include <QToolTip>
#include <QStaticText>
#include <QDebug>

#include <limits>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

GraphWindow::GraphWindow(GraphDataController *c, QWidget *parent)
    : AbstractGraphView(c, parent)
{
    vModes.insert(global::ViewUpdateMode::DISPLAY);

    QSizePolicy sp = sizePolicy();
    sp.setControlType(QSizePolicy::Frame);
    sp.setVerticalPolicy(QSizePolicy::Expanding);
    sp.setHorizontalPolicy(QSizePolicy::Expanding);
    setSizePolicy(sp);
    setAccessibleName(tr("Refined graph display"));

    setMouseTracking(true);
    recalcPlot = true;
    darkTheme = false;

    ihandler = new ImportHandler();
    keyHandler = new GraphWindowKeyEventHandler(this);
    rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    legend = new GraphWindowLegend(c, this);
    c->addView(legend);
    connect(legend, SIGNAL(sigRequestRedraw()), this, SLOT(forceUpdate()));

    contextMenuXaxis    = new QMenu(this);
    contextMenuYaxis    = new QMenu(this);
    contextMenuDataArea = new QMenu(this);

    contextMenuXaxis->addAction(QString(tr("2%1 Scale")).arg(global::theta), this, SLOT(setXaxis2theta()));
    contextMenuXaxis->addAction(tr("d Spacing Scale"), this, SLOT(setXaxisD()));
    contextMenuXaxis->addAction(tr("Q Scale"), this, SLOT(setXaxisQ()));
    contextMenuXaxis->addSeparator();
    contextMenuXaxis->addAction(tr("Reset Zoom"), this, SLOT(resetZoomX()));

    contextMenuYaxis->addAction(tr("Linear Scale"), this, SLOT(setYaxisLinear()));
    contextMenuYaxis->addAction(tr("Sqrt Scale"), this, SLOT(setYaxisSqrt()));
    contextMenuYaxis->addAction(tr("Log10 Scale"), this, SLOT(setYaxisLog10()));
    contextMenuYaxis->addSeparator();
    contextMenuYaxis->addAction(tr("Zoom to Baseline"), this, SLOT(zoomFromZeroY()));
    contextMenuYaxis->addAction(tr("Zoom to Minimum Intensity"), this, SLOT(zoomFromMinY()));
    contextMenuYaxis->addAction(tr("Reset Zoom"), this, SLOT(resetZoomY()));

    pBuffer = QPixmap(width(), height());

    crossHair = false;
    noiseCursor = false;
    middleMouseButtonDragging = false;
    inspector = false;
    drawRubberBand = false;
    dragging = false;
    zoomed = false;
    specLines = false;
    useBackgroundColors = false;
    firstDoubleClickPoint = QPoint();
    peakPreviewMode = PPMNONE;
    rangeSelectMode = false;
    fillActive = true;
    hasOverrideWaveLength = false;

    _maxIntensAbs = 0.0;
    _minIntensAbs = 0.0;
    points = 0;
    _maxAngDisp = 0.0;
    _minAngDisp = 0.0;
    difOffset = 0.0;
    anchorDragged = -1;

    waveLength[0] = -1.0;
    waveLength[1] = -1.0;
    waveLength[2] = -1.0;
    wavelengthMode = Scan::WavelengthMode::UNKNOWN;

    // sample height displacement, as used by BGMN's EPS2 parameter
    // delta = sample displacement in mm
    // iRadius = goniometer radius in mm
    //
    // EPS2 = 2delta / iRadius
    eps1 = 0.0;
    eps2 = 0.0;
    eps3 = 0.0;

    pScale = 1.0;
    scanMaxIntensity = 0.0;
    prevStrucScaleFactor = 1.0;
    prevScanScaleFactor = 1.0;

    completeFileName = QString();
    fileName = QString();
    fileUid = QString();

    colorTable.append(QColor(0, 0, 0, 255));
    colorTable.append(QColor(255, 0, 0, 255));
    colorTable.append(QColor(0, 0, 255, 255));
    colorTable.append(QColor(0, 0, 0, 255));

    updateCoordinateTimer.setSingleShot(true);

    xScaling = XSCALETWOTHETA;
    yScaling = YSCALELIN;
    antiAliasing = false;
    showGridMinorX = false;
    showGridMajorX = false;
    showGridMinorY = false;
    showGridMajorY = false;

    QAction *actCopy = addAction(tr("Copy graph"));
    actCopy->setShortcut(global::Functions::keyCopy());
    actCopy->setShortcutContext(Qt::WidgetWithChildrenShortcut);

    connect(actCopy, &QAction::triggered, this, &GraphWindow::copyPixmapToClipboard);

    initSettings();
}

// Destructor
GraphWindow::~GraphWindow()
{
    delete ihandler;
    delete keyHandler;

    delete contextMenuXaxis;
    delete contextMenuYaxis;
    delete contextMenuDataArea;
    delete legend;
}

void GraphWindow::initSettings()
{
    oFontTitle.fromString(settings->value("graph/fontTitle", font().toString()).toString());
    oFontAxis.fromString(settings->value("graph/fontAxis", font().toString()).toString());
    oFontTicks.fromString(settings->value("graph/fontTicks", font().toString()).toString());
    oFontLegend.fromString(settings->value("graph/fontLegend", font().toString()).toString());
    lineWidth = settings->value("graph/lineWidth", 1).toInt();
    symbolSize = settings->value("graph/crossSize", 1).toInt();
    printingLineWidth = settings->value("graph/printingLineWidth", 3).toInt();
    antiAliasing = settings->value("graph/antiAliasing", false).toBool();
    fillActive = settings->value("graph/fillActiveScan", true).toBool();
    drawDifferenceTickLabels = settings->value("graph/differenceTickLabels", false).toBool();
    differenceColor = QColor(settings->value("graph/idiffColor", "#bbbbbb").toString());

    int xUnit = settings->value("graph/xAxisUnit", 0).toInt();
    int yScale = settings->value("graph/yAxisScale", 0).toInt();

    if      (static_cast<Scale>(xUnit + 4) == XSCALETWOTHETA) setXaxis2theta(false);
    else if (static_cast<Scale>(xUnit + 4) == XSCALED)        setXaxisD(false);
    else if (static_cast<Scale>(xUnit + 4) == XSCALEQ)        setXaxisQ(false);

    if      (static_cast<Scale>(yScale) == YSCALELIN)         setYaxisLinear(false);
    else if (static_cast<Scale>(yScale) == YSCALESQRT)        setYaxisSqrt(false);
    else if (static_cast<Scale>(yScale) == YSCALELOG10)       setYaxisLog10(false);

    if (waveLength[0] < 0.0) {
        scanControl->getNearestCharacteristicWaveLength(waveLength[0], waveLength[1], waveLength[2], settings->defaultWavelength());
    }

    useCountsPerSecond = settings->value("graph/countsPerSecond", false).toBool();
    showGridMinorX = settings->value("graph/showMinorGridLinesX", false).toBool();
    showGridMajorX = settings->value("graph/showMajorGridLinesX", false).toBool();
    showGridMinorY = settings->value("graph/showMinorGridLinesY", false).toBool();
    showGridMajorY = settings->value("graph/showMajorGridLinesY", false).toBool();

    tickDensityX = settings->value("graph/tickMarkDensityX", 10).toInt();
    tickDensityY = settings->value("graph/tickMarkDensityY", 5).toInt();
    tickDensityD = settings->value("graph/tickMarkDensityD", 2).toInt();

    fontTitle  = oFontTitle;
    fontAxis   = oFontAxis;
    fontTicks  = oFontTicks;
    fontLegend = oFontLegend;

    hklReferenceBase = settings->value("graph/hklOnBackground", 1).toInt();
    hklScanBase      = settings->value("graph/hklScanOnBackground", 0).toInt();

    // margin colors
    useBackgroundColors = settings->value("graph/useBackgroundColors", true).toBool();

    // background color
    QColor bgC = QColor(settings->value("graph/backgroundColor", "").toString());
    bgColor = bgC.isValid() ? bgC : QGuiApplication::palette().color(QPalette::Base);

    // determine the dark theme from the background color, not from the system palette. Because we can
    // select a dark background even if the theme is light. Then we need the plot to behave as in dark theme.
    darkTheme = bgColor.value() < 150;

    printingBgColor = Qt::white;
    activeColor     = QColor(settings->value("graph/activeColor",     "#ff9999").toString());
    completedColor  = QColor(settings->value("graph/completeColor",   "#99ff99").toString());
    abortedColor    = QColor(settings->value("graph/errorColor",      "#ffff99").toString());
    peakSelectColor = QColor(settings->value("graph/peakSelectColor", "#7ec3eb").toString());
    idleColor = bgColor;
    marginColor = bgColor;

    // axis color
    axisColor = QColor(settings->value("graph/axisColor", "#000000").toString());

    if (!settings->value("bgmnProject/readLamFile", false).toBool()) {
         scanControl->clearCustomWaveLength();
    }

    legend->initSettings();

    if (scanControl->count() > 0) {
        changeGraphStyles();
    }
}

void GraphWindow::updateView()
{
    eps1 = scanControl->getAngularCorrEPS1();
    eps2 = scanControl->getAngularCorrEPS2();
    eps3 = scanControl->getAngularCorrEPS3();
    setupNewScans();
    forceUpdate();
}

void GraphWindow::resetView()
{
    hasOverrideWaveLength = false;
}

void GraphWindow::resizeEvent(QResizeEvent *)
{
    double dpr = devicePixelRatioF();
    pBuffer = QPixmap(width() * dpr, height() * dpr);
    pBuffer.setDevicePixelRatio(dpr);
    recalcPlot = true;
}

void GraphWindow::paintEvent(QPaintEvent *e)
{
    if (pBuffer.width() == 0 || pBuffer.height() == 0) {
        return;
    }

    QPainter p;

    if (recalcPlot) {
        p.begin(&pBuffer);
            calcMargins(p, true);
            initPlot(rect());

            p.setRenderHint(QPainter::Antialiasing, antiAliasing);
            p.setRenderHint(QPainter::TextAntialiasing, true);
            p.setRenderHint(QPainter::SmoothPixmapTransform, true);

            drawMargin(p);
            drawWindow(p);
            drawHighlightedRegions(p);
            drawPlot(p, false);
            drawAnchorPoints(p);
            drawHklTicks(p);
            drawHklScans(p);
            drawHklReferenceLines(p);
            drawTicks(p);
            drawLabels(p);
            drawLegend(p);
        p.end();
    }

    p.begin(this);
        double dpr = devicePixelRatioF();
        QRectF trgt(e->rect());
        QRectF srcr(e->rect().x()*dpr, e->rect().y()*dpr, e->rect().width()*dpr, e->rect().height()*dpr);

        p.drawPixmap(trgt, pBuffer, srcr);
        p.setClipRect(plot);

        // draw temporary overlays
        if (crossHair)                   drawCrossHairCursor(p);
        if (noiseCursor)                 drawNoiseCursor(p);
        if (specLines)                   drawSpecLineCursor(p);
        if (inspector)                   drawInspector(p);
        if (peakPreviewMode != PPMNONE)  drawPeakPreviews(p);
        if (rangeSelectMode)             drawRange(p);
        if (keyHandler->hasKeyCommand()) drawKeySequenceOverlay(p);
    p.end();

    recalcPlot = false;
}

void GraphWindow::drawCrossHairCursor(QPainter &p)
{
    QPen pen(darkTheme ? Qt::white : Qt::black);
    pen.setStyle(Qt::DashLine);
    pen.setWidth(lineWidth);
    p.setPen(pen);

    QPoint pt = mapFromGlobal(cursor().pos());
    p.drawLine(plot.x(), pt.y(), plot.width() + plot.x() + 1, pt.y());
    p.drawLine(pt.x(), plot.y(), pt.x(), plot.height() + plot.y() + 1);
}

void GraphWindow::drawNoiseCursor(QPainter &p)
{
    if (!scanControl->count()) return;
    QFontMetrics fml(fontLegend);
    QPen pen(darkTheme ? Qt::white : Qt::black);
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(lineWidth);
    p.setPen(pen);

    int px = mapFromGlobal(cursor().pos()).x();
    int py = mapFromGlobal(cursor().pos()).y();
    double yc = getRealY(py);
    bool dCurve = false;

    if (py > getYcoord(0.0)) { // below the zero line
        // on difference curve use the intensity of the active scan
        Scan *sc = scanControl->firstActiveScan();
        double xc = getRealX(px);
        int angIdx = sc->indexOfAngle(xc + angularCorrection(xc) - sc->xOffset(), 0);
        yc = sc->intensity(angIdx);
        dCurve = true;
    }

    if (yc > 0.0) {
        int endWidth = fml.horizontalAdvance("m");
        int midWidth = endWidth / 2;

        int yTop = 0;
        int yCnt = 0;
        int yBot = 0;
        int yDif = 0;

        if (yScaling == YSCALELIN) {
            yTop = getYcoord(yc + sqrt(yc));
            yCnt = getYcoord(yc);
            yBot = getYcoord(yc - sqrt(yc));
            yDif = getYcoord(sqrt(yc));
        }

        if (yScaling == YSCALESQRT) {
            yTop = getYcoord(getSqrt(yc + sqrt(yc)));
            yCnt = getYcoord(getSqrt(yc));
            yBot = getYcoord(getSqrt(yc - sqrt(yc)));
            yDif = getYcoord(getSqrt(sqrt(yc)));
        }

        int pyt = py + yTop - yCnt;
        int pyc = py;
        int pyb = py - yCnt + yBot;
        int pyd = yDif - getYcoord(0.0);

        if (dCurve) {
            // on the difference curve
            p.drawLine(px,          pyc+pyd, px,          pyc-pyd); // vertical
            p.drawLine(px-endWidth, pyc-pyd, px+endWidth, pyc-pyd); // upper end
            p.drawLine(px-midWidth, pyc,     px+midWidth, pyc);     // center
            p.drawLine(px-endWidth, pyc+pyd, px+endWidth, pyc+pyd); // lower end
        } else {
            // in the positive area
            p.drawLine(px,          pyt, px,          pyb); // vertical
            p.drawLine(px-endWidth, pyt, px+endWidth, pyt); // upper end
            p.drawLine(px-midWidth, pyc, px+midWidth, pyc); // center
            p.drawLine(px-endWidth, pyb, px+endWidth, pyb); // lower end
        }
    }
}

void GraphWindow::drawSpecLineCursor(QPainter &p)
{
    QFontMetrics fml(fontLegend);
    QPen pen;
    pen.setStyle(Qt::DashLine);
    pen.setWidth(lineWidth);
    p.setFont(fontLegend);

    double twotheta = getRealX(mapFromGlobal(cursor().pos()).x());
    double d = global::Functions::twoThetaToD(twotheta, waveLength[0]);

    QMap<QString, double> wlengths;

    if (!scanControl->getCustomWaveLength()->isEmpty()) {
        // spectral lines from *.lam files were parsed, display those
        pen.setColor(darkTheme ? Qt::white : Qt::black);
        p.setPen(pen);

        wlengths = *(scanControl->getCustomWaveLength());
    } else if (waveLength[0] > 0.0) {
        pen.setColor(darkTheme ? global::Functions::colorToDarkMode(Qt::blue) : Qt::blue);
        p.setPen(pen);

        QString labl1 = QString("K%1%2").arg(global::alpha).arg(global::subOne);
        QString labl2 = QString("K%1%2").arg(global::alpha).arg(global::subTwo);
        QString labl3 = QString("K%1").arg(global::beta);

        switch (wavelengthMode) {
        case Scan::WavelengthMode::UNKNOWN:
            wlengths[labl1] = waveLength[0];
            if (waveLength[1] > 0.0) wlengths[labl2] = waveLength[1];
            if (waveLength[2] > 0.0) wlengths[labl3] = waveLength[2];
            break;

        case Scan::WavelengthMode::CHARACTERISTIC:
            wlengths[labl1] = waveLength[0];
            if (waveLength[1] > 0.0) wlengths[labl2] = waveLength[1];
            if (waveLength[2] > 0.0) wlengths[labl3] = waveLength[2];
            break;

        case Scan::WavelengthMode::SYNCHROTRON:
            wlengths["Synchrotron"] = waveLength[0];
            break;

        case Scan::WavelengthMode::NEUTRON:
            wlengths["Neutron"] = waveLength[0];
            break;

        default:
            wlengths[labl1] = waveLength[0];
            if (waveLength[1] > 0.0) wlengths[labl2] = waveLength[1];
            if (waveLength[2] > 0.0) wlengths[labl3] = waveLength[2];
            break;
        }

    }

    if (!wlengths.isEmpty()) {
        p.setClipRect(plot.x() + lineWidth, plot.y() + lineWidth, plot.width()-lineWidth, plot.height()-lineWidth);

        QMap<QString, double>::ConstIterator itw = wlengths.constBegin();

        while (itw != wlengths.constEnd()) {
            int ttip = getXcoord(global::Functions::dToTwoTheta(d, itw.value())) + plot.x();

            p.drawLine(ttip, plot.y() + 3 * fml.ascent(), ttip, plot.height() + plot.y() + 1);
            p.drawText(ttip - fml.horizontalAdvance(itw.key()) / 2, plot.y() + 2 * fml.ascent(), itw.key());

            pen.setColor(darkTheme ? Qt::white : Qt::black);
            p.setPen(pen);

            ++itw;
        }

        QStringList wlines = settings->value("graph/specCursorTungstenLines", QStringList()).toStringList();

        if (wlines.size()) {
            pen.setColor(darkTheme ? global::Functions::colorToDarkMode(Qt::gray) : Qt::gray);
            p.setPen(pen);

            for (int tl = 0; tl < wlines.size(); ++tl) {
                QString wLbl = wlines.at(tl);
                double wWl = global::tungstenLines.value(wLbl);

                int wLinePos  = getXcoord(global::Functions::dToTwoTheta(d, wWl)) + plot.x();
                p.drawLine(wLinePos, plot.y() + 3 * fml.ascent(), wLinePos, plot.height() + plot.y() + 1);
                p.drawText(wLinePos - fml.horizontalAdvance(wLbl) / 2, plot.y() + 2 * fml.ascent(), wLbl);
            }
        }
    }
}

void GraphWindow::drawInspector(QPainter &p)
{
    QPoint pt(mapFromGlobal(cursor().pos()));

    if (!plot.contains(pt)) return;

    QFontMetrics fms(fontLegend);
    int size = fms.horizontalAdvance("n");

    QPen pen(darkTheme ? Qt::white : Qt::black);
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(lineWidth);
    p.setPen(pen);

    const Scan *scan = scanControl->firstActiveScan();
    if (!scan) scan = scanControl->first();
    if (!scan) return;

    double rc = getRealX(pt.x());

    int angIdx = scan->indexOfAngle(rc + angularCorrection(rc) - scan->xOffset(), 0);

    if (angIdx < 0) return;

    double tps = 1.0;

    if (useCountsPerSecond) {
        tps = scan->timePerStep() > 0.0 ? scan->timePerStep() : 1.0;
    }

    double xc  = scan->angle(angIdx);
    double yc  = scan->intensity(angIdx) * scan->scaleFactor() / tps;

    QString s_deg = QString("%1%2%3").arg(global::degree).arg("2").arg(global::theta);
    QString s_unt = useCountsPerSecond ? "cps" : "counts";
    QString str   = QString("%1 %2 / %3 %4 / n = %5")
            .arg(xc, 0, 'f', 4)
            .arg(s_deg)
            .arg(yc, 0, 'f', 2)
            .arg(s_unt)
            .arg(angIdx);

    int vpos = pt.y() - size;
    if (vpos < plot.top() + fms.height()) vpos = pt.y() + size + fms.ascent();

    int hpos = pt.x() + size;
    if (hpos > plot.right() - fms.horizontalAdvance(str) - size) hpos = pt.x() - size - fms.horizontalAdvance(str);

    p.drawText(hpos, vpos, str);

    double yo = scan->yOffset();

    if (yScaling == YSCALESQRT)  {
        yo = getSqrt(yo);
        yc = getSqrt(yc);
    }

    if (yScaling == YSCALELOG10) {
        yo = getLog10(yo);
        yc = getLog10(yc);
    }

    int sx = getXcoord(xc - angularCorrection(xc) + scan->xOffset()) + plot.x();
    int sy = getYcoord(yc + yo) + plot.y();

    p.drawLine(sx - size, sy - size, sx + size, sy + size);
    p.drawLine(sx - size, sy + size, sx + size, sy - size);
    p.drawLine(sx,        sy + size, sx,        sy - size);
    p.drawLine(sx - size, sy,        sx + size, sy);
}

void GraphWindow::drawPeakPreviews(QPainter &p)
{
    QPoint pt(mapFromGlobal(cursor().pos()));

    if (firstDoubleClickPoint.isNull()) {
        if (peakPreviewMode == PPMLIN)        emit sigCursorMessage(tr("Double-click to set first point"), rangeCallerUid);
        else if (peakPreviewMode == PPMPEAK)  emit sigCursorMessage(tr("Double-click to set tip of peak"), rangeCallerUid);
        return;
    }

    if (peakPreviewMode == PPMLIN)        emit sigCursorMessage(tr("Double-click to set second point"), rangeCallerUid);
    else if (peakPreviewMode == PPMPEAK)  emit sigCursorMessage(tr("Double-click to set base of peak"), rangeCallerUid);

    p.save();
    p.setCompositionMode(compositionModeLines());

    QPen pen(darkTheme ? Qt::white : Qt::black);
    pen.setStyle(Qt::DashLine);
    pen.setWidth(lineWidth);
    p.setPen(pen);

    if (peakPreviewMode == PPMLIN) {
        p.drawLine(firstDoubleClickPoint.x(), firstDoubleClickPoint.y(), pt.x(), pt.y());
    }

    if (peakPreviewMode == PPMPEAK) {
        int dx = pt.x() - firstDoubleClickPoint.x();
        int dy = pt.y() - firstDoubleClickPoint.y();
        p.drawLine(firstDoubleClickPoint.x(),
                   firstDoubleClickPoint.y(),
                   firstDoubleClickPoint.x() - dx,
                   firstDoubleClickPoint.y() + dy);
        p.drawLine(firstDoubleClickPoint.x(),
                   firstDoubleClickPoint.y(),
                   firstDoubleClickPoint.x() + dx,
                   firstDoubleClickPoint.y() + dy);
        p.drawLine(firstDoubleClickPoint.x() - int(0.5*dx),
                   firstDoubleClickPoint.y() + int(0.5*dy),
                   firstDoubleClickPoint.x() + int(0.5*dx),
                   firstDoubleClickPoint.y() + int(0.5*dy));
    }

    p.restore();
}

void GraphWindow::drawRange(QPainter &p)
{
    if (firstDoubleClickPoint.isNull()) {
        emit sigCursorMessage(tr("Double-click to set start of range"), rangeCallerUid);
        return;
    } else {
        emit sigCursorMessage(tr("Double-click to set end of range"), rangeCallerUid);
    }

    QPoint ptA(firstDoubleClickPoint.x(), plot.top());
    QPoint ptB(mapFromGlobal(cursor().pos()).x(), plot.bottom());

    QColor rgCol = QColor(settings->value("graph/integralRangeColor", QString("#cccccc")).toString());
    if (darkTheme) rgCol = rgCol.darker(350);

    p.save();

    p.setCompositionMode(compositionModeFill());
    p.fillRect(QRect(ptA, ptB), rgCol);

    p.restore();
}

void GraphWindow::drawKeySequenceOverlay(QPainter &p)
{
    static QRegularExpression rx("^(q|w|z[ai]?)\\s*((?:\\d+\\.?\\d*)|m|c|n)?(\\s+)?((?:\\d+\\.?\\d*)|m|c|n)?");
    qDebug() << keyHandler->getKeyCommand();
    QRegularExpressionMatch rm = rx.match(keyHandler->getKeyCommand());
    if (!rm.hasMatch()) return;

    QString str;
    QString valA;
    QString valB;

    if (rm.captured(1) == "z" || rm.captured(1) == "za") {
        valA = rm.captured(2).isNull() ? "&lt;start&gt;" : rm.captured(2);
        valB = rm.captured(4).isNull() ? "&lt;end&gt;"   : rm.captured(4);

        if (rm.captured(2).isNull()) {
            str = QString("Zoom angle: <b>%1</b> <font color=\"#777777\">%2</font>").arg(valA, valB);
        } else if (rm.captured(3).isNull()) {
            str = QString("Zoom angle: <b>%1</b> <font color=\"#777777\">%2</font>").arg(valA, valB);
        } else if (rm.captured(4).isNull()) {
            str = QString("Zoom angle: %1 <b>%2</b>").arg(valA, valB);
        } else {
            str = QString("Zoom angle: %1 <b>%2</b>").arg(valA, valB);
        }
    } else if (rm.captured(1) == "zi") {
        valA = rm.captured(2).isNull() ? "&lt;start&gt;" : rm.captured(2);
        valB = rm.captured(4).isNull() ? "&lt;end&gt;"   : rm.captured(4);

        if (rm.captured(2).isNull()) {
            str = QString("Zoom intensity: <b>%1</b> <font color=\"#777777\">%2</font>").arg(valA, valB);
        } else if (rm.captured(3).isNull()) {
            str = QString("Zoom intensity: <b>%1</b> <font color=\"#777777\">%2</font>").arg(valA, valB);
        } else if (rm.captured(4).isNull()) {
            str = QString("Zoom intensity: %1 <b>%2</b>").arg(valA, valB);
        } else {
            str = QString("Zoom intensity: %1 <b>%2</b>").arg(valA, valB);
        }
    } else if (rm.captured(1) == "q") {
        valA = rm.captured(2).isNull() ? "&lt;center&gt;"  : rm.captured(2);
        valB = rm.captured(4).isNull() ? "(&lt;width&gt;)" : rm.captured(4);

        if (rm.captured(2).isNull()) {
            str = QString("Center: <b>%1</b> <font color=\"#777777\">%2</font>").arg(valA, valB);
        } else if (rm.captured(3).isNull()) {
            str = QString("Center: <b>%1</b> <font color=\"#777777\">%2</font>").arg(valA, valB);
        } else if (rm.captured(4).isNull()) {
            str = QString("Center: %1 <b>%2</b>").arg(valA, valB);
        } else {
            str = QString("Zoom: %1 <b>%2</b>").arg(valA, valB);
        }
    } else if (rm.captured(1) == "w")  {
        valA = rm.captured(2).isNull() ? "&lt;width&gt;" : rm.captured(2);
        str = QString("Width: <b>%1</b>").arg(valA);
    } else {
        return;
    }

    QStaticText stxt(str); // required for html text painting
    QTextOption otxt = stxt.textOption();
    otxt.setWrapMode(QTextOption::NoWrap);
    stxt.setTextOption(otxt);

    p.save();

    QPen pen;
    pen.setColor(darkTheme ? global::Functions::colorToDarkMode(axisColor) : axisColor);
    p.setPen(pen);
    p.setFont(fontAxis);

    QFontMetrics fma(fontAxis);

    p.drawStaticText(plot.center().x() - fma.horizontalAdvance("Center: XX.XX XX.XX") / 2,
                     plot.center().y() - fma.descent() / 2,
                     stxt);

    p.restore();
}

/*
 * calculates the margins as a function of font sizes.
 * three constants are required:
 * margin_o: outer margin between border and axis labels / title, will be
 *           set to 0 for exporting bitmaps
 * margin_m: space between axis lables and tick lables
 * margin_i: inner space between lables and axes
 */
void GraphWindow::calcMargins(const QPainter &, bool border)
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

    QString stra = QString("%1").arg(_maxIntensAbs, 0, 'f', 0);
    int spc = margin_o + margin_i;
    int vTickLabelLength = qMax(fmTicks.horizontalAdvance(stra), fmTicks.horizontalAdvance("000000"));
    int spr = int(fmTicks.horizontalAdvance("MMM.MM") / 2.0);

    margin_l = fmAxis.height() + vTickLabelLength + margin_m + spc;
    margin_r = spr + margin_o;
    margin_t = fmTitle.height() + spc;
    margin_b = fmAxis.height() + fmTicks.height() + margin_m + spc;
}

/*
 * calculates the size of the plot area
 */
void GraphWindow::initPlot(const QRect &rc)
{
    canvas = rc;
    plot.setRect(margin_l, margin_t, rc.width() - (margin_r + margin_l), rc.height() - (margin_t + margin_b));
    topMarginRect.setRect(plot.left(), 0, plot.width(), plot.top());
    bottomMarginRect.setRect(plot.left(), plot.bottom(), plot.width(), margin_b);
    leftMarginRect.setRect(0, plot.top(), margin_l, plot.height());
    rightMarginRect.setRect(plot.right(), plot.top(), margin_r, plot.height());
}

void GraphWindow::drawMargin(QPainter &p)
{
    if (!p.isActive()) return;
    p.save();

    p.fillRect(rect(), bgColor);

    if (useBackgroundColors && (marginColor != bgColor)) {
        drawStyledBox(p, rect(), margin_o, marginColor, bgColor);
    }

    p.restore();
}

void GraphWindow::drawStyledBox(QPainter &p, const QRect &box, int wdt, const QColor &colOuter, const QColor &colInner)
{
    int w = 1 * wdt;
    QLinearGradient gradL(0, 0, 1, 0);
    QLinearGradient gradT(0, 0, 0, 1);
    QLinearGradient gradR(1, 1, 0, 1);
    QLinearGradient gradB(0, 1, 0, 0);
    QRadialGradient gradTL(1, 1, 1);
    QRadialGradient gradTR(0, 1, 1);
    QRadialGradient gradBL(1, 0, 1);
    QRadialGradient gradBR(0, 0, 1);

    gradL.setCoordinateMode(QGradient::ObjectMode);
    gradT.setCoordinateMode(QGradient::ObjectMode);
    gradR.setCoordinateMode(QGradient::ObjectMode);
    gradB.setCoordinateMode(QGradient::ObjectMode);
    gradTL.setCoordinateMode(QGradient::ObjectMode);
    gradTR.setCoordinateMode(QGradient::ObjectMode);
    gradBL.setCoordinateMode(QGradient::ObjectMode);
    gradBR.setCoordinateMode(QGradient::ObjectMode);

    gradL.setColorAt(1, colInner);
    gradT.setColorAt(1, colInner);
    gradR.setColorAt(1, colInner);
    gradB.setColorAt(1, colInner);

    gradTL.setColorAt(0, colInner);
    gradTR.setColorAt(0, colInner);
    gradBL.setColorAt(0, colInner);
    gradBR.setColorAt(0, colInner);

    gradL.setColorAt(0, colOuter);
    gradT.setColorAt(0, colOuter);
    gradR.setColorAt(0, colOuter);
    gradB.setColorAt(0, colOuter);

    gradTL.setColorAt(1, colOuter);
    gradTR.setColorAt(1, colOuter);
    gradBL.setColorAt(1, colOuter);
    gradBR.setColorAt(1, colOuter);

    p.fillRect(QRect(box.left(), box.top() + w, w, box.height() - 2*w - 1), gradL);
    p.fillRect(QRect(box.left() + w, box.top(), box.width() - 2*w, w), gradT);
    p.fillRect(QRect(box.right() - w + 1, box.top() + w, w, box.height() - 2*w - 1), gradR);
    p.fillRect(QRect(box.left() + w, box.bottom() - w, box.width() - 2*w, w), gradB);

    p.fillRect(QRect(box.left(), box.top(), w, w), gradTL);
    p.fillRect(QRect(box.right() - w + 1, box.top(), w, w), gradTR);
    p.fillRect(QRect(box.left(), box.height() - w - 1, w, w), gradBL);
    p.fillRect(QRect(box.right() - w + 1, box.height() - w - 1, w, w), gradBR);
}

/*
 * draws axis labels and file name
 */
void GraphWindow::drawWindow(QPainter &p, bool replaceChars)
{
    if (!p.isActive()) return;
    p.save();

    QPen pen;
    QString xa;
    QString ya;
    QString wl = settings->value("graph/showWlInXaxisLabel", false).toBool()
            ? QString(" (%1 = %2 %3)").arg(global::lambda).arg(waveLength[0], 0, 'f', 6).arg(global::angstrom)
            : QString();

    if (xScaling == XSCALED) {
        if (replaceChars) {
            xa = tr("d Spacing [Angstrom]");
        } else {
            xa = tr("d Spacing [") + QString(global::angstrom) + "]";
        }
    } else if (xScaling == XSCALEQ) {
        if (replaceChars) {
            xa = tr("Q [1/Angstrom]");
        } else {
            xa = tr("Q [") + QString(global::angstrom) + QString(global::superMinus) + QString(global::superOne) + "]";
        }
    } else {
        if (replaceChars) {
            xa = tr("Diffraction Angle [degrees 2theta]");
        } else {
            xa = tr("Diffraction Angle [") + QString(global::degree) + "2" + QString(global::theta) + "]" + wl;
        }
    }

    if (useCountsPerSecond && scanControl->first()->timePerStep() > 0.0) {
        ya = QString(tr("Intensity [cps]"));
    } else {
        ya = QString(tr("Intensity [counts]"));
    }

    pen.setWidth(lineWidth);
    pen.setColor(darkTheme ? global::Functions::colorToDarkMode(axisColor) : axisColor);

    QFontMetrics fmt(fontTitle);
    QFontMetrics fma(fontAxis);

    p.setPen(pen);
    p.setFont(fontTitle);
    p.drawText(margin_o, fmt.ascent() + margin_o, scanControl->getSampleId());

    QString displayName = settings->value("graph/drawCompleteFileName", false).toBool()
            ? QDir::toNativeSeparators(scanControl->fileInfo().absoluteFilePath())
            : scanControl->fileInfo().fileName();

    p.drawText(canvas.width() - margin_o - fmt.horizontalAdvance(displayName), fmt.ascent() + margin_o, displayName);

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

void GraphWindow::drawHighlightedRegions(QPainter &p)
{
    if (!highlightRegions.count()) return;
    if (!p.isActive()) return;
    p.save();

    p.translate(plot.x(), plot.y());

    QRegion plotRegion(lineWidth, lineWidth, plot.width() - lineWidth, plot.height() - lineWidth, QRegion::Rectangle);
    p.setClipRegion(plotRegion);
    p.setCompositionMode(compositionModeFill());

    QColor centLineCol = darkTheme ? global::Functions::colorToDarkMode(axisColor) : axisColor;

    for (int i = 0; i < highlightRegions.count(); ++i) {
        for (int j = 0; j < highlightRegions.at(i).count(); ++j) {
            double _sx = getXcoord(highlightRegions.at(i).at(j).start.x());
            double _ex = getXcoord(highlightRegions.at(i).at(j).end.x());

            QPen linePen = highlightRegions.at(i).at(j).centerLinePen;
            linePen.setColor(centLineCol);
            linePen.setWidth(lineWidth);

            // the bounding rect holds screen coordinates. so we must update it in the drawing routine.
            highlightRegions[i][j].boundingRect = QRectF(_sx, 0.0, _ex - _sx, double(plot.height()));
            highlightRegions[i][j].centerLinePen = linePen;

            // drawPlainHightlightedRegion(p, _sx, _ex, highlightRegions[i][j]);
            drawBoxHightlightedRegion(p, _sx, _ex, highlightRegions[i][j]);
            // drawStyledHighlightedRegion(p, _sx, _ex, highlightRegions[i][j]);
        }
    }

    p.restore();
}

void GraphWindow::drawPlainHightlightedRegion(QPainter &p, double _sx, double _ex, global::HighlightRegion &region)
{
    QColor fillCol = darkTheme ? region.fillColor.darker(1500) : region.fillColor;
    p.fillRect(QRectF(_sx, 0.0, _ex - _sx, double(plot.height())), fillCol);

    if (region.centerLine) {
        p.setPen(region.centerLinePen);
        double _cx = (_sx + _ex) / 2.0;
        p.drawLine(QPointF(_cx, 0.0), QPointF(_cx, double(plot.height())));
    }

    if (!region.name.isEmpty() && _ex > 0.0) {
        p.setPen(darkTheme ? Qt::white : Qt::black);
        QFontMetrics fm(font());
        QPointF labelPos(fm.horizontalAdvance("M") + (_sx < 0.0 ? 0.0 : _sx), 2 * fm.ascent());
        QRectF labelRect(fm.boundingRect(labelPos.x(), labelPos.y() - fm.ascent(), 0, 0, Qt::AlignLeft, region.name));
        region.labelBoundingRect = labelRect;
        p.drawText(labelPos, region.name);
    }
}

void GraphWindow::drawBoxHightlightedRegion(QPainter &p, double _sx, double _ex, global::HighlightRegion &region)
{
    QFontMetrics fm(font());
    int hgt = 3 * fm.ascent();
    QColor fillCol = darkTheme ? region.fillColor.darker(1500) : region.fillColor;
    QPen penBox(fillCol);
    penBox.setWidth(2*lineWidth);
    QRectF headerBox(_sx - lineWidth, 0.0, _ex - _sx + 2 * lineWidth, double(hgt));

    p.setPen(penBox);
    p.fillRect(headerBox, fillCol);
    p.drawLine(_sx, hgt + 1, _sx, plot.height());
    p.drawLine(_ex, hgt + 1, _ex, plot.height());

    if (region.centerLine) {
        p.setPen(region.centerLinePen);
        double _cx = (_sx + _ex) / 2.0;
        p.drawLine(QPointF(_cx, 0.0), QPointF(_cx, double(plot.height())));
    }

    if (!region.name.isEmpty() && _ex > 0.0) {
        p.setPen(darkTheme ? Qt::white : Qt::black);
        QPointF labelPos(fm.horizontalAdvance("M") + (_sx < 0.0 ? 0.0 : _sx), 2 * fm.ascent());
        region.labelBoundingRect = headerBox;
        p.drawText(labelPos, region.name);
    }
}

void GraphWindow::drawStyledHighlightedRegion(QPainter &p, double _sx, double _ex, global::HighlightRegion &region)
{
    QRect boxRect(int(_sx), 0, int(_ex - _sx), plot.height());
    drawStyledBox(p, boxRect, margin_o, region.fillColor, bgColor);

    if (region.centerLinePen.color().isValid() && region.centerLine) {
        p.setPen(region.centerLinePen);
        double _cx = (_sx + _ex) / 2.0;
        p.drawLine(QPointF(_cx, 0.0), QPointF(_cx, double(plot.height())));
    }

    if (!region.name.isEmpty() && _ex > 0.0) {
        p.setPen(darkTheme ? Qt::white : Qt::black);
        QFontMetrics fm(font());
        QPointF labelPos(fm.horizontalAdvance("M") + (_sx < 0.0 ? 0.0 : _sx), 2 * fm.ascent());
        QRectF labelRect(fm.boundingRect(labelPos.x(), labelPos.y() - fm.ascent(), 0, 0, Qt::AlignLeft, region.name));
        region.labelBoundingRect = labelRect;
        p.drawText(labelPos, region.name);
    }
}

void GraphWindow::setIntegralRanges(const QList<global::HighlightRegion> &l)
{
    if (highlightRegions.size() < 1) highlightRegions.resize(1, QList<global::HighlightRegion>());
    highlightRegions[0] = l;
}

void GraphWindow::setPeakFitRanges(const QList<global::HighlightRegion> &l)
{
    if (highlightRegions.size() < 2) highlightRegions.resize(2, QList<global::HighlightRegion>());
    highlightRegions[1] = l;
}

/*
 * draws hkl tick marks of scans that contain both xy and hkl data.
 * hkl lines of pure hkl scans (no xy data) will be drawn elsewhere.
 */
void GraphWindow::drawHklTicks(QPainter &p)
{
    tt.clear();

    // painter not active: exit
    if (!p.isActive()) return;

    // no reflection available: exit
    if (!scanControl->count()) return;

    bool drawPhaseHkl = settings->value("graph/phaseVisibilityHkl", true).toBool();
    bool fillHklRect = settings->value("graph/fillActiveHkl", false).toBool();

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
    for (int n = 0; n < scanControl->count(); ++n) {
        int idx = legend->isBottomUp() ? scanControl->count() - n - 1 : n;

            const Scan *scan = scanControl->at(idx);
        if (!scan) continue;

        bool isPhase = scan->scanTypes().testFlag(Scan::PHASE);

        // skip scans that don't contain hkl data or scan data
        if (scan->pDataAngle().isEmpty() || scan->pDataIntensity().isEmpty() || scan->pDataHkl().isEmpty()) {
            continue;
        }

        // skip invisible scans
        if (!scan->isVisible()) {
            continue;
        }

        if (isPhase && !drawPhaseHkl) continue;

        const QVector<Hkl> &vec = scan->pDataHkl();
        int of = vpos * vspacing + top;

        if (darkTheme) {
            pen.setColor(global::Functions::colorToDarkMode(scan->color()));
        } else {
            pen.setColor(scan->color());
        }

        bool convertToTT = scan->getHklXunit() == "dnm";

        if (scan->isActive() && fillActive && fillHklRect) {
            QBrush brush(settings->toFillColor(pen.color(), bgColor));
            p.setCompositionMode(compositionModeFill());
            p.setBrush(brush);
            QPen fillPen(QColor(0, 0, 0, 0));
            fillPen.setWidth(0);
            p.setPen(fillPen);
            p.fillRect(QRect(0, top + of - int(0.25 * len) + lineWidth, plot.width(), int(1.5 * len)), brush);
            p.setCompositionMode(QPainter::CompositionMode_SourceOver);
            p.setBrush(QBrush());
        }

        p.setPen(pen);

        // draw line for reflections
        for (int i = 0; i < vec.size(); ++i) {
            Hkl hkl = vec.at(i);

            if (hkl.status() == 0) {
                continue;
            }

            int x = getXcoord(convertToTT ? global::Functions::dToTwoTheta(hkl.position(), 0.1*waveLength[0]) : hkl.position());

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

            if (tt.value(r.bottom(), QMap<int, tTip>()).contains(r.left())) {
                // if a ttip at position r.left() already exists, just append the new string to
                // the existing ttip's string
                if (!tt.value(r.bottom(), QMap<int, tTip>()).value(r.left()).str.contains(str)) {
                    tt[r.bottom()][r.left()].str.append(str);
                }
            } else {
                // if no ttip exists at position r.left(), add a new one
                tTip _ttip;

                _ttip.pos = r;
                _ttip.str = QStringList(str);

                tt[r.bottom()][r.left()] = _ttip;
            }
        }

        ++vpos;
    }

    p.restore();
}

/*
 * draw scans that only contain hkl lines
 *
 * if a scan contains scan data AND hkl data, the hkl tick lines are drawn at the top of the scan (not here)
 * if it only contains hkl data, the hkl lines are drawn from the base line with correct (normalized) intensities
 * (here).
 */
void GraphWindow::drawHklScans(QPainter &p)
{
    if (!p.isActive()) return;

    for (int i = 0; i < scanControl->count(); ++i) {
        const Scan *scan = scanControl->at(i);
        bool skip = false;

        if (!scan->hasHklData()) skip = true; // skip scans without hkl data
        if (!scan->isVisible())  skip = true; // skip scans set to invisible
        if (scan->hasScanData()) skip = true; // skip scans with xy scan data

        if (!skip) drawHklLines(p, scan, hklScanBase);
    }
}

/*
 * draw hkl lines of the selected reference structure
 * note: reference reflections are stored in d-positions, not 2theta
 */
void GraphWindow::drawHklReferenceLines(QPainter &p)
{
    if (!p.isActive()) return;
    if (!refStructure.hasHklData()) return;

    QColor col(settings->value("graph/hklLineColor", "#009900").toString());

    double yoffset = 0.0;

    if (hklReferenceBase == 1) {
        Scan *actScan = scanControl->firstActiveScan();
        if (actScan) yoffset = actScan->yOffset();
    } else if (hklReferenceBase == 2) {
        yoffset = difOffset;
    }

    refStructure.setColor(darkTheme ? global::Functions::colorToDarkMode(col) : col);
    refStructure.setHklXunit("dnm");
    refStructure.setYoffset(yoffset);
    refStructure.setHklSymbol(settings->value("config/hklSymbol", 0).toInt());

    drawHklLines(p, &refStructure, hklReferenceBase);
}

/*
 * draws hkl lines with correct intensities from the baseline
 */
void GraphWindow::drawHklLines(QPainter &p, const Scan *hklScan, int mode)
{
    if (!hklScan) return;
    Scan *bkgrScan = nullptr;
    if (mode == 1) bkgrScan = scanControl->backgroundScan();

    QList<QList<int> > hklScreenCoordinates = getHklScreenCoordinates(hklScan, bkgrScan, mode);

    p.save();

    QFontMetrics fm(fontLegend);
    int markerSize = fm.ascent()/2;
    int lwidth = hklScan->isActive() ? 2 * lineWidth : lineWidth;
    int hklSymb = settings->value("config/hklSymbol", 0).toInt();

    QPen pen;
    pen.setColor(darkTheme ? global::Functions::colorToDarkMode(hklScan->color()) : hklScan->color());
    pen.setWidth(lwidth);
    p.setPen(pen);
    p.translate(plot.x(), plot.y());
    p.setClipRect(QRect(lwidth, lwidth, plot.width() - lwidth, plot.height() - lwidth));

    for (int i = 0; i < hklScreenCoordinates.size(); ++i) {
        QList<int> hklCoords = hklScreenCoordinates.at(i);
        if (hklCoords.size() < 4) continue;

        Hkl hkl = hklScan->pDataHkl().at(hklCoords.at(0));
        int x  = hklCoords.at(1);
        int y0 = hklCoords.at(2);
        int y1 = hklCoords.at(3);

        // draw highlight marker for active hkl lines
        if (hkl.status() == 2) {
            // use bold lines for hkl lines with status = 2
            pen.setWidth(lwidth == 2 * lineWidth ? lwidth : 2 * lwidth);
            p.setPen(pen);
            p.drawLine(x - markerSize/2, y0 + markerSize, x, y0);
            p.drawLine(x + markerSize/2, y0 + markerSize, x, y0);
            p.drawLine(x - markerSize/2, y1 - markerSize, x, y1);
            p.drawLine(x + markerSize/2, y1 - markerSize, x, y1);
        }

        p.drawLine(x, y0, x, y1);
        int dx =  symbolSize * lineWidth + 1;
        if      (hklSymb == 1) p.drawLine(x - dx, y1, x + dx, y1);
        else if (hklSymb == 2) p.drawEllipse(x - dx, y1 - dx, 4, 4);

        if (hkl.status() == 2) {
            // reset bold lines
            pen.setWidth(lwidth);
            p.setPen(pen);
        }

        QString str;

        if (hkl.phase().isEmpty()) {
            str = QString("Ref: (%1)").arg(hkl.hkl());
        } else {
            str = QString("Ref: %1 (%2)").arg(hkl.phase(), hkl.hkl());
        }

        // create the tool tip
        tTip _ttip;

        _ttip.pos = QRect(x - 2 + plot.x(), plot.y() + y1, 4, y0 - y1);
        _ttip.str = QStringList(str);

        tt[std::numeric_limits<int>::max()][_ttip.pos.left()] = _ttip;
    }

    p.restore();
}

/*
 * returns a list with screen coordinates:
 * (index in original scan 1, x1, y1_bottom, y1_top)
 * (index in original scan 2, x2, y2_bottom, y2_top)
 * ...
 */
QList<QList<int> > GraphWindow::getHklScreenCoordinates(const Scan *hklScan, const Scan *bkgrScan, int mode)
{
    QList<QList<int> > list;
    QList<Hkl> hklDisp;
    QList<double> baseDisp;
    QList<int> xposDisp;

    double normFactor = std::numeric_limits<double>::max();

    // determine normalization factor and store some intermediate results
    for (int i = 0; i < hklScan->pDataHkl().size(); ++i) {
        Hkl hkl = hklScan->pDataHkl().at(i);
        if (hkl.status() == 0) continue; // marked as hidden

        double ttPos = (hklScan->getHklXunit() == "dnm")
                           ? global::Functions::dToTwoTheta(10.0 * hkl.position(), waveLength[0])
                           : hkl.position();

        int x = getXcoord(ttPos + hklScan->xOffset());

        if ((x < 0) || (x > plot.width())) continue;

        double hklIntens = hkl.intensity();
        double baseInt = 0.0;
        double topInt = (mode == 2) ? qAbs(difOffset) : scanMaxIntensity;

        if ((mode == 1) && bkgrScan) {
            baseInt = bkgrScan->intensity(ttPos);
        }

        normFactor = qMin(normFactor, (topInt - baseInt) / hklIntens);

        hklDisp.append(hkl);
        baseDisp.append(baseInt);
        xposDisp.append(x);
    }

    // calculate screen coordinates of visible hkl lines
    for ( int i = 0; i < qMin(hklDisp.size(), qMin(baseDisp.size(), xposDisp.size())); ++i) {
        Hkl hkl = hklDisp.at(i);

        double dy0 = baseDisp.at(i);
        double dy1 = dy0 + hkl.intensity() * normFactor * hklScan->scaleFactor();
        double dyOff = hklScan->yOffset();

        if (yScaling == YSCALESQRT) {
            dy1   = getSqrt(dy1);
            dy0   = getSqrt(dy0);
            dyOff = getSqrt(dyOff);
        } else if (yScaling == YSCALELOG10) {
            dy1   = getLog10(dy1);
            dy0   = getLog10(dy0);
            dyOff = getLog10(dyOff);
        }

        int x  = xposDisp.at(i);
        int y0 = getYcoord(dy0 + dyOff);
        int y1 = getYcoord(dy1 + dyOff);

        QList<int> hklCoords = QList<int>() << i << x << y0 << y1;
        list.append(hklCoords);
    }

    return list;
}

/*
 * draws the plot lines
 */
void GraphWindow::drawPlot(QPainter &p, bool clipManually)
{
    if (!p.isActive()) return;
    p.save();

    int aLineWidth = settings->value("graph/useActiveLineWidth", true).toBool()
            ? settings->value("graph/activeLineWidth", 2).toInt()
            : lineWidth;

    bool drawPhaseScans = settings->value("graph/phaseVisibilityPattern", true).toBool();

    QPainter::CompositionMode fillCompMode = darkTheme ? QPainter::CompositionMode_Plus : QPainter::CompositionMode_Multiply;
    QPolygon bgScreenPolygon = getBackgroundScreenPolygon(clipManually);

    // reset the offset value for the difference curve
    difOffset = 0.0;

    QPen pen;
    pen.setWidth(lineWidth);
    pen.setColor(darkTheme ? Qt::white : Qt::black);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::RoundJoin);

    QBrush brush(QColor(255, 255, 255));

    p.setPen(pen);
    p.setBrush(brush);
    p.translate(plot.x(), plot.y());

    if (qFuzzyIsNull(waveLength[0])) {
        waveLength[0] = settings->defaultWavelength();
    }

    // now clip the drawing rect, so as not to print over the axes
    p.setClipRect(0, 0, plot.width(), plot.height());

    // loop over all scans
    for (int ix = 0; ix <= scanControl->count(); ++ix) {
        const Scan *scan = nullptr;

        if (ix < scanControl->count()) {
            scan = scanControl->at(ix);
        } else {
            if (settings->value("graph/showRefStructureScan", false).toBool()) {
                scan = &refStructure;
            }
        }

        if (!scan) break;

        // skip invisible scans
        if (!scan->isVisible()) continue;

        // skip scans of size 0
        if (!scan->size()) continue;

        pen.setColor(darkTheme ? global::Functions::colorToDarkMode(scan->color()) : scan->color());

        bool isBg = scan->scanTypes().testFlag(Scan::BACKGROUND);
        bool isDiff = scan->scanTypes().testFlag(Scan::DIFF);
        bool isPhase = scan->scanTypes().testFlag(Scan::PHASE);
        bool fillToBackground = scan->scanTypes().testFlag(Scan::ISABOVEBACKGROUND);

        // drawing of phase scans not requested
        if (isPhase && !drawPhaseScans) continue;

        // don't convert the background scan to screen again if we already have done it
        QPolygon polygon = (isBg && bgScreenPolygon.size() > 1) ? bgScreenPolygon : getScanScreenPolygon(scan, clipManually);
        QPolygon fillPoly;

        // only draw the scan if the polygon contains at least 2 points
        if (polygon.size() > 2) {
            // check if this is the active scan. If yes, use double line width
            int cLineWidth = scan->isActive() ? aLineWidth : lineWidth;

            if (scan->isActive() && fillActive) {
                if (!isBg && !isDiff) {
                    if (fillToBackground) {
                        // fill to the background curve
                        fillPoly = appendBackgroundPolygon(polygon, bgScreenPolygon);
                    } else {
                        // fill to the zero line
                        fillPoly = appendBackgroundPolygon(polygon, QPolygon());
                    }

                    brush.setColor(settings->toFillColor(pen.color(), bgColor));
                }

                p.setCompositionMode(fillCompMode);
                p.setBrush(brush);
                QPen fillPen(QColor(0, 0, 0, 0));
                fillPen.setWidth(0);
                p.setPen(fillPen);
                p.drawPolygon(fillPoly);
                p.setCompositionMode(QPainter::CompositionMode_SourceOver);
                p.setBrush(QBrush());
            }

            int sstyle = (int)scan->pointSymbol();

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

            if (ix == scanControl->count()) break;

            // if there are active integral ranges, draw the linear background
            if (highlightRegions.size() > 0) {
                // the y-coordinates of the integral range points have no other meaning than indicating whether or not
                // the background line should be drawn: y > 0 = draw, y <= 0 = don't draw
                for (int i = 0; i < highlightRegions.at(0).size(); ++i) {
                    const global::HighlightRegion hl = highlightRegions.at(0).at(i);
                    if (hl.start.y() <= 0.0) continue;

                    double yoff = scan->yOffset();
                    double intens_s = scan->intensity(scan->indexOfAngle(hl.start.x() + angularCorrection(hl.start.x()), 1));
                    double intens_e = scan->intensity(scan->indexOfAngle(hl.end.x() + angularCorrection(hl.end.x()), 2));

                    if (yScaling == YSCALESQRT) {
                        yoff     = getSqrt(yoff);
                        intens_s = getSqrt(intens_s);
                        intens_e = getSqrt(intens_e);
                    }
                    else if (yScaling == YSCALELOG10) {
                        yoff     = getLog10(yoff);
                        intens_s = getLog10(intens_s);
                        intens_e = getLog10(intens_e);
                    }

                    int x_s = getXcoord(hl.start.x());
                    int x_e = getXcoord(hl.end.x());
                    int y_s = getYcoord(intens_s + yoff);
                    int y_e = getYcoord(intens_e + yoff);

                    p.drawLine(x_s, y_s, x_e, y_e);
                }
            }
        }

        difOffset = qMin(scan->yOffset(), difOffset);
    }

    // draw a horizontal line at yOffset if yOffset is < 0. This is only used to draw a line
    // at the center of the difference curve
    if (difOffset < 0.0) {
        QPen hlinePen(darkTheme ? Qt::white : Qt::black, lineWidth, Qt::DashLine);
        p.setPen(hlinePen);

        double yCenterLine = getYcoord(yValueToScale(difOffset));
        p.drawLine(0, yCenterLine, plot.width(), yCenterLine);
    }

    // draw axes
    pen.setColor(darkTheme ? Qt::white : Qt::black);
    pen.setWidth(lineWidth);
    p.setPen(pen);
    p.setBrush(QBrush());
    // increase the clipped rect
    p.setClipRect(-lineWidth, -lineWidth, plot.width() + 2*lineWidth, plot.height() + 2*lineWidth);
    p.drawRect(0, 0, plot.width() , plot.height());

    p.restore();
}

QPolygon GraphWindow::getScanScreenPolygon(const Scan *scan, bool clipManually)
{
    if (!scan) return QPolygon();
    QPolygon polygon;

    double yoff = scan->yOffset();
    double tps = 1.0;

    if (useCountsPerSecond) {
        tps = scan->timePerStep() > 0.0 ? scan->timePerStep() : 1.0;
    }

    if (yScaling == YSCALELOG10) yoff = getLog10(yoff);
    if (yScaling == YSCALESQRT)  yoff = getSqrt(yoff);

    int firstIdx = 0;
    int lastIdx = scan->size() - 1;

    double leftAng = getRealX(plot.left());
    double rightAng = getRealX(plot.right());

    // loop from the left to determine the start of the visible range
    for (int i = 0; i < scan->size(); ++i) {
        if (scan->angle(i) - angularCorrection(scan->angle(i)) + scan->xOffset() >= leftAng) {
            break;
        }

        firstIdx = i;
    }

    // loop from the right to determine the end of the visible range
    for (int i = scan->size() - 1; i > firstIdx; --i) {
        if (scan->angle(i) - angularCorrection(scan->angle(i)) + scan->xOffset() <= rightAng) {
            break;
        }

        lastIdx = i;
    }

    // loop over the visible range and calculate screen coordinates
    for (int i = firstIdx; i <= lastIdx; ++i) {
        // calculate intensity
        double scint = scan->intensity(i) * scan->scaleFactor() / tps;

        if (yScaling == YSCALESQRT)  scint = getSqrt(scint);
        if (yScaling == YSCALELOG10) scint = getLog10(scint);

        // calculate screen coordinates
        int x = getXcoord(scan->angle(i) + scan->xOffset() - angularCorrection(scan->angle(i)));
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

QPolygon GraphWindow::getBackgroundScreenPolygon(bool clipManually)
{
    if (fillActive && (scanControl->hasActiveScan())) {
        for (int i = 0; i < scanControl->count(); ++i) {
            const Scan *scan = scanControl->at(i);
            if (!scan) continue;
            if (scan->scanTypes().testFlag(Scan::BACKGROUND)) return getScanScreenPolygon(scan, clipManually);
        }
    }

    return QPolygon();
}

QPolygon GraphWindow::appendBackgroundPolygon(const QPolygon &sc, const QPolygon &bg)
{
    QPolygon poly = sc;

    if (bg.size() < 2) {
        int fx = sc.first().x();
        int lx = sc.last().x();
        int fy = getYcoord(0.0);

        poly.prepend(QPoint(fx, fy));
        poly.append(QPoint(lx, fy));
    } else {
        for (int i = bg.size() - 1; i >= 0; --i) {
            poly.append(bg.at(i));
        }
    }

    return poly;
}

void GraphWindow::drawAnchorPoints(QPainter &p)
{
    if (anchorPoints.isEmpty()) return;
    if (!scanControl->count()) return;
    if (!p.isActive()) return;
    p.save();

    int dia = int(pScale * 5);

    const Scan *scan = scanControl->last();

    // reset the offset value for the difference curve
    difOffset = 0.0;

    QPen pen;
    pen.setWidth(lineWidth);
    pen.setColor(darkTheme ? global::Functions::colorToDarkMode(scan->color()) : scan->color());
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::RoundJoin);

    p.setPen(pen);
    p.translate(plot.x(), plot.y());

    QBrush brush(scan->color(), Qt::SolidPattern);

    // now clip the drawing rect, so as not to print over the axes
    p.setClipRect(0, 0, plot.width() + lineWidth, plot.height() + lineWidth);

    double yoff = scan->yOffset();
    if (yScaling == YSCALELOG10) yoff = getLog10(yoff);
    if (yScaling == YSCALESQRT)  yoff = getSqrt(yoff);

    double tps = 1.0;

    if (useCountsPerSecond) {
        tps = scan->timePerStep() > 0.0 ? scan->timePerStep() : 1.0;
    }

    int firstIdx = 0;
    int lastIdx = anchorPoints.size() - 1;

    double leftAng = getRealX(plot.left());
    double rightAng = getRealX(plot.right());

    // loop from the left to determine the start of the visible range
    for (int i = 0; i < anchorPoints.size(); ++i) {
        if (anchorPoints.at(i).angle >= leftAng) {
            break;
        }

        firstIdx = i;
    }

    // loop from the right to determine the end of the visible range
    for (int i = anchorPoints.size() - 1; i > firstIdx; --i) {
        if (anchorPoints.at(i).angle <= rightAng) {
            break;
        }

        lastIdx = i;
    }

    // loop over the visible range and calculate screen coordinates
    for (int i = firstIdx; i <= lastIdx; ++i) {
        // calculate intensity
        double ancint = anchorPoints.at(i).intensity * scan->scaleFactor() / tps;

        if (yScaling == YSCALESQRT)  ancint = getSqrt(ancint);
        if (yScaling == YSCALELOG10) ancint = getLog10(ancint);

        // calculate screen coordinates
        int x = getXcoord(anchorPoints.at(i).angle + scan->xOffset());
        int y = getYcoord(ancint + yoff);

        if (anchorPoints.at(i).highlight) p.setBrush(brush);
        else                              p.setBrush(Qt::NoBrush);

        p.drawEllipse(QPoint(x, y), dia, dia);
    }

    p.restore();
}

/*
 * draws the legend
 */
void GraphWindow::drawLegend(QPainter &p)
{
    legend->redraw(p, fontLegend, bgColor, plot);
}

/*
 * returns a tooltip at position pt
 *
 * QMap<int, QMap<int, tTip> > tt stores tool tips as follows:
 *      y_bottom  x    tTip
 */
QString GraphWindow::tip(const QPoint &pt) const
{
    int yIdx = -1;

    // locate the phase with bottom y coordinate of the hkl lines just below the cursor
    QMapIterator<int, QMap<int, tTip> > itPhase(tt);
    while (itPhase.hasNext()) {
        itPhase.next();

        if (pt.y() <= itPhase.key()) {
            yIdx = itPhase.key();
            break;
        }
    }

    if (yIdx < 0) return QString();

    // now locate the rect containing the cursor in the phase
    QMapIterator<int, tTip> itLine(tt.value(yIdx));
    while (itLine.hasNext()) {
        itLine.next();
        if (itLine.value().pos.contains(pt)) {
            return itLine.value().str.join("\n");
        }
    }

    return QString();
}

QString GraphWindow::getXaxisUnit()
{
    if (scanControl->hasData()) return scanControl->first()->xAxisLabel();

    QString xa = "Diffraction Angle [" + QString(global::degree) + "2" + QString(global::theta) + "]";
    return xa;
}

/*
 * this function accepts an angle and returns it
 * in *widget* coordinates
 */
int GraphWindow::getXcoord(double i) const
{
    return int(plot.width() * (i - _minAngDisp) / (_maxAngDisp - _minAngDisp));
}

/*
 * this function accepts an intensity and returns it
 * in *widget* coordinates
 */
int GraphWindow::getYcoord(double i) const
{
    return int(plot.height() - plot.height() * (i - _minIntensDisp ) / (_maxIntensDisp - _minIntensDisp));
}

/*
 * conversion from *widget* X coordinate to angle
 */
double GraphWindow::getRealX(int i) const
{
    return _minAngDisp + (_maxAngDisp - _minAngDisp) * (i - margin_l) / plot.width();
}

/*
 * conversion from *widget* Y coordinate to intensity
 * if dif = false: returns negative coordinate below the zero line
 * if dif = true:  returns coordinate relative to the zero line of the difference curve
 */
double GraphWindow::getRealY(int i, bool dif) const
{
    double y = _minIntensDisp + double(plot.height() - i + margin_t) * (_maxIntensDisp - _minIntensDisp) / plot.height();

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
 * shows the tooltip if placed on a reflection
 */
bool GraphWindow::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        return showToolTip(e);
    }

    e->ignore();
    return QWidget::event(e);
}

bool GraphWindow::showToolTip(QEvent *e)
{
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);
    QString s = tip(helpEvent->pos());

    if (!s.isNull()) {
        QToolTip::showText(helpEvent->globalPos(), s);
        e->accept();
    } else {
        const Scan *scan = getNearestScan(helpEvent->pos());

        if (scan) {
            QToolTip::showText(helpEvent->globalPos(), scanControl->scanName(scan));
            e->accept();
        } else {
            QToolTip::hideText();
            e->ignore();
        }
    }

    return true;
}

void GraphWindow::mousePressEvent(QMouseEvent *e)
{
    clickedPosition = QPoint(int(e->position().x() + 0.5), int(e->position().y() + 0.5));

    if (e->button() == Qt::LeftButton) {
        mouseButtonLeft(e);
    } else if (e->button() == Qt::RightButton) {
        mouseButtonRight(e);
    } else if (e->button() == Qt::MiddleButton) {
        mouseButtonMiddle(e);
    } else {
        e->ignore();
    }
}

void GraphWindow::mouseReleaseEvent(QMouseEvent *e)
{
    clickedPosition = QPoint();

    if (drawRubberBand) {
        mouseButtonStopRubberBand(e);
    }

    if (middleMouseButtonDragging) {
        mouseButtonStopStruScaling(e);
    }

    if (dragging) {
        mouseButtonStopDragging(e);
    }

    e->ignore();
}

void GraphWindow::mouseMoveEvent(QMouseEvent *e)
{
    // no scans: do nothing
    if (!scanControl->count()) {
        e->ignore();
        return;
    }

    broadcastCoordinates(e->pos());

    if (drawRubberBand) {
        mouseMoveRubberBand(e);
    }

    if (middleMouseButtonDragging) {
        if (anchorPoints.size()) {
            mouseMoveAnchorPointDragging(e);
        } else {
            mouseMoveStrucScaling(e);
        }
    }

    if (dragging) {
        mouseMoveDragging(e);
    }

    if (crossHair || noiseCursor || specLines || inspector || rangeSelectMode || (peakPreviewMode != PPMNONE)) {
        // special cursors only need a buffered update
        update();
    }

    e->accept();
}

void GraphWindow::wheelEvent(QWheelEvent *e)
{
    if (!plot.contains(e->position().toPoint())) return;

    if (e->modifiers().testFlag(Qt::NoModifier)) {
        zoomToPoint(e->position().toPoint(), e->angleDelta().y(), true, false);
        e->accept();
        return;
    }

    if (e->modifiers().testFlag(Qt::ControlModifier)) {
        zoomToPoint(e->position().toPoint(), e->angleDelta().y(), false, true);
        e->accept();
        return;
    }

    e->ignore();
}

void GraphWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        if (peakPreviewMode != PPMNONE) {
            if (firstDoubleClickPoint.isNull()) {
                firstDoubleClickPoint = e->pos();
            } else {
                double ptD  = 0.0; // unused
                double ptAx = 0.0;
                double ptAy = 0.0;
                double ptBx = 0.0;
                double ptBy = 0.0;
                calcCoordinates(firstDoubleClickPoint, ptAx, ptAy, ptD);
                calcCoordinates(e->pos(),              ptBx, ptBy, ptD);
                emit sigPeakPreviewPoints(QPointF(ptAx, ptAy), QPointF(ptBx, ptBy), peakPreviewMode, rangeCallerUid, true);
                firstDoubleClickPoint = QPoint();
                emit sigCursorMessage(QString(), rangeCallerUid);
                e->accept();
                update();
            }
        } else if (rangeSelectMode) {
            if (firstDoubleClickPoint.isNull()) {
                firstDoubleClickPoint = e->pos();
            } else {
                double ptD  = 0.0; // unused
                double ptAx = 0.0;
                double ptAy = 0.0;
                double ptBx = 0.0;
                double ptBy = 0.0;
                calcCoordinates(firstDoubleClickPoint, ptAx, ptAy, ptD);
                calcCoordinates(e->pos(),              ptBx, ptBy, ptD);
                emit sigRangePoints(QPointF(ptAx, ptAy), QPointF(ptBx, ptBy), rangeCallerUid);
                firstDoubleClickPoint = QPoint();
                emit sigCursorMessage(QString(), rangeCallerUid);
                e->accept();
                update();
            }
        } else {
            const Scan *aScan = scanControl->firstActiveScan();
            if (!aScan) return;

            bool onRegionLabel = false;
            QPoint clickPos(e->pos().x() - plot.x(), e->pos().y() - plot.y());
            const global::HighlightRegion *hlRegLblClicked = nullptr;
            int regGroup = -1; // 0 = integrals, 1 = curve fits
            int regNum   = -1;

            for (int i = 0; i < highlightRegions.size(); ++i) {
                for (int j = 0; j < highlightRegions.at(i).size(); ++j) {
                    if (highlightRegions.at(i).at(j).labelBoundingRect.contains(clickPos)) {
                        hlRegLblClicked = &(highlightRegions.at(i).at(j));
                        onRegionLabel = true;
                        regGroup = i;
                        regNum   = j;
                        break;
                    }
                }

                if (onRegionLabel) break;
            }

            if (onRegionLabel && hlRegLblClicked) {
                double sx = hlRegLblClicked->start.x();
                double ex = hlRegLblClicked->end.x();
                double sy = aScan->mid(sx, ex).minIntensity();
                double ey = aScan->mid(sx, ex).maxIntensity();
                Q_UNUSED(sy);

                setZoomRange(sx, ex, 0.0, 1.05 * ey, true);
                emit sigRangeDoubleClicked(regGroup, regNum);
                e->accept();
                return;
            } else {
                double x = 0.0;
                double y = 0.0;
                double d = 0.0;

                calcCoordinates(e->pos(), x, y, d);

                // We emit two different signals, depending on whether Ctrl is pressed or not.
                // These can be used for different actions in the parent object.
                // Also distinguish whether or not the cursor position was inside the plot area
                if (e->modifiers() == Qt::NoModifier) {
                    if (plot.contains(e->pos())) emit sigDoubleClickA(x, y, d);
                }
                if (e->modifiers() == Qt::ControlModifier) {
                    if (plot.contains(e->pos())) emit sigDoubleClickB(x, y, d);
                }

                e->accept();
                return;
            }
        }
    } else {
        e->ignore();
    }
}

void GraphWindow::mouseButtonLeft(QMouseEvent *e)
{
    if (e->modifiers().testFlag(Qt::AltModifier)) {
        mouseButtonStartStrucScaling(e);
    } else if (e->modifiers().testFlag(Qt::ControlModifier)) {
        mouseButtonStartDragging(e);
    } else if (e->modifiers().testFlag(Qt::ShiftModifier)) {
        selectClickedScan(e->pos());
    } else if (e->modifiers().testFlag(Qt::NoModifier)) {
        if (plot.contains(e->pos())) {
            mouseButtonStartRubberBand(e);
        }
    }
}

void GraphWindow::mouseButtonMiddle(QMouseEvent *e)
{
    if (e->modifiers().testFlag(Qt::NoModifier)) {
        mouseButtonStartStrucScaling(e);
    }
}

void GraphWindow::mouseButtonRight(QMouseEvent *e)
{
    if (e->modifiers().testFlag(Qt::NoModifier)) {
        if (plot.contains(e->pos())) {
            if (legend->boundingRect().contains(e->pos())) {
                legend->showContextMenu(e);
            } else {
                rightClickPlot(e);
            }
            e->accept();
        } else if (leftMarginRect.contains(e->pos())) {
            rightClickLeftMargin(e);
            e->accept();
        } else if (bottomMarginRect.contains(e->pos())) {
            rightClickBottomMargin(e);
            e->accept();
        }
    } else if (e->modifiers().testFlag(Qt::ControlModifier)) {
        if (anchorPoints.size()) {
            mouseButtonRemoveAnchor(e);
        }
    }
}

void GraphWindow::rightClickPlot(QMouseEvent *)
{
    bool fullUpdate = false;

    if (refStructure.hasHklData()) {
        if (!qFuzzyCompare(1.0, refStructure.scaleFactor())) {
            refStructure.setScaleFactor(1.0);
            prevStrucScaleFactor = 1.0;
            fullUpdate = true;
        }
    }

    QVector<Scan *> aScans = scanControl->activeScans();

    for (int i = 0; i < aScans.size(); ++i) {
        if (!qFuzzyCompare(1.0, aScans.at(i)->scaleFactor())) {
            aScans[i]->setScaleFactor(1.0);
            fullUpdate = true;
        }
    }

    if (fullUpdate) {
        scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
    } else {
        resetZoom();
        forceUpdate();
    }
}

void GraphWindow::rightClickLeftMargin(QMouseEvent *e)
{
    contextMenuYaxis->exec(e->globalPosition().toPoint());
}

void GraphWindow::rightClickBottomMargin(QMouseEvent *e)
{
    contextMenuXaxis->exec(e->globalPosition().toPoint());
}

void GraphWindow::mouseButtonStartStrucScaling(QMouseEvent *e)
{
    if (anchorPoints.size()) {
        anchorDragged = getNearestAnchorIndex(clickedPosition);
        if (anchorDragged >= 0) {
            anchorStartPosition = QPointF(anchorPoints.at(anchorDragged).angle, anchorPoints.at(anchorDragged).intensity);
        }
    } else if (scanControl->hasActiveScan()) {
        prevScanScaleFactor = scanControl->firstActiveScan()->scaleFactor();
    }

    middleMouseButtonDragging = true;
    e->accept();
}

void GraphWindow::mouseButtonStartDragging(QMouseEvent *e)
{
    pointMouseOperationStart = e->pos();
    dragging = true;
    e->accept();
}

void GraphWindow::mouseButtonStartRubberBand(QMouseEvent *e)
{
    pointMouseOperationStart = e->pos();
    rubberBand->setGeometry(QRect(e->pos(), QSize()));
    rubberBand->show();
    drawRubberBand = true;
    e->accept();
}

void GraphWindow::mouseButtonStopStruScaling(QMouseEvent *e)
{
    middleMouseButtonDragging = false;

    if (anchorDragged >= 0) {
        emit sigAnchorMoved(anchorDragged, anchorPoints.at(anchorDragged).angle, anchorPoints.at(anchorDragged).intensity);
    }
    anchorDragged = -1;

    prevStrucScaleFactor = refStructure.scaleFactor();
    e->accept();
}

void GraphWindow::mouseButtonStopDragging(QMouseEvent *e)
{
    dragging = false;
    e->accept();
}

void GraphWindow::mouseButtonStopRubberBand(QMouseEvent *e)
{
    drawRubberBand = false;
    rubberBand->hide();
    zoom(pointMouseOperationStart, e->pos());
    e->accept();
}

void GraphWindow::mouseButtonRemoveAnchor(QMouseEvent *)
{
    int idx = getNearestAnchorIndex(clickedPosition);
    if (idx >= 0) emit sigAnchorRemove(idx);
}

void GraphWindow::mouseMoveStrucScaling(QMouseEvent *e)
{
    if (clickedPosition.isNull()) return;

    int y0 = clickedPosition.y();
    int zeroLine = getYcoord(0.0);
    int diffLine = getYcoord(difOffset);

    int baseline = margin_t + plot.height();

    if      (y0 < diffLine) baseline = diffLine;
    else if (y0 < zeroLine) baseline = zeroLine;

    double deltaY = double(baseline - e->position().y() + margin_t) / double(baseline - y0 + margin_t);

    QVector<Scan*> aScans = scanControl->activeScans();

    if (aScans.size()) {
        for (int i = 0; i < aScans.size(); ++i) {
            double sfac = prevScanScaleFactor * deltaY;
            aScans[i]->setScaleFactor(sfac);
        }

        scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
    } else if (refStructure.hasHklData()) {
        double sfac = prevStrucScaleFactor * deltaY;
        refStructure.setScaleFactor(sfac);
        scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
    }
}

void GraphWindow::mouseMoveAnchorPointDragging(QMouseEvent *e)
{
    if (clickedPosition.isNull()) return;
    if (anchorDragged < 0) return;

    double x0 = getRealX(clickedPosition.x());
    double y0 = getRealY(clickedPosition.y());
    double x1 = getRealX(int(e->position().x() + 0.5));
    double y1 = getRealY(int(e->position().y() + 0.5));

    double xpos = anchorStartPosition.x() - x0 + x1;
    double ypos = anchorStartPosition.y() - y0 + y1;

    anchorPoints[anchorDragged].angle     = xpos;
    anchorPoints[anchorDragged].intensity = ypos;

    forceUpdate();
}

int GraphWindow::getNearestAnchorIndex(const QPoint p)
{
    int maxDist = int(pScale * 5);
    int idx = -1;
    double minDist = std::numeric_limits<double>::max();

    for (int i = 0; i < anchorPoints.size(); ++i) {
        double dx = double(getXcoord(anchorPoints.at(i).angle) - (p.x() - margin_l));
        double dy = double(getYcoord(anchorPoints.at(i).intensity) - (p.y() - margin_t));

        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist < minDist) {
            idx = i;
            minDist = dist;
        }
    }

    if (minDist <= maxDist) return idx;
    return -1;
}

void GraphWindow::mouseMoveDragging(QMouseEvent *e)
{
    int dx = pointMouseOperationStart.x() - e->pos().x();
    int dy = pointMouseOperationStart.y() - e->pos().y();

    double startX = getRealX(margin_l + dx);
    double startY = getRealY(margin_t + plot.height() + dy);
    double endX   = getRealX(margin_l + plot.width()  + dx);
    double endY   = getRealY(margin_t + dy);

    double limYstart = yScaleToValue(_minIntensAbs);
    double limYend   = yScaleToValue(_maxIntensAbs);

    if ((startX < _minAngAbs) || (endX > _maxAngAbs)) {
        startX = _minAngDisp;
        endX   = _maxAngDisp;
    }

    if ((startY < limYstart) || (endY > limYend)) {
        startY = yScaleToValue(_minIntensDisp);
        endY   = yScaleToValue(_maxIntensDisp);
    }

    setZoomRange(startX, endX, startY, endY, true);
    pointMouseOperationStart = e->pos();
}

void GraphWindow::mouseMoveRubberBand(QMouseEvent *e)
{
    rubberBand->setGeometry(QRect(pointMouseOperationStart, e->pos()).normalized());
}

void GraphWindow::selectClickedScan(const QPoint &p)
{
    int n = getNearestScanIndex(p);

    if (n >= 0) {
        QList<int> activeIdx = scanControl->activeScanIncides();

        if (activeIdx.contains(n)) {
            activeIdx.removeAll(n);
        } else {
            activeIdx.append(n);
        }
        scanControl->setActiveScans(activeIdx);
    } else {
        scanControl->clearActiveScans();
    }
    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

int GraphWindow::getNearestScanIndex(const QPoint &p)
{
    QFontMetrics fml(fontLegend);
    double rWidth = double(fml.horizontalAdvance("m"));
    double angle = 0.0;
    double intens = 0.0;
    double dummyD = 0.0;

    calcCoordinates(QPoint(p.x(), p.y()), angle, intens, dummyD);
    double aCorr = angularCorrection(angle);

    QMap<double, int> dist;

    for (int i = 0; i < scanControl->count(); ++i) {
        const Scan *scan = scanControl->at(i);
        int idxA = scan->indexOfAngle(angle - scan->xOffset() + aCorr, 1);
        int idxB = scan->indexOfAngle(angle - scan->xOffset() + aCorr, 2);

        if (idxA < 0 || idxB < 0) continue;

        double dmin = std::numeric_limits<double>::max();
        QPoint pA = getScreenCoordinates(scan->angle(idxA), scan->intensity(idxA), scan);
        QPoint pB = getScreenCoordinates(scan->angle(idxB), scan->intensity(idxB), scan);
        double d = global::Functions::distanceFromLine(p, pA, pB, 1);
        if (d >= 0.0) dmin = qMin(dmin, d);

        if (idxA > 0) { // we also test the segment to the left
            pA = getScreenCoordinates(scan->angle(idxA-1), scan->intensity(idxA-1), scan);
            pB = getScreenCoordinates(scan->angle(idxA),   scan->intensity(idxA), scan);
            d = global::Functions::distanceFromLine(p, pA, pB, 1);
            if (d >= 0.0) dmin = qMin(dmin, d);
        }

        if (idxB < scan->size() - 1) { // we also test the segment to the right
            pA = getScreenCoordinates(scan->angle(idxB),   scan->intensity(idxB), scan);
            pB = getScreenCoordinates(scan->angle(idxB+1), scan->intensity(idxB+1), scan);
            d = global::Functions::distanceFromLine(p, pA, pB, 1);
            if (d >= 0.0) dmin = qMin(dmin, d);
        }

        if (dmin < rWidth) dist[dmin] = i;
    }

    if (dist.size()) return dist.first();
    return -1;
}

const Scan * GraphWindow::getNearestScan(const QPoint &p)
{
    return scanControl->at(getNearestScanIndex(p));
}

QPoint GraphWindow::getScreenCoordinates(double x, double y, const Scan *s) const
{
    double xoff = s->xOffset();
    double yoff = s->yOffset();

    int px = plot.x() + getXcoord(x + xoff - angularCorrection(x));
    int py = plot.y();

    if (yScaling == YSCALESQRT) {
        yoff = getSqrt(yoff);
        py += getYcoord(getSqrt(y + yoff));
    } else if (yScaling == YSCALELOG10) {
        yoff = getLog10(yoff);
        py += getYcoord(getLog10(y + yoff));
    } else {
        py += getYcoord(y + yoff);
    }

    return QPoint(px, py);
}

void GraphWindow::broadcastCoordinates(const QPoint &p)
{
    if (updateCoordinateTimer.isActive()) return;

    double x = 0.0;
    double y = 0.0;
    double d = 0.0;

    calcCoordinates(p, x, y, d);
    emit sigCoordinates(x, y, d);

    // block coordinate updates to reduce CPU load
    // note: intervals > 50 ms will be noticably choppy
    updateCoordinateTimer.start(40);
}

void GraphWindow::keyPressEvent(QKeyEvent *e)
{
    keyHandler->keyPressed(e);
}

void GraphWindow::keyReleaseEvent(QKeyEvent *e)
{
    keyHandler->keyReleased(e);
}

void GraphWindow::applyKeySequence(const QString &s)
{
    QRegularExpressionMatch rm;
    static QRegularExpression rxZoomAng("^za?\\s*([\\d\\.]+|[mc])(\\s+[\\d\\.]+|\\s*[mc])");
    static QRegularExpression rxZoomInt("^zi\\s*([\\d\\.]+|[mcn])(\\s+[\\d\\.]+|\\s*[mcn])");
    static QRegularExpression rxCenterAng("^q\\s*([\\d\\.]+)(?:\\s+([\\d\\.]+))?");
    static QRegularExpression rxWidthAng("^w\\s*([\\d\\.]+)");

    rm = rxZoomAng.match(s);
    if (rm.hasMatch()) {
        applyZoomAngleKeySequence(rm.captured(1), rm.captured(2));
        return;
    }

    rm = rxZoomInt.match(s);
    if (rm.hasMatch()) {
        applyZoomIntensityKeySequence(rm.captured(1), rm.captured(2));
        return;
    }

    rm = rxCenterAng.match(s);
    if (rm.hasMatch()) {
        applyCenterAngleKeySequence(rm.captured(1), rm.captured(2));
        return;
    }

    rm = rxWidthAng.match(s);
    if (rm.hasMatch()) {
        applyWidthAngleKeySequence(rm.captured(1));
        return;
    }
}

void GraphWindow::applyZoomAngleKeySequence(const QString &mi, const QString &ma)
{
    bool miOk, maOk;
    double aMin = mi.toDouble(&miOk);
    double aMax = ma.toDouble(&maOk);
    if (!miOk) aMin = (mi == "m") ? _minAngAbs : _minAngDisp;
    if (!maOk) aMax = (ma == "m") ? _maxAngAbs : _maxAngDisp;

    setZoomRange(aMin, aMax, _minIntensDisp, _maxIntensDisp, true);
}

void GraphWindow::applyZoomIntensityKeySequence(const QString &mi, const QString &ma)
{
    bool miOk, maOk;
    double iMin = mi.toDouble(&miOk);
    double iMax = ma.toDouble(&maOk);

    // "m" = maximum value
    // "n" = normalize in visible range
    // "c" = current value (fallback)

    if (!miOk) {
        if (mi.trimmed() == "m")      iMin = _minIntensAbs;
        else if (mi.trimmed() == "n") {
                                      double d = getVisibleMinIntensity();
                                      iMin = ((d < 0.0) ? 1.1 : 0.9) * d;
        }
        else                          iMin = _minIntensDisp;
    }

    if (!maOk) {
        if (ma.trimmed() == "m")      iMax = _maxIntensAbs;
        else if (ma.trimmed() == "n") iMax = 1.1 * getVisibleMaxIntensity();
        else                          iMax = _maxIntensDisp;
    }

    setZoomRange(_minAngDisp, _maxAngDisp, iMin, iMax, true);
}

void GraphWindow::applyCenterAngleKeySequence(const QString &mc, const QString &mw)
{
    bool mcOk, mwOk;
    double aCent = mc.toDouble(&mcOk);
    double aWid  = mw.toDouble(&mwOk);

    if (!mcOk) {
        // if the center angle wasn't matched, we bail out
        return;
    }

    if (!mwOk) {
        // if the width wasn't matched, we fall back to default values
        if (qFuzzyCompare(_minAngDisp, _minAngAbs) && qFuzzyCompare(_maxAngDisp, _maxAngAbs)) {
            // not zoomed horizontally: default to 2.0 deg
            aWid = 2.0; //
        } else {
            // zoomed horizontally: keep the current width
            aWid = _maxAngDisp - _minAngDisp;
        }
    }

    // check limits here, because the check in setZoomRange may fail
    // if both angles are out of range
    aCent = qMin(aCent - 0.5 * aWid, _maxAngAbs);
    aCent = qMax(aCent + 0.5 * aWid, _minAngAbs);

    double aMin = aCent - 0.5 * aWid;
    double aMax = aCent + 0.5 * aWid;

    setZoomRange(aMin, aMax, _minIntensDisp, _maxIntensDisp, true);
}

void GraphWindow::applyWidthAngleKeySequence(const QString &mw)
{
    bool mwOk;
    double aWid  = mw.toDouble(&mwOk);

    if (!mwOk) return;

    double aCent = (_minAngDisp + _maxAngDisp) / 2.0;
    double aMin = aCent - 0.5 * aWid;
    double aMax = aCent + 0.5 * aWid;

    setZoomRange(aMin, aMax, _minIntensDisp, _maxIntensDisp, true);
}

/*
 * calculates the real coordinates of widget point p.
 * returns x (in x-axis units), y (in y-axis units), and d (in A)
 */
void GraphWindow::calcCoordinates(const QPoint &p, double &x, double &y, double &d)
{
    // calculate the coordinates and emit a signal
    if (plot.contains(p)) {
        x = getRealX(p.x());

        // avoid division by 0
        if (!qFuzzyIsNull(x)) {
            d = 10.0 * global::Functions::twoThetaToD(x, waveLength[0] / 10.0);
        }

        y = getRealY(p.y(), true);
    }
}

/*
 * zoom region between pa and pb (in widget coordinates)
 */
void GraphWindow::zoom(QPoint pa, QPoint pb)
{
    // request a minimum difference, otherwise do nothing
    int limit = 5;

    setFocus();

    if ((abs(long(pa.x() - pb.x())) <= limit)
        || (abs(long(pa.y() - pb.y())) <= limit)) {
        return;
    }

    double rpax = getRealX(pa.x());
    double rpay = getRealY(pa.y());
    double rpbx = getRealX(pb.x());
    double rpby = getRealY(pb.y());

    setZoomRange(rpax, rpbx, rpay, rpby, true);
}

/*
 * zoom a certain step into the graph
 *
 * p = mouse position, or center of plot
 * i = number of zoom steps, can be > 0 or < 0
 *
 * horizontal: set to true to zoom in horizontal direction
 * vertical:   set to true to zoom in vertical direction
 *
 * both can be true at the same time
 */
void GraphWindow::zoomToPoint(QPoint p, int i, bool horizontal, bool vertical)
{
    double base = 1.0;  // fraction which is multiplied, increase to increase the zoom step

    int x_a = plot.left();
    int x_b = plot.right() + 1;
    int y_a = plot.top();
    int y_b = plot.bottom() + 1;

    if (horizontal) {
        x_a += int(i * base * (double(p.x() - plot.left()) / double(plot.width())));
        x_b -= int(i * base * (double(plot.width() - p.x() + plot.left()) / double(plot.width())));
    }

    if (vertical) {
        y_a += int(i * base * (double(p.y() - plot.left()) / double(plot.height())));
        y_b -= int(i * base * (double(plot.height() - p.y() + plot.top()) / double(plot.height())));
    }

    QPoint pa(x_a, y_a);
    QPoint pb(x_b, y_b);
    zoom(pa, pb);
}

void GraphWindow::resetZoom()
{
    calcLimits();

    _maxAngDisp = _maxAngAbs;
    _minAngDisp = _minAngAbs;
    _maxIntensDisp = _maxIntensAbs;
    _minIntensDisp = _minIntensAbs;

    zoomed = false;
}

void GraphWindow::resetZoomX()
{
    calcLimits();

    _maxAngDisp = _maxAngAbs;
    _minAngDisp = _minAngAbs;

    if (qFuzzyCompare(_maxIntensDisp, _maxIntensAbs) && qFuzzyCompare(_minIntensDisp, _minIntensAbs)) {
        zoomed = false;
    } else {
        zoomed = true;
    }

    forceUpdate();
}

void GraphWindow::resetZoomY()
{
    calcLimits();

    _minIntensDisp = _minIntensAbs;
    _maxIntensDisp = _maxIntensAbs;

    if (qFuzzyCompare(_maxAngDisp, _maxAngAbs) && qFuzzyCompare(_minAngDisp, _minAngAbs)) {
        zoomed = false;
    } else {
        zoomed = true;
    }

    forceUpdate();
}

/*
 * calculate max and min angle and intensity of all scans
 */
void GraphWindow::calcLimits()
{
    if (!scanControl->count()) {
        _minIntensAbs = 0.0;
        _maxIntensAbs = 105.0;
        scanMaxIntensity = 100.0;
        _minAngAbs = 5.0;
        _maxAngAbs = 60.0;
        return;
    }

    _minIntensAbs = 0.0;
    _maxIntensAbs = 0.0;
    scanMaxIntensity = 0.0;
    _minAngAbs = std::numeric_limits<double>::max();
    _maxAngAbs = 0.0;

    bool hasXyData = scanControl->hasScanData();

    for (int i = 0; i < scanControl->count(); i++) {
        const Scan *scan = scanControl->at(i);
        if (!scan) continue;

        double _xmin = std::numeric_limits<double>::max();
        double _xmax = 0.0;
        double _ymin = 0.0;
        double _ymax = 0.0;

        if (scan->hasScanData()) {
            ScanOps::scanMetrics(*scan, _xmin, _xmax, _ymin, _ymax);
        } else if (scan->hasHklData() && !hasXyData) {
            // if other scans have XY data, we completely ignore those only containing Hkl data,
            // otherwise the zooming behaviour feels wrong
            ScanOps::hklMetricsTt(*scan, _xmin, _xmax, _ymin, _ymax, 0.1 * waveLength[0]);
            _xmin -= 2.0;
            _xmax += 2.0;
            if (_xmin < 0.0)   _xmin = 0.0;
            if (_xmax > 180.0) _xmax = 180.0;
        } else {
            continue;
        }

        double cps = 1.0;
        if (useCountsPerSecond) {
            cps = scan->timePerStep() < 0.0 ? 1.0 : scan->timePerStep();
        }

        _minAngAbs = qMin(_minAngAbs, _xmin + scan->xOffset());
        _maxAngAbs = qMax(_maxAngAbs, _xmax + scan->xOffset());

        double scMinInt = _ymin / cps;
        double scMaxInt = _ymax / cps;
        scanMaxIntensity = qMax(scanMaxIntensity, scMaxInt);

        double yoff = scan->yOffset();
        double scint_i = scMinInt;
        double scint_a = scMaxInt;

        if (yScaling == YSCALESQRT) {
            yoff = getSqrt(scan->yOffset());
            scint_i = getSqrt(scMinInt);
            scint_a = getSqrt(scMaxInt);
        }

        if (yScaling == YSCALELOG10) {
            yoff = getLog10(scan->yOffset());
            scint_i = getLog10(scMinInt);
            scint_a = getLog10(scMaxInt);
        }

        _minIntensAbs = qMin(_minIntensAbs, scint_i + yoff);
        _maxIntensAbs = qMax(_maxIntensAbs, scint_a + yoff);
    }

    // give a slight margin above the highest and lowest point
    _maxIntensAbs *= 1.05;
    _minIntensAbs *= 1.05;
}

double GraphWindow::getVisibleMaxIntensity(const Scan *s)
{
    if (s) return s->maxIntensity(_minAngDisp, _maxAngDisp);

    double d = 0.0;

    for (int i = 0; i < scanControl->count(); ++i) {
        d = qMax(d, scanControl->at(i)->maxIntensity(_minAngDisp, _maxAngDisp) + scanControl->at(i)->yOffset());
    }

    return d;
}

double GraphWindow::getVisibleMinIntensity(const Scan *s)
{
    if (s) return s->minIntensity(_minAngDisp, _maxAngDisp);

    double d = std::numeric_limits<double>::max();

    for (int i = 0; i < scanControl->count(); ++i) {
        d = qMin(d, scanControl->at(i)->minIntensity(_minAngDisp, _maxAngDisp) + scanControl->at(i)->yOffset());
    }

    return d;
}

void GraphWindow::updateTickDensity()
{
    QFontMetrics fm(fontTicks);
    int scanBottom = qMin(getYcoord(_minIntensDisp), getYcoord(0.0));
    int scanTop    = getYcoord(_maxIntensDisp);
    int diffZero   = qMin(getYcoord(_minIntensDisp), getYcoord(yValueToScale(difOffset)));
    int diffTop    = qMax(getYcoord(0.0), getYcoord(_maxIntensDisp));
    int diffBottom = getYcoord(_minIntensDisp);

    int rangeGraphHeight  = scanBottom - scanTop;
    int rangeDiffHeight   = qMax(diffZero - diffTop, diffBottom - diffZero);

    if (rangeGraphHeight > 0) { // else the graph is off screen
        tickDensityY = int(qMin(6.0, rangeGraphHeight / (3.0 * double(fm.height()))));
    }

    if (rangeDiffHeight > 0) { // else the difference curve is off screen
        tickDensityD = int(qMin(6.0, rangeDiffHeight / (3.0 * double(fm.height()))));
    }
}

/*
 * Draw minor tick marks. Major marks are drawn in ::drawLabels()
 */
void GraphWindow::drawTicks(QPainter &p)
{
    if (!p.isActive()) return;

    // check if valid axes are defined
    if (_minIntensDisp >= _maxIntensDisp) return;
    if (_minAngDisp    >= _maxAngDisp)    return;

    updateTickDensity();

    QPen pen;
    pen.setColor(darkTheme ? global::Functions::colorToDarkMode(axisColor) : axisColor);
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
        pen.setColor(darkTheme ? global::Functions::colorToDarkMode(Qt::lightGray) : Qt::lightGray);
        pen.setStyle(Qt::DotLine);
        p.setPen(pen);

        if (showGridMinorX) drawGridXaxis(p, hPos);
        if (showGridMinorY) drawGridYaxis(p, vPos);
    }

    p.restore();
}

QList<int> GraphWindow::drawTicksXaxis(QPainter &p, int tLength)
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

QList<int> GraphWindow::drawTicksYaxis(QPainter &p, int tLength)
{
    int pw = plot.width();
    QList<Tick> ticks = getTickPositionsY(yScaling, 6 * tickDensityY);
    QList<int> pos;

    // zero line if difference curve is visible
    if (_minIntensDisp <= 0) {
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

QList<int> GraphWindow::drawTicksDiff(QPainter &p, int tLength)
{
    if (_minIntensDisp > 0) return QList<int>();

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
QList<Tick> GraphWindow::getTickPositionsX(Scale xscale, int tDensity, int tType)
{
    QList<Tick> ticks;

    if (xscale == XSCALED)  {
        // tVec contains evenly spaced d values, must be converted to 2theta for drawing
        QList<double> tVecX = global::Functions::scaleAxisD(_minAngDisp, _maxAngDisp, waveLength[0], tType, tDensity);

        for (int i = tVecX.size() - 1; i >= 0; --i) {
            int pos = getXcoord(global::Functions::dToTwoTheta(tVecX.at(i), waveLength[0]));
            Tick t(tVecX.at(i), tVecX.at(i), pos);
            ticks.append(t);
        }
    } else if (xscale == XSCALEQ) {
        // tVec contains evenly spaced Q values, must be converted to 2theta for drawing
        QList<double> tVecX = global::Functions::scaleAxisQ(_minAngDisp, _maxAngDisp, waveLength[0], tDensity);

        for (int i = tVecX.size() - 1; i >= 0; --i) {
            int pos = getXcoord(global::Functions::qToTwoTheta(tVecX.at(i), waveLength[0]));
            Tick t(tVecX.at(i), tVecX.at(i), pos);
            ticks.append(t);
        }
    } else /* XSCALETWOTHETA */ {
        // tVec contains evenly spaced 2theta values
        QList<double> tVecX = global::Functions::scaleAxis1(_minAngDisp, _maxAngDisp, tDensity);

        for (int i = 0; i < tVecX.size(); ++i) {
            int pos = getXcoord(tVecX.at(i));
            Tick t(tVecX.at(i), tVecX.at(i), pos);
            ticks.append(t);
        }
    }

    return ticks;
}

QList<Tick> GraphWindow::getTickPositionsY(Scale yscale, int tDensity)
{
    QList<Tick> ticks;

    double yMin = yScaleToValue(_minIntensDisp);
    double yMax = yScaleToValue(_maxIntensDisp);
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

QList<Tick> GraphWindow::getTickPositionsD(Scale yscale, int tDensity)
{
    QList<Tick> ticks;

    double doffScale = yValueToScale(difOffset);
    double difZeroPos = getYcoord(doffScale);
    double difScreenOffset = difZeroPos - getYcoord(yValueToScale(0.0));

    // Visible range above and below the difference zero line. If the range is visible, the value is positive.
    // If the range is off screen, the value is negative.
    double posLim = qMin(-difOffset, yScaleToValue(_maxIntensDisp - doffScale));
    double negLim = -yScaleToValue(_minIntensDisp - doffScale);

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

void GraphWindow::drawGridXaxis(QPainter &p, const QList<int> &hPos)
{
    int ph = plot.height();

    for (int i = 0; i < hPos.size(); ++i) {
        p.drawLine(hPos.at(i), 0.0, hPos.at(i), ph);
    }
}

void GraphWindow::drawGridYaxis(QPainter &p, const QList<int> &vPos)
{
    int pw = plot.width();

    for (int i = 0; i < vPos.size(); ++i) {
        p.drawLine(0.0, vPos.at(i), pw, vPos.at(i));
    }
}

/*
 * draw major tickmarks and lables
 */
void GraphWindow::drawLabels(QPainter &p)
{
    if (!p.isActive()) return;

    // check if valid axes are defined
    if (_minIntensDisp >= _maxIntensDisp) return;
    if (_minAngDisp >= _maxAngDisp) return;

    QPen pen;
    pen.setColor(darkTheme ? global::Functions::colorToDarkMode(axisColor) : axisColor);
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
        pen.setColor(darkTheme ? global::Functions::colorToDarkMode(Qt::gray) : Qt::gray);
        pen.setStyle(Qt::DashLine);
        p.setPen(pen);

        if (showGridMajorX) drawGridXaxis(p, hPos);
        if (showGridMajorY) drawGridYaxis(p, vPos);
    }

    p.restore();
}

QList<int> GraphWindow::drawLabelsXaxis(QPainter &p, const QFontMetrics &fm, int tLength, int lSpacing)
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

QList<int> GraphWindow::drawLabelsYaxis(QPainter &p, const QFontMetrics &fm, int tLength, int lSpacing)
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

QList<int> GraphWindow::drawLabelsDiff(QPainter &p, const QFontMetrics &fm, int tLength, int lSpacing)
{
    if (_minIntensDisp > 0) return QList<int>();

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

/*
 * toggles drawing of the legend
 */
void GraphWindow::slotSetDrawLegend(bool a)
{
    settings->setValue("graph/drawLegend", a);
    legend->initSettings();
    forceUpdate();
}

/*
 * moves visible section
 * used by tool buttons or key press events
 */
void GraphWindow::moveLeft(double fraction)
{
    if (!zoomed) return;

    double d = _maxAngDisp - _minAngDisp;
    _maxAngDisp += d * fraction;

    if (_maxAngDisp > _maxAngAbs) _maxAngDisp = _maxAngAbs;

    _minAngDisp = _maxAngDisp - d;

    forceUpdate();
}

/*
 * moves visible section
 * used by tool buttons or key press events
 */
void GraphWindow::moveRight(double fraction)
{
    if (!zoomed) return;

    double d = _maxAngDisp - _minAngDisp;
    _minAngDisp -= d * fraction;

    if (_minAngDisp < _minAngAbs) _minAngDisp = _minAngAbs;

    _maxAngDisp = _minAngDisp + d;

    forceUpdate();
}

/*
 * moves visible section
 * used by tool buttons or key press events
 */
void GraphWindow::moveUp(double fraction)
{
    if (!zoomed) return;

    double d = _maxIntensDisp - _minIntensDisp;
    _minIntensDisp -= d * fraction;

    if (_minIntensDisp < _minIntensAbs) _minIntensDisp = _minIntensAbs;

    _maxIntensDisp = _minIntensDisp + d;

    forceUpdate();
}

/*
 * moves visible section
 * used by tool buttons or key press events
 */
void GraphWindow::moveDown(double fraction)
{
    if (!zoomed) return;

    double d = _maxIntensDisp - _minIntensDisp;
    _maxIntensDisp += d * fraction;

    if (_maxIntensDisp > _maxIntensAbs) _maxIntensDisp = _maxIntensAbs;

    _minIntensDisp = _maxIntensDisp - d;

    forceUpdate();
}

/*
 * moves visible section
 * used by tool buttons or key press events
 */
void GraphWindow::moveToStart()
{
    if (!zoomed) return;

    double d = _maxAngDisp - _minAngDisp;
    _minAngDisp = _minAngAbs;
    _maxAngDisp = _minAngAbs + d;
    forceUpdate();
}

/*
 * moves visible section
 * used by tool buttons or key press events
 */
void GraphWindow::moveToEnd()
{
    if (!zoomed) return;

    double d = _maxAngDisp - _minAngDisp;
    _maxAngDisp = _maxAngAbs;
    _minAngDisp = _maxAngAbs - d;
    forceUpdate();
}

/*
 * zooms on the center of the graph
 * fraction < 1.0: zoom in
 * fraction > 1.0: zoom out
 * used by tool buttons or key press events
 */
void GraphWindow::zoomStepAngle(double fraction)
{
    if (!zoomed && (fraction > 1.0)) return;

    double c = (_minAngDisp + _maxAngDisp) / 2.0; // center position
    double d = (_maxAngDisp - _minAngDisp) / 2.0; // half width of displayed range

    _minAngDisp = c - d * fraction;
    _maxAngDisp = c + d * fraction;

    if (_minAngDisp < _minAngAbs) _minAngDisp = _minAngAbs;
    if (_maxAngDisp > _maxAngAbs) _maxAngDisp = _maxAngAbs;

    zoomed = !(qFuzzyCompare(_minAngDisp, _minAngAbs) && qFuzzyCompare(_maxAngDisp, _maxAngAbs));
    forceUpdate();
}

/*
 * zooms on the center of the graph
 * fraction < 1.0: zoom in
 * fraction > 1.0: zoom out
 * used by tool buttons or key press events
 */
void GraphWindow::zoomStepIntensity(double fraction)
{
    if (!zoomed && (fraction > 1.0)) return;

    double c = (_minIntensDisp + _maxIntensDisp) / 2.0; // center position
    double d = (_maxIntensDisp - _minIntensDisp) / 2.0; // half width of displayed range

    _minIntensDisp = c - d * fraction;
    _maxIntensDisp = c + d * fraction;

    if (_minIntensDisp < _minIntensAbs) _minIntensDisp = _minIntensAbs;
    if (_maxIntensDisp > _maxIntensAbs) _maxIntensDisp = _maxIntensAbs;

    zoomed = !(qFuzzyCompare(_minIntensDisp, _minIntensAbs) && qFuzzyCompare(_maxIntensDisp, _maxIntensAbs));
    forceUpdate();
}

/*
 * returns the minimum and maximum angle/intensity of the graph
 */
void GraphWindow::getMaxRange(double &a_min, double &a_max, double &i_min, double &i_max)
{
    a_min = _minAngAbs;
    a_max = _maxAngAbs;
    i_min = _minIntensAbs;
    i_max = _maxIntensAbs;
}

/*
 * writes the currently active zoom range in angle/intensity units
 */
void GraphWindow::getZoomRange(double &a_min, double &a_max, double &i_min, double &i_max)
{
    a_min = _minAngDisp;
    a_max = _maxAngDisp;
    i_min = yScaleToValue(_minIntensDisp);
    i_max = yScaleToValue(_maxIntensDisp);
}

/*
 * sets the zoom range in angle/intensity units
 */
void GraphWindow::setZoomRange(double a_min, double a_max, double i_min, double i_max, bool checkLimits)
{
    double curImin = yValueToScale(i_min);
    double curImax = yValueToScale(i_max);

    if (checkLimits) {
        _minAngDisp    = qMax(_minAngAbs,    qMin(a_min, a_max));
        _maxAngDisp    = qMin(_maxAngAbs,    qMax(a_min, a_max));
        _minIntensDisp = qMax(_minIntensAbs, qMin(curImin, curImax));
        _maxIntensDisp = qMin(_maxIntensAbs, qMax(curImin, curImax));
    } else {
        _minAngDisp    = qMin(a_min, a_max);
        _maxAngDisp    = qMax(a_min, a_max);
        _minIntensDisp = qMin(curImin, curImax);
        _maxIntensDisp = qMax(curImin, curImax);
    }

    //check if we zoomed all the way out
    if (qFuzzyCompare(_minAngDisp, _minAngAbs)
        && qFuzzyCompare(_maxAngDisp, _maxAngAbs)
        && qFuzzyCompare(_minIntensDisp, _minIntensAbs)
        && qFuzzyCompare(_maxIntensDisp, _maxIntensAbs)) {
        zoomed = false;
    } else {
        zoomed = true;
    }

    forceUpdate();
}

/*
 * forces a complete redraw of the plot
 * use this instead of update() if zoom ranges,
 * number of scans, visibilities etc. changed.
 *
 * update() only uses the buffered plot to redraw
 * the widget, e.g. after drawing the crosshair
 */
void GraphWindow::forceUpdate()
{
    recalcPlot = true;
    legend->updateView();
    update();
}

bool GraphWindow::setStatus(const global::RefinementStatus &i)
{
    switch (i) {
    case global::RefinementStatus::IDLE:
        marginColor = idleColor;
        break;
    case global::RefinementStatus::RUNNING:
        marginColor = activeColor;
        break;
    case global::RefinementStatus::MATCHING:
        marginColor = activeColor;
        break;
    case global::RefinementStatus::COMPLETED:
        marginColor = completedColor;
        break;
    case global::RefinementStatus::SCHEDULED:
        marginColor = idleColor;
        break;
    case global::RefinementStatus::ABORTED:
        marginColor = abortedColor;
        break;
    case global::RefinementStatus::FAILURE:
        marginColor = abortedColor;
        break;
    case global::RefinementStatus::CRASH:
        marginColor = abortedColor;
        break;
    case global::RefinementStatus::FITSCHEDULED:
        break;
    case global::RefinementStatus::FITRUNNING:
        break;
    default:
        break;
    }

    return true;
}

/*
 * changes the state of the graph to active
 */
void GraphWindow::setActive()
{
    marginColor = activeColor;
    recalcPlot = true;
    update();
}

/*
 * changes the state of the graph to idle
 */
void GraphWindow::setIdle()
{
    marginColor = idleColor;
    recalcPlot = true;
    update();
}

/*
 * changes the state of the graph to complete
 */
void GraphWindow::setComplete()
{
    marginColor = completedColor;
    recalcPlot = true;
    update();
}

void GraphWindow::setPeakPreviewMode(const PeakPreviewMode &p, const QUuid &u)
{
    // do not allow other callers than rangeCallerUid to disable active peak preview modes
    if (peakPreviewMode != PPMNONE && u != rangeCallerUid) return;

    peakPreviewMode = p;

    if (p != PPMNONE) {
        marginColor = peakSelectColor;
        setCursor(Qt::CrossCursor);
        rangeCallerUid = u;
    } else {
        marginColor = idleColor;
        setCursor(Qt::ArrowCursor);
        emit sigCursorMessage(QString(), rangeCallerUid);
        rangeCallerUid = QUuid();
    }

    recalcPlot = true;
    update();
}

void GraphWindow::setRangeSelectMode(bool b, const QUuid &u)
{
    // do not allow other callers than rangeCallerUid to disable the selection mode
    if (rangeSelectMode && !b && u != rangeCallerUid) return;

    rangeSelectMode = b;

    if (b) {
        marginColor = peakSelectColor;
        setCursor(Qt::CrossCursor);
        rangeCallerUid = u;
    } else {
        marginColor = idleColor;
        setCursor(Qt::ArrowCursor);
        emit sigCursorMessage(QString(), rangeCallerUid);
        rangeCallerUid = QUuid();
    }

    recalcPlot = true;
    update();
}

void GraphWindow::setReferenceReflections(const Scan &sc)
{
    tt.clear();
    refStructure = sc;
    refStructure.setScaleFactor(prevStrucScaleFactor);

    const Scan *scanActive = scanControl->firstActiveScan();
    const Scan *scanBkgr   = scanControl->backgroundScan();

    double _refYHklMax = refStructure.hklMaxIntensity();
    double _yHeight = scanMaxIntensity;
    double _yTip = scanMaxIntensity;
    double _yBase = 0.0;

    if (scanActive) {
        if (scanActive->hasScanData()) {
            int n = scanActive->indexOfMaxIntensity();
            _yTip = scanActive->at(n);

            if (scanBkgr) {
                _yBase = scanBkgr->at(n);
            }
        }
    }

    if (settings->value("graph/hklOnBackground", 1).toInt() > 0) {
        _yHeight = _yTip - _yBase;
    }

    // normalize the hkl intensities
    for (int i = 0; i < refStructure.pDataHkl().size(); ++i) {
        refStructure.pDataHkl()[i].setIntensity(refStructure.pDataHkl().at(i).intensity() * _yHeight / _refYHklMax);
    }

    // exit here if only hkl lines are available in refStructure
    if (!settings->value("graph/showRefStructureScan", false).toBool()) {
        forceUpdate();
        return;
    }

    if (!refStructure.hasScanData()) {
        forceUpdate();
        return;
    }

    // normalize the xy reference pattern
    double _refYXyMax  = refStructure.maxIntensity();

    if (_refYXyMax < 0.0) return;

    for (int i = 0; i < refStructure.pDataIntensity().size(); ++i) {
        refStructure.pDataIntensity()[i] = _yHeight * refStructure.pDataIntensity().at(i) / _refYXyMax;
        if (scanBkgr) {
            refStructure.pDataIntensity()[i] += scanBkgr->intensity(refStructure.pDataAngle().at(i));
        }
    }

    forceUpdate();
}

void GraphWindow::normalizeHkl(Scan *scan)
{
    double _ymaxHkl = 0.0;
    for (int i = 0; i < scan->pDataHkl().size(); ++i) {
        _ymaxHkl = qMax(_ymaxHkl, scan->pDataHkl().at(i).intensity());
    }

    double _ymaxScan = qFuzzyIsNull(scanMaxIntensity) ? 100.0 : scanMaxIntensity;

    for (int i = 0; i < scan->pDataHkl().size(); ++i) {
        double _d = scan->pDataHkl().at(i).intensity();
        scan->pDataHkl()[i].setIntensity(_ymaxScan * _d / _ymaxHkl);
    }
}

/*
 * call this function after changing the content of scanHeap
 * (e.g. after loading, adding, reloading scans).
 * It applies the correct styles, wavelength, loads hkl reflections etc.
 */
void GraphWindow::setupNewScans()
{
    if (!scanControl->hasData()) return;

    double w = scanControl->getWaveLength(settings->defaultWavelength());

    if (!hasOverrideWaveLength) {
        if ((wavelengthMode == Scan::WavelengthMode::UNKNOWN) || (wavelengthMode == Scan::WavelengthMode::CHARACTERISTIC)) {
            scanControl->getNearestCharacteristicWaveLength(waveLength[0], waveLength[1], waveLength[2], w);
        } else {
            // wavelengthMode = Scan::WavelengthMode::SYNCHROTRON or ::NEUTRON
            waveLength[0] = w;
            waveLength[1] = -1.0;
            waveLength[2] = -1.0;
        }
    } // else don't change the override wavelength

    changeGraphStyles();
    legend->updateView();

    if (zoomed) calcLimits();
    else        resetZoom();
}

/*
 * calculates the y coordinate at position x by interpolating between a and b
 */
QPoint GraphWindow::interpolate(const QPoint &a, const QPoint &b, int x)
{
    if (a.x() == b.x()) return a;

    int dy = int(0.5 + a.y() + (b.y() - a.y()) * (double(x) - a.x()) / (b.x() - a.x()));

    return QPoint(x, dy);
}

void GraphWindow::changeGraphStyles()
{
    scanControl->updateScanColors(false);
    scanControl->updateScanStyles(false);
}

void GraphWindow::print(QPrinter &printer, QPainter &painter)
{
    // scale factor used to scale the output for printing (will be 1.0 for screen painting)
    // we pretend the widget has the size of the desktop, to make the output independent
    // of the widget size. Else by resizing the profex window the printed font size would change.
    double pFontSize = settings->value("graph/printingFontSize", 1.25).toDouble();
    pScale = pFontSize * qMin(double(printer.pageRect(QPrinter::DevicePixel).width()) / 1920.0,
                        double(printer.pageRect(QPrinter::DevicePixel).height()) / 1080.0);

    QFontInfo fiTitle(oFontTitle);
    QFontInfo fiAxis(oFontAxis);
    QFontInfo fiTicks(oFontTicks);
    QFontInfo fiLegend(oFontLegend);
    bool oDarkTheme = darkTheme;
    darkTheme = false;

    fontTitle.setPixelSize(int(pScale * double(fiTitle.pixelSize())));
    fontAxis.setPixelSize(int(pScale * double(fiAxis.pixelSize())));
    fontTicks.setPixelSize(int(pScale * double(fiTicks.pixelSize())));
    fontLegend.setPixelSize(int(pScale * double(fiLegend.pixelSize())));

    int w = printer.pageRect(QPrinter::DevicePixel).width();
    int h = printer.pageRect(QPrinter::DevicePixel).height();

    // temporarily switch the line width to the printing line width
    int screenLineWidth = lineWidth;
    lineWidth = printingLineWidth;

    calcMargins(painter, false);

    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, antiAliasing);
    painter.fillRect(0, 0, w, h, idleColor);
    initPlot(printer.pageRect(QPrinter::DevicePixel).toRect());
    drawWindow(painter);
    drawHighlightedRegions(painter);
    drawPlot(painter, false);
    drawHklTicks(painter);
    drawHklScans(painter);
    drawTicks(painter);
    drawLabels(painter);
    drawLegend(painter);

    lineWidth = screenLineWidth;
    pScale = 1.0;

    fontTitle = oFontTitle;
    fontAxis = oFontAxis;
    fontTicks = oFontTicks;
    fontLegend = oFontLegend;
    darkTheme = oDarkTheme;

    resize(width(), height());
    update();
}

/*
 * renders the graph to a bitmap
 */
void GraphWindow::renderBitmap(const QString &s, int w, int h)
{
    QPixmap pix = renderBitmap(w, h, settings->value("graph/rasterBackgroundFilled", false).toBool());

    if (pix.isNull()) {
        return;
    }

    pix.save(s, "PNG");
}

QPixmap GraphWindow::renderBitmap(int w, int h, bool fillbg)
{
    if ((w <= 0) || (h <= 0)) {
        return QPixmap();
    }

    int screenLineWidth = lineWidth;
    QPixmap pix(w, h);

    if (fillbg) {
        pix.fill(bgColor);
    } else {
        pix.fill(Qt::transparent);
    }

    // leave the reference hard-coded to obtain resolution-independent font sizes
    pScale = qMin(double(pix.width()) / 1152.0, double(pix.height()) / 768.0);

    QFontInfo fiTitle(oFontTitle);
    QFontInfo fiAxis(oFontAxis);
    QFontInfo fiTicks(oFontTicks);
    QFontInfo fiLegend(oFontLegend);

    fontTitle.setPixelSize(int(pScale * fiTitle.pixelSize()));
    fontAxis.setPixelSize(int(pScale * fiAxis.pixelSize()));
    fontTicks.setPixelSize(int(pScale * fiTicks.pixelSize()));
    fontLegend.setPixelSize(int(pScale * fiLegend.pixelSize()));
    bool oDarkTheme = darkTheme;
    QColor oBgColor = bgColor;
    darkTheme = false;
    bgColor = Qt::white;

    QPainter painter;
    painter.begin(&pix);
        calcMargins(painter, false);

        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, true);

        initPlot(pix.rect());
        drawWindow(painter);
        drawHighlightedRegions(painter);
        drawPlot(painter, false);
        drawAnchorPoints(painter);
        drawHklTicks(painter);
        drawHklScans(painter);
        drawHklReferenceLines(painter);
        drawTicks(painter);
        drawLabels(painter);
        drawLegend(painter);
    painter.end();

    lineWidth = screenLineWidth;
    pScale = 1.0;

    fontTitle = oFontTitle;
    fontAxis = oFontAxis;
    fontTicks = oFontTicks;
    fontLegend = oFontLegend;
    darkTheme = oDarkTheme;
    bgColor = oBgColor;

    return pix;
}

void GraphWindow::renderSvg(QBuffer *buf, double a)
{
    QSvgGenerator svg;
    svg.setOutputDevice(buf);
    svg.setTitle(scanControl->getSampleId());

    double scaleFac = settings->value("graph/svgScaleFactor", 1.0).toDouble();

    // using an aspect ratio of sqrt(2) : 1
    double w = scaleFac * 27.0 * double(svg.resolution()) / 2.54;
    double h = w / (qFuzzyIsNull(a) ? sqrt(float(2.0)) : a);
    QRectF pageRect(0.0, 0.0, w, h);

    svg.setSize(QSize(int(w), int(h)));
    svg.setViewBox(pageRect);

    pScale = settings->value("graph/svgExportFontSize", 1.0).toDouble(); // smaller values reduce font size

    QFontInfo fiTitle(oFontTitle);
    QFontInfo fiAxis(oFontAxis);
    QFontInfo fiTicks(oFontTicks);
    QFontInfo fiLegend(oFontLegend);
    bool oDarkTheme = darkTheme;
    QColor oBgColor = bgColor;
    darkTheme = false;
    bgColor = Qt::white;

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
        drawHighlightedRegions(painter);
        drawPlot(painter, true);
        drawAnchorPoints(painter);
        drawHklTicks(painter);
        drawHklScans(painter);
        drawHklReferenceLines(painter);
        drawTicks(painter);
        drawLabels(painter);
        drawLegend(painter);
    painter.end();

    pScale = 1.0;

    fontTitle = oFontTitle;
    fontAxis = oFontAxis;
    fontTicks = oFontTicks;
    fontLegend = oFontLegend;
    darkTheme = oDarkTheme;
    bgColor = oBgColor;
}

/*
 * renders the graph to a SVG file
 */
void GraphWindow::saveSvg(const QString &s)
{
    QBuffer buf;
    buf.open(QBuffer::ReadWrite);
    renderSvg(&buf);

    QFile f(s);

    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))     {
        QTextStream str(&f);
        buf.seek(0);
        str << buf.readAll();
        f.close();
    }

    buf.close();
}

/*
 * returns an SVG file as a string
 */
QByteArray GraphWindow::getSvg(double a)
{
    QBuffer buf;
    buf.open(QBuffer::ReadWrite);
    renderSvg(&buf, a);

    buf.seek(0);
    QByteArray data = buf.readAll();

    buf.close();
    return data;
}

void GraphWindow::setXaxis2theta(bool upd)
{
    setXaxisScale(XSCALETWOTHETA, upd);
}

void GraphWindow::setXaxisD(bool upd)
{
    setXaxisScale(XSCALED, upd);
}

void GraphWindow::setXaxisQ(bool upd)
{
    setXaxisScale(XSCALEQ, upd);
}

void GraphWindow::setXaxisScale(Scale s, bool upd)
{
    if (xScaling == s) return;

    double amin, amax, imin, imax;
    getZoomRange(amin, amax, imin, imax);

    xScaling = s;
    calcLimits();

    if (zoomed) {
        setZoomRange(amin, amax, imin, imax, true);
    } else {
        resetZoom();
    }

    if (upd) forceUpdate();
}

void GraphWindow::setYaxisLinear(bool upd)
{
    setYaxisScale(YSCALELIN, upd);
}

void GraphWindow::setYaxisSqrt(bool upd)
{
    setYaxisScale(YSCALESQRT, upd);
}

void GraphWindow::setYaxisLog10(bool upd)
{
    setYaxisScale(YSCALELOG10, upd);
}

void GraphWindow::setYaxisScale(Scale s, bool upd)
{
    double amin, amax, imin, imax;
    getZoomRange(amin, amax, imin, imax);

    yScaling = s;
    calcLimits();

    if (zoomed) {
        setZoomRange(amin, amax, imin, imax, true);
    } else {
        resetZoom();
    }

    if (upd) forceUpdate();
}

void GraphWindow::zoomFromZeroY(bool upd)
{
    double amin, amax, imin, imax;
    getZoomRange(amin, amax, imin, imax);

    calcLimits();
    setZoomRange(amin, amax, 0.0, imax, true);

    if (upd) forceUpdate();
}

void GraphWindow::zoomFromMinY(bool upd)
{
    if (!scanControl->count()) return;

    double amin, amax, imin, imax;
    getZoomRange(amin, amax, imin, imax);

    double currentScanMinInt = scanControl->first()->minIntensity(amin, amax);
    imin = currentScanMinInt < 0.0 ? std::numeric_limits<double>::max() : currentScanMinInt;

    for (int i = 1; i < scanControl->count(); ++i) {
        currentScanMinInt = scanControl->at(i)->minIntensity(amin, amax);
        if (currentScanMinInt >= 0.0) imin = qMin(currentScanMinInt, imin);
    }

    calcLimits();
    setZoomRange(amin, amax, imin, imax, true);

    if (upd) forceUpdate();
}

void GraphWindow::copyPixmapToClipboard()
{
    QColor oldMarginColor = marginColor;
    QColor oldBgColor = bgColor;
    bool oldDarkTheme = darkTheme;
    darkTheme = false;

    marginColor = Qt::white;
    bgColor = Qt::white;
    forceUpdate();

    int w = settings->value("graph/rasterResolutionWidth", 1536).toInt();
    int h = settings->value("graph/rasterResolutionHeight", 1024).toInt();

    QGuiApplication::clipboard()->setImage(renderBitmap(w, h, true).toImage());

    qDebug() << QString("GraphWindow::copyPixmapToClipboard(): Pixmap of size %1 x %2 copied to clipboard").arg(w).arg(h);

    // flicker the background color to grey to give visual feedback
    marginColor = Qt::lightGray;
    bgColor = Qt::lightGray;

    forceUpdate();
    qApp->processEvents();

    marginColor = oldMarginColor;
    bgColor = oldBgColor;
    darkTheme = oldDarkTheme;

    QTimer::singleShot(200, this, SLOT(forceUpdate()));
}

void GraphWindow::overrideWaveLength(double ka1, Scan::WavelengthMode wm)
{
    hasOverrideWaveLength = true;

    // ka1 in Angstrom
    if ((wm == Scan::WavelengthMode::UNKNOWN) || (wm == Scan::WavelengthMode::CHARACTERISTIC)) {
        scanControl->getNearestCharacteristicWaveLength(waveLength[0], waveLength[1], waveLength[2], ka1);
    } else {
        waveLength[0] = ka1;
        waveLength[1] = ka1;
        waveLength[2] = ka1;
    }

    forceUpdate();
}

void GraphWindow::setOverrideHklBaseLine(int n)
{
    hklReferenceBase = n;
    forceUpdate();
}

void GraphWindow::togglePhaseVisibility()
{
    scanControl->togglePhaseVisibility();
    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, nullptr);
}

void GraphWindow::changeCursor()
{
    if (noiseCursor || specLines || crossHair || inspector) {
        setCursor(Qt::CrossCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void GraphWindow::setWavelengthMode(const Scan::WavelengthMode &w)
{
    wavelengthMode = w;
}

QPainter::CompositionMode GraphWindow::compositionModeLines() const
{
    return darkTheme ? QPainter::CompositionMode_Plus : QPainter::CompositionMode_Multiply;
}

QPainter::CompositionMode GraphWindow::compositionModeFill() const
{
    return darkTheme ? QPainter::CompositionMode_Plus : QPainter::CompositionMode_Multiply;
}

/*
 * converts a true y value to a value on the y scale
 * e.g. y -> sqrt(y)
 */
double GraphWindow::yValueToScale(double d) const
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

/*
 * converts a value on the y scale to the true value
 * e.g. sqrt(y) -> y
 */
double GraphWindow::yScaleToValue(double d) const
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

/** EOF **/



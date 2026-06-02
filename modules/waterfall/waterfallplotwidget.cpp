/***************************************************************************
                          waterfallplotwidget.cpp  -  description
                             -------------------
    begin                : Tue Aug 03 12:57:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#include "waterfallplotwidget.h"
#include "../../libXrdIO/scanops.h"
#include "../../libXrdIO/structs.h"
#include "../../libXrdIO/functions.h"
#include "../../libXrdIO/bgmnfileio.h"
#include <limits>
#include <QDebug>
#include <QApplication>
#include <QFileInfo>
#include <QBuffer>
#include <QSvgGenerator>

WaterfallPlotWidget::WaterfallPlotWidget(QWidget *parent) : QFrame(parent)
{
    _scanHeap = nullptr;
    _uidList = nullptr;
    _blockUpdate = false;
    _yScale = 1;
    _lutOffset = 0;
    _lutSign = 1;
    _gamma = 1.0;
    _nPoints = 0;
    _wmin = _wminDisp = _wminDispClicked = 5.0;
    _wmax = _wmaxDisp = _wmaxDispClicked = 60.0;
    _imin = _iminDisp = 0.0;
    _imax = _imaxDisp = 1.0;
    _iRangeMinLin = _iRangeMinDisp = 0.0;
    _iRangeMaxLin = _iRangeMaxDisp = 1.0;
    _smoothInterpol = 0;
    _drawGridH = false;
    _showScanLabels = true;
    _lineBuffer = QImage();
    _scaledBuffer = QImage();
    setMouseTracking(true);
}

WaterfallPlotWidget::~WaterfallPlotWidget()
{
}

/************************* Public functions ******************************/

void WaterfallPlotWidget::updateScanData()
{
    if (!_scanHeap) return;
    if (!_uidList)  return;

    _labels.clear();

    for (int i = 0; i < _uidList->size(); ++i) {
        if (!_scanHeap->contains(_uidList->at(i))) continue;

        bool hasLbl;
        QString l = _scanHeap->value(_uidList->at(i)).auxInfo("waterfallPlotLabel", hasLbl).toString();

        if (hasLbl) {
            _labels.append(l);
        } else {
            QFileInfo fi(_scanHeap->value(_uidList->at(i)).sourceFileName());
            _labels.append(fi.fileName());
        }
    }

    if (!_blockUpdate) updateAll();
}

void WaterfallPlotWidget::clearScans()
{
    _labels.clear();
    _lineBuffer = QImage();
    _scaledBuffer = QImage();
    _nPoints = 0;

    if (!_blockUpdate) updateAll();
}

void WaterfallPlotWidget::updateAll()
{
    if (!_scanHeap) return;

    if (_scanHeap->size()) {
        qApp->setOverrideCursor(Qt::WaitCursor);
            calcDrawRegions(this, fontMetrics());
            calcLimits();
            calcXtickMarks();
            calcDisplayYlimits();
            renderImageBuffer();
            scaleDisplayBuffer();
        qApp->restoreOverrideCursor();
    }

    update();
}

void WaterfallPlotWidget::setSmoothInterpolation(bool b)
{
    _smoothInterpol = b ? 2 : 0;

    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        qApp->setOverrideCursor(Qt::WaitCursor);
            scaleDisplayBuffer();
            update();
        qApp->restoreOverrideCursor();
    }
}

void WaterfallPlotWidget::setDrawGridHorizontal(bool b)
{
    _drawGridH = b;
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        update();
    }
}

void WaterfallPlotWidget::setDrawGridVertical(bool b)
{
    _drawGridV = b;
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        update();
    }
}

void WaterfallPlotWidget::setShowScanLabels(bool b)
{
    _showScanLabels = b;
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        calcDrawRegions(this, fontMetrics());
        scaleDisplayBuffer();
        update();
    }
}

void WaterfallPlotWidget::setYscale(int i)
{
    _yScale = i;
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        qApp->setOverrideCursor(Qt::WaitCursor);
            calcDisplayYlimits();
            renderImageBuffer();
            scaleDisplayBuffer();
            update();
        qApp->restoreOverrideCursor();
    }
}

void WaterfallPlotWidget::setGamma(double g)
{
    _gamma = g < 0.0 ? 0.0 : g;
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        qApp->setOverrideCursor(Qt::WaitCursor);
            calcDisplayYlimits();
            renderImageBuffer();
            scaleDisplayBuffer();
            update();
        qApp->restoreOverrideCursor();
    }
}

void WaterfallPlotWidget::setXrange(double min, double max)
{
    _wminDisp = qMax(min, _wmin);
    _wmaxDisp = qMin(max, _wmax);
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        qApp->setOverrideCursor(Qt::WaitCursor);
            calcXtickMarks();
            scaleDisplayBuffer();
            update();
        qApp->restoreOverrideCursor();
    }

}

void WaterfallPlotWidget::setYrange(double min, double max)
{
    _iRangeMinLin = qMax(min, _imin < 0.0 ? _imin : 0.0);
    _iRangeMaxLin = qMin(max, _imax);
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        qApp->setOverrideCursor(Qt::WaitCursor);
            calcDisplayYlimits();
            renderImageBuffer();
            scaleDisplayBuffer();
            update();
        qApp->restoreOverrideCursor();
    }
}

void WaterfallPlotWidget::getXrange(double &min, double &max)
{
    min = _wminDisp;
    max = _wmaxDisp;
}

void WaterfallPlotWidget::getYrange(double &min, double &max)
{
    min = _iRangeMinLin;
    max = _iRangeMaxLin;
}

void WaterfallPlotWidget::getXlimits(double &min, double &max)
{
    min = _wmin;
    max = _wmax;
}

void WaterfallPlotWidget::getYlimits(double &min, double &max)
{
    min = _imin;
    max = _imax;
}

void WaterfallPlotWidget::resetXrange()
{
    _wminDisp = _wmin;
    _wmaxDisp = _wmax;
    emit sigXrangeWasZoomed();

    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        qApp->setOverrideCursor(Qt::WaitCursor);
            calcXtickMarks();
            scaleDisplayBuffer();
            update();
        qApp->restoreOverrideCursor();
    }
}

void WaterfallPlotWidget::resetYrange()
{
    _iRangeMinLin = _imin < 0.0 ? _imin : 0.0;
    _iRangeMaxLin = _imax;
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        qApp->setOverrideCursor(Qt::WaitCursor);
            calcDisplayYlimits();
            renderImageBuffer();
            scaleDisplayBuffer();
            update();
        qApp->restoreOverrideCursor();
    }
}

void WaterfallPlotWidget::setLut(const colorMaps::Lut &l)
{
    _lut = l;
    setLutOffset();
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        qApp->setOverrideCursor(Qt::WaitCursor);
            renderImageBuffer();
            scaleDisplayBuffer();
            update();
        qApp->restoreOverrideCursor();
    }
}

void WaterfallPlotWidget::setColorTemp(int i)
{
    _colTemp = i;
    setLutOffset();
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        qApp->setOverrideCursor(Qt::WaitCursor);
            renderImageBuffer();
            scaleDisplayBuffer();
            update();
        qApp->restoreOverrideCursor();
    }
}

void WaterfallPlotWidget::highlightScans(const QList<QUuid> &l)
{
    _hlIdx = l;
    if (!_scanHeap) return;

    if (_scanHeap->size() && !_blockUpdate) {
        update();
    }
}

/************************* Private functions ******************************/

bool WaterfallPlotWidget::hasData()
{
    if (!_scanHeap) return false;
    if (!_uidList)  return false;
    if (!_uidList->size()) return false;
    if (!_scanHeap->size()) return false;

    return true;
}

void WaterfallPlotWidget::setLutOffset()
{
    _lutOffset = 0;
    _lutSign   = 1;
    _lutBins = _lut.hasNegativeRange ? _lut.data.size() / 2 : _lut.data.size();

    if (_lut.hasNegativeRange) {
        _lutOffset = _lutBins - 1;
        if (_colTemp > 0) _lutSign = -1;
    } else if (_colTemp > 0) {
        _lutOffset = _lutBins - 1;
        _lutSign = -1;
    }
}

void WaterfallPlotWidget::calcLimits()
{
    if (!hasData()) {
        _wmin = 5.0;
        _wmax = 60.0;
        _wminDisp = 0.0;
        _wmaxDisp = 1.0;
        _imin = 0.0;
        _imax = 1.0;
        _nPoints = 0;
        return;
    }

    _wmin = std::numeric_limits<double>::max();
    _wmax = 0.0;
    _imin = std::numeric_limits<double>::max();
    _imax = 0.0;
    _nPoints = 0;
    double stepsize = std::numeric_limits<double>::max();

    for (int i = 0; i < _uidList->size(); ++i) {
        if (!_scanHeap->contains(_uidList->at(i))) {
            continue;
        }

        Scan sc = _scanHeap->value(_uidList->at(i));

        if (!_metricsBuffer.contains(_uidList->at(i))) {
            double twmin, twmax, timin, timax;
            ScanOps::scanMetrics(sc, twmin, twmax, timin, timax);
            _metricsBuffer.insert(_uidList->at(i), WpScanMetrics(twmin, twmax, timin, timax, sc.size()));
        }

        WpScanMetrics sm = _metricsBuffer.value(_uidList->at(i));
        _wmin = qMin(_wmin, sm.wmin);
        _wmax = qMax(_wmax, sm.wmax);
        _imin = qMin(_imin, sm.imin);
        _imax = qMax(_imax, sm.imax);

        if (sm.npoints > 0) {
            stepsize = qMin(stepsize, (sm.wmax - sm.wmin)/double(sm.npoints));
        }
    }

    _nPoints = qFuzzyIsNull(stepsize) ? 0 : int(0.5 + (_wmax - _wmin) / stepsize);
    _wminDisp = _wmin;
    _wmaxDisp = _wmax;

    _iRangeMinLin = _imin < 0.0 ? _imin : 0.0;
    _iRangeMaxLin = _imax;
}

void WaterfallPlotWidget::calcXtickMarks()
{
    _majTicks.clear();
    _minTicks.clear();

    if (!_scanHeap) return;

    if (_scanHeap->size()) {
        _majTicks = global::Functions::scaleAxis1(_wminDisp, _wmaxDisp, 10);
        _minTicks = global::Functions::scaleAxis1(_wminDisp, _wmaxDisp, 60);
    }
}

void WaterfallPlotWidget::calcDisplayYlimits()
{
    _iminDisp = qFuzzyCompare(_gamma, 1.0) ? _imin : pow(_imin, _gamma);
    _imaxDisp = qFuzzyCompare(_gamma, 1.0) ? _imax : pow(_imax, _gamma);
    _iRangeMinDisp = qFuzzyCompare(_gamma, 1.0) ? _iRangeMinLin : pow(_iRangeMinLin, _gamma);
    _iRangeMaxDisp = qFuzzyCompare(_gamma, 1.0) ? _iRangeMaxLin : pow(_iRangeMaxLin, _gamma);
}

void WaterfallPlotWidget::renderImageBuffer()
{
    if (!hasData() || qFuzzyCompare(_wmin, _wmax)) {
        _nPoints = 0;
        return;
    }

    _lineBuffer = QImage(_nPoints, _uidList->size(), _lut.bits == 8 ? QImage::Format_Grayscale8 : QImage::Format_ARGB32);
    _lineBuffer.fill(Qt::black);

    for (int y = 0; y < _uidList->size(); ++y) {
        if (!_scanHeap->contains(_uidList->at(y))) continue;
        renderLine(_scanHeap->value(_uidList->at(y)), _lineBuffer, y);
    }
}

void WaterfallPlotWidget::renderLine(const Scan &sc, QImage &img, int y)
{
    if (img.width() < 1) return;

    int imgSz = img.width();
    const QVector<double> *dataAng = &(sc.pDataAngle());
    const QVector<double> *dataInt = &(sc.pDataIntensity());

    int dx = angleToBufferIndex(dataAng->at(0));

    for (int i = 0; i < qMin(dataAng->size(), dataInt->size()); ++i) {
        int x = angleToBufferIndex(dataAng->at(i));
        if (x < 0) continue;

        QRgb z = pixelValueRgb(dataInt->at(i));

        while (dx <= x) {
            if (dx >= imgSz) break;
            img.setPixel(dx, y, z);
            dx++;
        }
    }
}

/*
 * we only use the scaled buffer if slow (high-quality) smooth scaling is used.
 * Otherwise the scaling (non-smooth or low-quality smooth) is done directly in the
 * paint event. This limits double QPainter::drawImage calls to high-quality smooth cases.
 */
void WaterfallPlotWidget::scaleDisplayBuffer()
{
    if (_smoothInterpol != 2) return;

    if ((_lineBuffer.isNull() || (qFuzzyCompare(_wminDisp, _wmaxDisp)) || (qFuzzyCompare(_wmin, _wmax)))) {
        _scaledBuffer = QImage();
        return;
    }

    double idxBufWmin = angleToBufferIndexF(_wminDisp);
    double idxBufWmax = angleToBufferIndexF(_wmaxDisp);
    double w = idxBufWmax - idxBufWmin;
    double h = double(_lineBuffer.height());

    _scaledBuffer = QImage(_imageRect.width(), _lineBuffer.height(), _lineBuffer.format());

    QPainter p(&_scaledBuffer);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true); // determines horizontal smoothing
    p.drawImage(_scaledBuffer.rect(), _lineBuffer, QRectF(idxBufWmin, 0.0, w, h));
    p.end();

    _scaledBuffer = _scaledBuffer.scaled(_imageRect.width(), _imageRect.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void WaterfallPlotWidget::drawFileNames(QPainter &p)
{
    if (!_showScanLabels) return;
    if (_labels.isEmpty()) return;

    double step    = double(_fileNameRect.height()) / double(_labels.size());
    double yOffset = double(_fileNameRect.top()) + 0.5 * (step + double(fontMetrics().ascent()));
    double lastY   = 0.0;

    for (int i = 0; i < _labels.size(); ++i) {
        double y = yOffset + step * double(i);

        if (y - lastY > fontMetrics().height()) {
            p.drawText(_fileNameRect.x(), int(0.5 + y), _labels.at(i));
            lastY = y;
        }
    }
}

void WaterfallPlotWidget::drawXaxis(QPainter &p)
{
    int s = fontMetrics().lineSpacing();
    QString l = QString("Diffraction Angle [%1%2%3]").arg(global::degree).arg(2).arg(global::theta);

    int tBase    = _xaxisRect.top() + s;
    int tTopMin  = _xaxisRect.top() + int(2.0 * s / 3.0);
    int tTopMaj  = _xaxisRect.top() + int(1.0 * s / 3.0);
    int lBase    = _xaxisRect.top() + 2*s;
    int nBase    = _xaxisRect.top() + 4*s - fontMetrics().descent();

    for (int i = 0; i < _minTicks.size(); ++i) {
        int x = _imageRect.left() + int(0.5 + _imageRect.width() * (_minTicks.at(i) - _wminDisp) / (_wmaxDisp - _wminDisp));
        p.drawLine(x, tBase, x, tTopMin);
    }

    for (int i = 0; i < _majTicks.size(); ++i) {
        int x = _imageRect.left() + int(0.5 + _imageRect.width() * (_majTicks.at(i) - _wminDisp) / (_wmaxDisp - _wminDisp));
        p.drawLine(x, tBase, x, tTopMaj);
        QString tLbl = QString("%1").arg(_majTicks.at(i), 0, 'f', 2);
        p.drawText(x - fontMetrics().horizontalAdvance(tLbl)/2, lBase, tLbl);
    }

    p.drawLine(_xaxisRect.left(), tBase, _xaxisRect.right(), tBase);
    p.drawText(_xaxisRect.left() + _xaxisRect.width() / 2 - fontMetrics().horizontalAdvance(l) / 2, nBase, l);
}

void WaterfallPlotWidget::drawGrid(QPainter &p)
{
    if (!_drawGridH && !_drawGridV) return;

    QPen pen(Qt::white);
    pen.setStyle(Qt::DashLine);
    p.setPen(pen);

    QPainter::CompositionMode cpm = p.compositionMode();
    p.setCompositionMode(QPainter::CompositionMode_Difference);

    if (_drawGridH) {
        double step = double(_imageRect.height()) / double(_labels.size());
        double yOffset = double(_imageRect.top()) + 0.5 * step;

        for (int i = 0; i < _labels.size(); ++i) {
            double y = yOffset + step * i;
            p.drawLine(_imageRect.x(), int(0.5 + y), _imageRect.x() + _imageRect.width(), y);
        }
    }

    if (_drawGridV) {
        for (int i = 0; i < _majTicks.size(); ++i) {
            int x = _imageRect.left() + int(0.5 + _imageRect.width() * (_majTicks.at(i) - _wminDisp) / (_wmaxDisp - _wminDisp));
            p.drawLine(x, _imageRect.top(), x, _imageRect.bottom());
        }
    }

    p.setCompositionMode(cpm);
}

void WaterfallPlotWidget::drawHighlight(QPainter &p)
{
    if (!_hlIdx.size()) return;

    QPen pen(Qt::white);
    pen.setStyle(Qt::DashLine);
    p.setPen(pen);

    QPainter::CompositionMode cpm = p.compositionMode();
    p.setCompositionMode(QPainter::CompositionMode_Multiply);

    double step = double(_imageRect.height()) / double(_labels.size());
    double yOffset = double(_imageRect.top()) + 0.5 * step;
    int w = _imageRect.width() - 1;
    int h = _smoothInterpol ? 1 : int(0.5 + 0.5 * step);
    int x = _imageRect.x();
    QRegion clipReg(_imageRect);

    for (int i = 0; i < _hlIdx.size(); ++i) {
        int n = _uidList->indexOf(_hlIdx.at(i));

        if (n >= 0) {
            int y = int(0.5 + yOffset + step * n);
            QRect r(x, y - h, w, 2 * h);
            clipReg -= QRegion(r, QRegion::Rectangle);
        }
    }

    p.setClipRegion(clipReg);
    p.fillRect(_imageRect, QBrush(Qt::gray));

    p.setCompositionMode(cpm);
}

void WaterfallPlotWidget::calcDrawRegions(const QPaintDevice *pd, const QFontMetrics &fm)
{
    const QPaintDevice *canvas = pd ? pd : this;

    int margin = fm.horizontalAdvance("M");
    int fnh = canvas->height() - 4 * fm.lineSpacing();
    int fnl = 0;
    int tkl = int (double(fm.horizontalAdvance(QString::number(_wmax, 'f', 2)))/2.0);

    if (_showScanLabels) {
        for (int i = 0; i < _labels.size(); ++i) {
            fnl = qMax(fnl, fm.horizontalAdvance(_labels.at(i)) + margin);
        }
    }

    _fileNameRect = QRect(QPoint(margin,       0),   QPoint(fnl + margin,          fnh));
    _imageRect    = QRect(QPoint(margin + fnl, 0),   QPoint(canvas->width() - tkl, fnh));
    _xaxisRect    = QRect(QPoint(margin + fnl, fnh), QPoint(canvas->width() - tkl, canvas->height()));
}

QRgb WaterfallPlotWidget::pixelValueRgb(double v)
{
    double val = qFuzzyCompare(_gamma, 1.0) ? v : pow(v, _gamma);
    int c = _lutOffset + int(_lutSign * double(_lutBins - 1) * (val - _iRangeMinDisp) / (_iRangeMaxDisp - _iRangeMinDisp) + 0.5);

    if (c >= _lut.data.size()) c = _lut.data.size() - 1;
    else if (c < 0)            c = 0;

    return _lut.data.at(c);
}

/*
 * returns the x-index of the pixel containing the angle a
 */
int WaterfallPlotWidget::angleToBufferIndex(double a)
{
    // don't call this function if _wmin == _wmax
    // (check before, for performance reasons)
    return int(_nPoints * (a - _wmin) / (_wmax - _wmin));
}

/*
 * returns the center x-coordinate of the pixel containing the angle a
 */
double WaterfallPlotWidget::angleToBufferIndexF(double a)
{
    // don't call this function if _wmin == _wmax
    // (check before, for performance reasons)
    return  0.5 + (_nPoints - 1) * (a - _wmin) / (_wmax - _wmin);
}

double WaterfallPlotWidget::bufferIndexToAngle(int i)
{
    return _wmin + double(_wmax - _wmin) * double(i) / double(_nPoints);
}

/* screen pixel to angle */
double WaterfallPlotWidget::pixelToAngle(int i)
{
    if (_imageRect.width() == 0) return 0.0;
    return _wminDisp + (_wmaxDisp - _wminDisp) * double(i - _imageRect.x()) / double(_imageRect.width());
}

int WaterfallPlotWidget::angleToIndex(double d)
{
    // don't call this function if _wmin == _wmax
    // (check before, for performance reasons)
    return int(0.5 + double(_nPoints) * (d - _wmin) / (_wmax - _wmin));
}

void WaterfallPlotWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(this->rect(), QBrush(QGuiApplication::palette().color(QPalette::Base)));

    if (!_lineBuffer.isNull()) {
        p.setRenderHint(QPainter::SmoothPixmapTransform, _smoothInterpol == 1);

        if (_smoothInterpol == 2) {
            p.drawImage(_imageRect, _scaledBuffer);
        } else {
            double idxBufWmin = angleToBufferIndexF(_wminDisp);
            double idxBufWmax = angleToBufferIndexF(_wmaxDisp);
            p.drawImage(_imageRect, _lineBuffer, QRectF(idxBufWmin, 0.0, idxBufWmax - idxBufWmin, double(_lineBuffer.height())));
        }

        drawFileNames(p);
        drawXaxis(p);
        drawGrid(p);
        drawHighlight(p);
    }

    p.end();
}

void WaterfallPlotWidget::resizeEvent(QResizeEvent *)
{
    if (!_scanHeap) return;

    if (_scanHeap->size()) {
        calcDrawRegions(this, fontMetrics());
        scaleDisplayBuffer();
        update();
    }
}

void WaterfallPlotWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (!_imageRect.contains(e->pos())) return;

    if (e->buttons() & Qt::LeftButton) {
        double a = _wminDispClicked + (_wmaxDispClicked - _wminDispClicked) * double(e->pos().x() - _imageRect.x()) / double(_imageRect.width());
        double da = a - _clickPos;

        if (_wminDispClicked - da <  _wmin) da = _wminDispClicked - _wmin;
        if (_wmaxDispClicked - da >= _wmax) da = _wmaxDispClicked - _wmax;

        _wminDisp = _wminDispClicked - da;
        _wmaxDisp = _wmaxDispClicked - da;

        calcXtickMarks();
        scaleDisplayBuffer();
        update();
        emit sigXrangeWasZoomed();
    } else {
        QString s;

        if (_labels.size()) {
            int fnIdx = int(_labels.size() * double(e->pos().y() - _imageRect.y()) / double(_imageRect.height()));
            if (fnIdx < 0) fnIdx = 0;
            s = fnIdx >= _labels.size() ? _labels.last() : _labels.at(fnIdx);
        }

        emit sigMouseCoordinates(QString("%1: 2%2 = %3").arg(s).arg(global::theta).arg(pixelToAngle(e->pos().x()), 0, 'f', 4));
    }
}

void WaterfallPlotWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        _clickPos = pixelToAngle(e->pos().x());
        _wminDispClicked = _wminDisp;
        _wmaxDispClicked = _wmaxDisp;
    } else if (e->button() == Qt::RightButton) {
        resetXrange();
    }
}

void WaterfallPlotWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        _clickPos = pixelToAngle(e->pos().x());
    }
}

void WaterfallPlotWidget::wheelEvent(QWheelEvent *e)
{
    if (!_imageRect.contains(e->position().toPoint())) return;

    if (e->modifiers().testFlag(Qt::NoModifier)) {
        zoomStep(e->position().toPoint().x(), e->angleDelta().y());
        e->accept();
        return;
    }
}

void WaterfallPlotWidget::zoomStep(int x, int n)
{
    double pAng = pixelToAngle(x);

    double dMin = pAng - _wminDisp;
    double dMax = _wmaxDisp - pAng;

    if (n > 0) {
        _wminDisp = qMax(_wmin, pAng - 0.9 * dMin);
        _wmaxDisp = qMin(_wmax, pAng + 0.9 * dMax);
    } else {
        _wminDisp = qMax(_wmin, pAng - 1.1 * dMin);
        _wmaxDisp = qMin(_wmax, pAng + 1.1 * dMax);
    }

    calcXtickMarks();
    scaleDisplayBuffer();
    update();
    emit sigXrangeWasZoomed();
}

void WaterfallPlotWidget::save(const QString &s, int w, int h)
{
    QFileInfo fi(s);
    if (fi.suffix().toLower() == "png") renderPixmap(fi.absoluteFilePath(), w, h);
    if (fi.suffix().toLower() == "svg") renderSvg(fi.absoluteFilePath());
    if (fi.suffix().toLower() == "grd") exportTextGrid(fi.absoluteFilePath());
}

QImage WaterfallPlotWidget::bufferedPixmap(int w, int h)
{
    QImage img(w, h, QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, true);

    if (!_lineBuffer.isNull()) {
        if (_smoothInterpol == 2) {
            p.drawImage(_imageRect, _scaledBuffer);
        } else {
            p.setRenderHint(QPainter::SmoothPixmapTransform, _smoothInterpol == 1);
            double idxBufWmin = angleToBufferIndexF(_wminDisp);
            double idxBufWmax = angleToBufferIndexF(_wmaxDisp);
            p.drawImage(_imageRect, _lineBuffer, QRectF(idxBufWmin, 0.0, idxBufWmax - idxBufWmin, double(_lineBuffer.height())));
        }
    }

    p.end();
    return img;
}

void WaterfallPlotWidget::renderPixmap(const QString &s, int w, int h)
{
    QPixmap pix(w, h);
    calcDrawRegions(&pix, fontMetrics());

    QPainter p(&pix);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, true);
    p.fillRect(pix.rect(), QBrush(Qt::white));

    p.drawImage(pix.rect(), bufferedPixmap(w, h));
    drawFileNames(p);
    drawXaxis(p);
    if (_drawGridH) drawGrid(p);

    p.end();
    calcDrawRegions(this, fontMetrics());

    if (pix.save(s, "PNG")) {
        qDebug() << QString("WaterfallPlotWidget::renderPixmap: Image saved to %1").arg(s);
    } else {
        qDebug() << QString("WaterfallPlotWidget::renderPixmap: Could not save image to %1").arg(s);
    }
}

void WaterfallPlotWidget::renderSvg(const QString &s)
{
    QBuffer buf;
    buf.open(QBuffer::ReadWrite);
    QSvgGenerator svg;
    svg.setOutputDevice(&buf);

    // using an aspect ratio of sqrt(2) : 1
    double w = 27.0 * double(svg.resolution()) / 2.54;
    double h = w / sqrt(2.0);
    QRectF pageRect(0.0, 0.0, w, h);

    svg.setSize(QSize(int(w), int(h)));
    svg.setViewBox(pageRect);

    QPainter p(&svg);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, true);
    QFont ft = font();
    ft.setPointSize(12);
    p.setFont(ft);
    calcDrawRegions(&svg, p.fontMetrics());

    p.drawImage(pageRect, bufferedPixmap(int(w), int(h)));
    drawFileNames(p);
    drawXaxis(p);
    if (_drawGridH) drawGrid(p);
    p.end();

    QFile f(s);

    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))     {
        QTextStream str(&f);
        buf.seek(0);
        str << buf.readAll();
        f.close();
    }

    buf.close();

    calcDrawRegions(this, fontMetrics());
}

void WaterfallPlotWidget::exportTextGrid(const QString &f)
{
    if ((_nPoints <= 1) || (qFuzzyCompare(_wmin, _wmax))) {
        qDebug() << QString("WaterfallPlotWidget::exportTextGrid(): No data available. Cannot export to Surfer 6 Text Grid file.");
        return;
    }

    QString s("DSAA\n");

    int pxMin = angleToIndex(_wminDisp);
    int pxMax = angleToIndex(_wmaxDisp);
    if ((pxMin < 0) || (pxMax < 0)) return;

    int nx = pxMax - pxMin;
    int ny = _uidList->size();

    double xlo = _wminDisp;
    double xhi = _wmaxDisp;
    double ylo = 0.0;
    double yhi = double(_uidList->size() - 1);
    double zlo = 0.0;
    double zhi = _imax;

    s += QString("%1 %2\n").arg(nx).arg(ny);
    s += QString("%1 %2\n").arg(xlo, 0, 'f', 6).arg(xhi, 0, 'f', 6);
    s += QString("%1 %2\n").arg(ylo, 0, 'f', 6).arg(yhi, 0, 'f', 6);
    s += QString("%1 %2\n").arg(zlo, 0, 'f', 6).arg(zhi, 0, 'f', 6);

    for(int i = 0; i < _uidList->size(); ++i) {
        if (!_scanHeap->contains(_uidList->at(i))) continue;

        QStringList l;

        for (int j = 0; j < nx; ++j) {
            double ang = _wminDisp + (double(j) / double(nx - 1)) * (_wmaxDisp - _wminDisp);
            double intens = _scanHeap->value(_uidList->at(i)).intensity(ang);

            l.append(QString::number(intens, 'f', 2));
        }

        s += l.join(" ") + "\n";
    }

    BgmnFileIO::writeTextFile(f, s);
    qDebug() << QString("WaterfallPlotWidget::exportTextGrid(): Data exported to %1 (%2 characters in string)").arg(f).arg(s.length());
}

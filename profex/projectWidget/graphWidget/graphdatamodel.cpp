/***************************************************************************
                          graphdatamodel.cpp  -  description
                             -------------------
    begin                : Tue Feb 11 18:00:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "graphdatamodel.h"
#include "../libXrdIO/import/importhandler.h"
#include "../libXrdIO/export/exporthandler.h"
#include "../libXrdIO/scanops.h"
#include "../libXrdIO/import/bgmndiaimport.h"
#include "../libXrdIO/functions.h"
#include <QFileInfo>
#include <QDebug>
#include <limits>

GraphDataModel::GraphDataModel()
{
    graphHeap = new QVector<Scan>;
    graphBuffer = new QVector<Scan>;
    strucRefls = new QVector<Hkl>;
    integralRanges = new QVector<QPointF>;
    waveLengthTable = new QVector<QVector<double> >();
    settings = SettingsManager::getInstance();
    wlMode = Scan::WavelengthMode::UNKNOWN;

    QList<global::CharWaveLength> wls = global::Functions::getAllWavelengths();
    for (int i = 0; i < wls.size(); ++i) {
        QVector<double> v;
        v.append(10.0 * wls.at(i).ka1); // convert from nm to A
        v.append(10.0 * wls.at(i).ka2);
        v.append(10.0 * wls.at(i).kb);
        waveLengthTable->append(v);
    }

    globalXrangeMin = 0.0;
    globalXrangeMax = 0.0;
    globalYrangeMin = 0.0;
    globalYrangeMax = 0.0;

    overrideWavelenght = -1.0;

    eps1 = 0.0;
    eps2 = 0.0;
    eps3 = 0.0;

    stacked = false;
    nFiles = 0;
}

GraphDataModel::~GraphDataModel()
{
    delete graphHeap;
    delete graphBuffer;
    delete strucRefls;
    delete integralRanges;
    delete waveLengthTable;
}

int GraphDataModel::count() const
{
    return graphHeap->size();
}

bool GraphDataModel::hasData() const
{
    return graphHeap->size() > 0;
}

bool GraphDataModel::hasScanData() const
{
    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).hasScanData()) return true;
    }

    return false;
}

bool GraphDataModel::hasHklData() const
{
    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).hasHklData()) return true;
    }

    return false;
}

QList<int> GraphDataModel::baseLines() const
{
    QList<int> l;

    for (int i = 0; i < graphHeap->size(); ++i) {
        const Scan s = graphHeap->at(i);
        bool ok;

        bool isBl = s.auxInfo("isBaseLine", ok).toBool();
        if (isBl && ok) l.append(i);
    }

    return l;
}

void GraphDataModel::keepTemporary()
{
    for (int i = 0; i < graphHeap->size(); ++i) {
        Scan::ScanTypes flags = graphHeap->at(i).scanTypes();

        if (flags.testFlag(Scan::TEMPORARY)) {
            (*graphHeap)[i].setTypes(Scan::XY | Scan::SYNTHETIC);
        }
    }
}

void GraphDataModel::keepTemporary(const QList<QUuid> &uids)
{
    for (int i = 0; i < uids.size(); ++i) {
        Scan *scan = getScan(uids.at(i));
        if (scan) scan->setTypes(Scan::XY | Scan::SYNTHETIC);
    }
}

void GraphDataModel::clearTemporary()
{
    for (int i = graphHeap->size() - 1; i >= 0; --i) {
        Scan::ScanTypes flags = graphHeap->at(i).scanTypes();

        if (flags.testFlag(Scan::TEMPORARY)) {
            graphHeap->remove(i);
        }
    }
}

void GraphDataModel::removeScans(const QList<QUuid> &uids)
{
    for (int i = 0; i < uids.size(); ++i) {
        removeScan(uids.at(i));
    }
}

void GraphDataModel::setOverrideWavelength(double d, const Scan::WavelengthMode &wlm)
{
    overrideWavelenght = d;
    wlMode = wlm;
}

double GraphDataModel::getWaveLength(double fallback)
{
    if (overrideWavelenght > 0.0001) {
        return overrideWavelenght;
    }

    if (graphHeap->size()) {
        double wl = graphHeap->first().waveLength();

        if (qFuzzyIsNull(wl)) {
            return fallback;
        }

        return wl;
    }

    return fallback;
}

bool GraphDataModel::getNearestCharacteristicWaveLength(double &ka1, double &ka2, double &kb, double currentWl)
{
    double tolerance = currentWl > 1.0 ? 0.05 : 0.02;

    for (int i = 0; i < waveLengthTable->size(); ++i) {
        if (qAbs(waveLengthTable->at(i).at(0) - currentWl) < tolerance) {
            ka1 = waveLengthTable->at(i).at(0);
            ka2 = waveLengthTable->at(i).at(1);
            kb  = waveLengthTable->at(i).at(2);
            return true;
        }
    }

    ka1 = currentWl;
    ka2 = currentWl;
    kb  = currentWl;
    return false;
}

int GraphDataModel::loadScanFile(const QString &s, const QString &uid, bool min, const QString &sid)
{
    QList<bool> vis;
    QList<bool> act;
    QList<double> yoff;
    QList<int> pos;

    getScanVisParameters(vis, act, yoff, pos);

    QFileInfo fi(s);
    QString fext = fi.suffix().toLower();

    *graphBuffer = *graphHeap;
    metricsBuffer = scanMetrics;

    graphHeap->clear();
    scanMetrics.clear();

    int n = addScanFile(fi, uid, min);
    applyScanVisParameters(QList<bool>(), act, QList<double>(), QList<int>());

    if (n <= 0) {
        // if reloading failed (probably due to timing issues), we roll back the
        // buffered version of the file to avoid a blank screen
        *graphHeap = *graphBuffer;
        scanMetrics = metricsBuffer;
    }

    if (n > 0) {
        if ((fext == "prf") || (fext == "dia")) {
            sampleId = sid;
        } else {
            sampleId = graphHeap->first().name();
        }
    }

    return n;
}

int GraphDataModel::loadDiaFile(const QString &s, bool min, const QString &sid, int &ll, int &tl)
{
    QList<bool> vis;
    QList<bool> act;
    QList<double> yoff;
    QList<int> pos;

    getScanVisParameters(vis, act, yoff, pos);

    QFileInfo fi(s);
    QString fext = fi.suffix().toLower();

    *graphBuffer = *graphHeap;
    metricsBuffer = scanMetrics;

    graphHeap->clear();
    scanMetrics.clear();

    int n = addDiaFile(fi, "BGMN_DIA", min, ll, tl);
    applyScanVisParameters(QList<bool>(), act, QList<double>(), QList<int>());

    if (n <= 0) {
        // if reloading failed (probably due to timing issues), we roll back the
        // buffered version of the file to avoid a blank screen
        *graphHeap = *graphBuffer;
        scanMetrics = metricsBuffer;
    } else {
        if ((fext == "prf") || (fext == "dia")) {
            sampleId = sid;
        } else {
            sampleId = graphHeap->first().name();
        }
    }

    return n;
}

bool GraphDataModel::saveScanFile(const QString &f, const QString &uid, int n, const QMap<QString, QVariant> &flags)
{
    if (n >= graphHeap->size()) {
        return false;
    }

    ExportHandler exhandler;

    if (n < 0) {
        return exhandler.save(uid, f, *graphHeap, flags);
    }

    return exhandler.save(uid, f, (*graphHeap)[n], flags);
}

int GraphDataModel::addScanFile(const QString &s, const QString &uid, bool min)
{
    return addScanFile(QFileInfo(s), uid, min);
}

int GraphDataModel::addScanFile(const QFileInfo &fi, const QString &uid, bool min)
{
    QString fuid = uid;
    ImportHandler iHandler;

    if (fuid.isEmpty()) {
        fuid = iHandler.uidByFileName(fi.absoluteFilePath());
    }

    completeFileName = fi.absoluteFilePath();
    fileUid = fuid;

    // some import filters clear the graphHeap. Here we don't want this, so we
    // first load the file to a temporary graphHeap and then append it to
    // the main graphHeap.
    QVector<Scan> *tGraphHeap = new QVector<Scan>;

    int n = iHandler.load(completeFileName, fileUid, *tGraphHeap, min);
    if (n < 1) return n;

    graphHeap->append(*tGraphHeap);
    delete tGraphHeap;

    if (!min) {
        iHandler.getReflections(completeFileName, *graphHeap);
    }

    for (int i = graphHeap->size() - n; i < graphHeap->size(); ++i) {
        Scan *scan = &(*graphHeap)[i];
        scan->setPosition(i);
        QVector<double> smetr(4, 0.0);
        ScanOps::scanMetrics(graphHeap->at(i), smetr[0], smetr[1], smetr[2], smetr[3]);
        scanMetrics[graphHeap->at(i).uid()] = smetr;

        if (scan->wavelengthMode() != Scan::WavelengthMode::UNKNOWN) {
            wlMode = scan->wavelengthMode();
        }
    }

    countFiles();
    updateMetrics();
    updateScanColors(true);
    updateScanStyles(true);

    return n;
}

int GraphDataModel::addDiaFile(const QString &s, const QString &uid, bool min, int &ll, int &tl)
{
    return addDiaFile(QFileInfo(s), uid, min, ll, tl);
}

int GraphDataModel::addDiaFile(const QFileInfo &fi, const QString &uid, bool min, int &ll, int &tl)
{
    fileUid = uid;
    BgmnDiaImport diaImport;
    ImportHandler iHandler;

    completeFileName = fi.absoluteFilePath();

    int n = diaImport.loadAndCheck(completeFileName, *graphHeap, ll, tl, min);
    if (n < 1) return n;

    if (!min) {
        iHandler.getReflections(completeFileName, *graphHeap);
    }

    for (int i = graphHeap->size() - n; i < graphHeap->size(); ++i) {
        Scan *scan = &(*graphHeap)[i];
        scan->setPosition(i);
        QVector<double> smetr(4, 0.0);
        ScanOps::scanMetrics(graphHeap->at(i), smetr[0], smetr[1], smetr[2], smetr[3]);
        scanMetrics[graphHeap->at(i).uid()] = smetr;

        if (scan->wavelengthMode() != Scan::WavelengthMode::UNKNOWN) {
            wlMode = scan->wavelengthMode();
        }
    }

    countFiles();
    updateMetrics();
    updateScanColors(true);
    updateScanStyles(true);

    return n;
}
int GraphDataModel::reloadScanFile(bool min)
{
    QList<bool> vis;
    QList<bool> act;
    QList<double> yoff;
    QList<int> pos;

    getScanVisParameters(vis, act, yoff, pos);

    *graphBuffer = *graphHeap;
    metricsBuffer = scanMetrics;

    graphHeap->clear();
    scanMetrics.clear();
    nFiles = 0;

    int n = addScanFile(QFileInfo(completeFileName), fileUid, min);

    if (n <= 0) {
        // if reloading failed (probably due to timing issues), we roll back the
        // buffered version of the file to avoid a blank screen
        *graphHeap = *graphBuffer;
        scanMetrics = metricsBuffer;
    }

    applyScanVisParameters(vis, act, yoff, pos);

    countFiles();
    updateMetrics();
    updateScanColors(true);
    updateScanStyles(true);

    return n;
}

int GraphDataModel::reloadDiaFile(bool min, int &ll, int &tl)
{
    QList<bool> vis;
    QList<bool> act;
    QList<double> yoff;
    QList<int> pos;

    getScanVisParameters(vis, act, yoff, pos);

    *graphBuffer = *graphHeap;
    metricsBuffer = scanMetrics;

    graphHeap->clear();
    scanMetrics.clear();
    nFiles = 0;

    int n = addDiaFile(QFileInfo(completeFileName), fileUid, min, ll, tl);

    if (n <= 0) {
        // if reloading failed (probably due to timing issues), we roll back the
        // buffered version of the file to avoid a blank screen
        *graphHeap = *graphBuffer;
        scanMetrics = metricsBuffer;
    } else {
        applyScanVisParameters(vis, act, yoff, pos);
    }

    countFiles();
    return n;
}

/*
 * vis = visibility (bool)
 * act = active (bool)
 * yoff = y-offset (double)
 * pos = position in scan list (int)
 */
void GraphDataModel::getScanVisParameters(QList<bool> &vis, QList<bool> &act, QList<double> &yoff, QList<int> &pos)
{
    vis.clear();
    act.clear();
    yoff.clear();
    pos.clear();

    for (int i = 0; i < graphHeap->size(); ++i) {
        vis.append(graphHeap->at(i).isVisible());
        act.append(graphHeap->at(i).isActive());
        yoff.append(graphHeap->at(i).yOffset());
        pos.append(graphHeap->at(i).getPosition());
    }
}

void GraphDataModel::applyScanVisParameters(const QList<bool> &vis, const QList<bool> &act, const QList<double> &yoff, const QList<int> &pos)
{
    for (int i = 0; i < graphHeap->size();  ++i) {
        if (i < vis.size()) (*graphHeap)[i].setVisible(vis.at(i));
        if (i < act.size()) (*graphHeap)[i].setActive(act.at(i));
        if (i < pos.size()) (*graphHeap)[i].setPosition(pos.at(i));

        if (i < yoff.size()) {
            if ((*graphHeap)[i].yOffset() >= 0.0) {
                // skip negative scans, otherwise the difference curve
                // would not move closer to the base line as the fit
                // gets better
                (*graphHeap)[i].setYoffset(yoff.at(i));
            }
        }
    }
}

void GraphDataModel::appendScan(const Scan &s)
{
    int n = graphHeap->size();
    graphHeap->append(s);
    (*graphHeap)[n].setPosition(n);

    countFiles();
    updateMetrics();
    updateScanColors(true);
}

void GraphDataModel::appendHklScan(const Scan &s)
{
    Scan hklScan(s);
    hklScan.setHklXunit("dnm");
    hklScan.setPosition(graphHeap->size());
    normalizeHklScan(&hklScan);
    graphHeap->append(hklScan);
    countFiles();
    updateMetrics();
    updateScanColors(true);
    updateScanStyles(true);
}

void GraphDataModel::setAllHklDisplayStatus(int i)
{
    for (int s = 0; s < graphHeap->size(); ++s) {
        for (int h = 0; h < graphHeap->at(s).pDataHkl().size(); ++h)  {
            (*graphHeap)[s].pDataHkl()[h].setStatus(i);
        }
    }
}

void GraphDataModel::replaceScan(int n, const Scan &s)
{
    if ((n >= graphHeap->size()) || (n < 0)) {
        appendScan(s);
    } else {
        graphHeap->replace(n, s);
        (*graphHeap)[n].setUid(s.uid());
        (*graphHeap)[n].setPosition(n);
        updateMetrics();
        updateScanColors(true);
        updateScanStyles(true);
    }
}

void GraphDataModel::replaceHklScan(int n, const Scan &s)
{
    Scan hklScan(s);
    hklScan.setHklXunit("dnm");
    normalizeHklScan(&hklScan);
    replaceScan(n, hklScan);
}

void GraphDataModel::removeScan(int n)
{
    if ((n >= graphHeap->size()) || (n < 0)) return;
    graphHeap->remove(n);

    countFiles();
    updateMetrics();
    updateScanColors(true);
    updateScanStyles(true);
}

void GraphDataModel::removeScan(const QUuid &uid)
{
    for (int i = graphHeap->size() - 1; i >= 0; --i) {
        if (graphHeap->at(i).uid() == uid) {
            graphHeap->remove(i);

            countFiles();
            updateMetrics();
            updateScanColors(true);
            updateScanStyles(true);
            return;
        }
    }
}

void GraphDataModel::updateMetrics()
{
    globalXrangeMin = std::numeric_limits<double>::max();
    globalXrangeMax = std::numeric_limits<double>::min();
    globalYrangeMin = std::numeric_limits<double>::max();
    globalYrangeMax = std::numeric_limits<double>::min();

    QMapIterator<QUuid, QVector<double> > it(scanMetrics);

    while (it.hasNext()) {
        it.next();
        globalXrangeMin = qMin(globalXrangeMin, it.value().at(0));
        globalXrangeMax = qMax(globalXrangeMax, it.value().at(1));
        globalYrangeMin = qMin(globalYrangeMin, it.value().at(2));
        globalYrangeMax = qMax(globalYrangeMax, it.value().at(3));
    }
}

void GraphDataModel::updateScanColors(bool force)
{
    QList<QColor> colorList = settings->getColorList(graphHeap->size());

    for (int i = 0; i < graphHeap->size(); ++i) {
        Scan * scan = &((*graphHeap)[i]);
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

void GraphDataModel::updateScanStyles(bool force)
{
    QList<int> styleList = settings->getScanStyleList(graphHeap->size());

    for (int i = 0; i < graphHeap->size(); ++i) {
        Scan * scan = &((*graphHeap)[i]);
        if (!scan) continue;

        if (scan->pointSymbol() < 0 || force) {
            scan->setPointSymbol(styleList.at(i));
        }
    }
}

void GraphDataModel::normalizeHklScan(Scan *scan)
{
    double _ymaxHkl = 0.0;

    for (int i = 0; i < scan->pDataHkl().size(); ++i) {
        _ymaxHkl = qMax(_ymaxHkl, scan->pDataHkl().at(i).intensity());
    }

    double _ymaxScan = qFuzzyIsNull(globalYrangeMax) ? 100.0 : globalYrangeMax;

    for (int i = 0; i < scan->pDataHkl().size(); ++i) {
        double _d = scan->pDataHkl()[i].intensity();
        scan->pDataHkl()[i].setIntensity(_ymaxScan * _d / _ymaxHkl);
    }
}

Scan * GraphDataModel::getScan(int i)
{
    if ((i >= 0) && (i < graphHeap->size())) {
        return &((*graphHeap)[i]);
    }

    return nullptr;
}

Scan * GraphDataModel::getScan(const QUuid &u)
{
    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).uid() == u) {
            return &((*graphHeap)[i]);
        }
    }

    return nullptr;
}

Scan * GraphDataModel::getFirst()
{
    if (graphHeap->size()) {
        return &((*graphHeap)[0]);
    }

    return nullptr;
}

Scan * GraphDataModel::getLast()
{
    if (graphHeap->size()) {
        return &((*graphHeap)[graphHeap->size() - 1]);
    }

    return nullptr;
}

const Scan * GraphDataModel::first()
{
    if (graphHeap->size()) {
        return &(graphHeap->first());
    }

    return nullptr;
}

const Scan * GraphDataModel::last()
{
    if (graphHeap->size()) {
        return &(graphHeap->last());
    }

    return nullptr;
}

const Scan * GraphDataModel::at(int i)
{
    if ((i >= 0) && (i < graphHeap->size())) {
        return &(graphHeap->at(i));
    }

    return nullptr;
}

const QVector<const Scan *> GraphDataModel::xyScans()
{
    QVector<const Scan *> vec;

    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).hasScanData()) {
            vec.append(&(graphHeap->at(i)));
        }
    }

    return vec;
}

const QVector<const Scan *> GraphDataModel::hklScans()
{
    QVector<const Scan *> vec;

    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).hasHklData()) {
            vec.append(&(graphHeap->at(i)));
        }
    }

    return vec;
}

const QVector<const Scan *> GraphDataModel::allScans()
{
    QVector<const Scan *> vec;

    for (int i = 0; i < graphHeap->size(); ++i) {
        vec.append(&(graphHeap->at(i)));
    }

    return vec;
}

const QVector<const Scan *> GraphDataModel::phaseScans()
{
    QVector<const Scan *> vec;

    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).scanTypes().testFlag(Scan::PHASE)) {
            vec.append(&(graphHeap->at(i)));
        }
    }

    return vec;
}

Scan * GraphDataModel::getFirstXy()
{
    if (!graphHeap->size()) return nullptr;

    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).hasScanData()) return &((*graphHeap)[i]);
    }

    return nullptr;
}

Scan * GraphDataModel::getFirstHkl()
{
    if (!graphHeap->size()) return nullptr;

    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).hasHklData()) return &((*graphHeap)[i]);
    }

    return nullptr;
}

int GraphDataModel::indexOf(const Scan &s) const
{
    if (!graphHeap->size()) return -1;

    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).uid() == s.uid()) {
            return i;
        }
    }

    return -1;
}

int GraphDataModel::indexOf(const QUuid &u) const
{
    if (!graphHeap->size()) return -1;

    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).uid() == u) {
            return i;
        }
    }

    return -1;
}

void GraphDataModel::clear()
{
    graphHeap->clear();
    graphBuffer->clear();
    strucRefls->clear();
    integralRanges->clear();
    integralRangeNames.clear();
    completeFileName = QString();
    fileUid = QString();
    sampleId = QString();
}

QUuid GraphDataModel::scanUid(int i) const
{
    if ((i >= 0) && (i < graphHeap->size())) {
        return graphHeap->at(i).uid();
    }

    return QUuid();
}

void GraphDataModel::setAngularCorrections(double e1, double e2, double e3)
{
    eps1 = e1;
    eps2 = e2;
    eps3 = e3;
}

double GraphDataModel::getAngularCorrEPS1() const
{
    return eps1;
}

double GraphDataModel::getAngularCorrEPS2() const
{
    return eps2;
}

double GraphDataModel::getAngularCorrEPS3() const
{
    return eps3;
}

bool GraphDataModel::sortScanOrder(const QVector<QUuid> &v)
{
    if (!graphHeap->size() || !v.size()) {
        return false;
    }

    // write the new display positions to the scans in graphHeap
    for (int i = 0; i < graphHeap->size(); ++i) {
        Scan *scan = &(*graphHeap)[i];
        int n = v.indexOf(scan->uid(), 0);
        scan->setPosition(n < 0 ? std::numeric_limits<int>::max() : n);
    }

    std::sort(graphHeap->begin(), graphHeap->end(), ScanOps::compareDisplayPosition);

    return true;
}

void GraphDataModel::yOffsetUp(bool firstOnTop)
{
    double initial = 2.0 * getYmax() / 3.0;
    int n = 0;

    for (int i = 0; i < graphHeap->size(); ++i) {
        int idx = firstOnTop ? graphHeap->size() - i - 1 : i;

        Scan *scan = &(*graphHeap)[idx];

        if (!scan) continue;
        if (scan->scanTypes().testFlag(Scan::DIFF)) continue;

        if (scan->yOffset() == 0.0) {
            scan->setYoffset(initial * double(n));
            ++n;
        } else if (scan->yOffset() > 0.0) {
            scan->setYoffset(scan->yOffset() * 1.5);
        }
    }

    stacked = true;
}

void GraphDataModel::yOffsetDown(bool firstOnTop)
{
    if (stacked) {
        for (int i = 0; i < graphHeap->size(); ++i) {
            Scan *scan = &(*graphHeap)[i];
            if (!scan) continue;
            if (scan->scanTypes().testFlag(Scan::DIFF)) continue;

            if (scan->yOffset() > 0.0) {
                scan->setYoffset(scan->yOffset() / 1.5);
            }
        }
    } else {
        // if not stacked, this function will displace each scan to
        // the maximum intensity of the previous scan. Scans with
        // negative yOffset are ignored
        int p = firstOnTop ? graphHeap->size() - 1 : 0;

        for (int i = 0; i < graphHeap->size(); ++i) {
            int idx = firstOnTop ? graphHeap->size() - i - 1 : i;

            Scan *prevScan = &(*graphHeap)[p];
            Scan *scan = &(*graphHeap)[idx];

            if (!scan || !prevScan) continue;
            if (scan->scanTypes().testFlag(Scan::DIFF)) continue;

            if (scan == prevScan) {
                p = idx;
                continue;
            }

            if (scan->yOffset() >= 0.0) {
                double pMx = prevScan->maxIntensity();
                if (pMx < 0.0) pMx = 0.0;

                double d = pMx * prevScan->scaleFactor() + prevScan->yOffset();

                if (d > 0.0) scan->setYoffset(d);
                p = idx;
            }
        }

        stacked = true;
    }
}

void GraphDataModel::xOffsetRight()
{
    double initial = (getXmax() - getXmin()) / 100.0;
    int n = 1;

    for (int i = 1; i < graphHeap->size(); ++i) {
        Scan *scan = &(*graphHeap)[i];
        if (!scan) continue;
        if (scan->scanTypes().testFlag(Scan::DIFF)) continue;

        if (scan->xOffset() == 0.0) {
            scan->setXoffset(initial * double(n));
            ++n;
        }

        if (scan->xOffset() > 0.0) {
            scan->setXoffset(scan->xOffset() * 1.5);
        }

        if (scan->xOffset() < 0.0) {
            scan->setXoffset(scan->xOffset() / 1.5);
        }
    }

    stacked = true;
}

void GraphDataModel::xOffsetLeft()
{
    double initial = -(getXmax() - getXmin()) / 100.0;
    int n = 1;

    for (int i = 1; i < graphHeap->size(); ++i) {
        Scan *scan = &(*graphHeap)[i];
        if (!scan) continue;
        if (scan->scanTypes().testFlag(Scan::DIFF)) continue;

        if (scan->xOffset() == 0.0) {
            scan->setXoffset(initial * double(n));
            ++n;
        }

        if (scan->xOffset() < 0.0) {
            scan->setXoffset(scan->xOffset() * 1.5);
        }

        if (scan->xOffset() > 0.0) {
            scan->setXoffset(scan->xOffset() / 1.5);
        }
    }

    stacked = true;
}

void GraphDataModel::resetOffset()
{
    for (int i = 0; i < graphHeap->size(); ++i) {
        Scan *scan = &(*graphHeap)[i];
        if (!scan) continue;

        scan->setXoffset(0.0);

        if (scan->yOffset() > 0.0) {
            // don't touch negative scans
            scan->setYoffset(0.0);
        }
    }

    stacked = false;
}

double GraphDataModel::getXmin() const
{
    double xmin = std::numeric_limits<double>::max();

    for (int i = 0; i < graphHeap->size(); ++i) {
        xmin = qMin(xmin, graphHeap->at(i).minAngle());
    }

    return xmin;
}

double GraphDataModel::getXmax() const
{
    double xmax = std::numeric_limits<double>::min();

    for (int i = 0; i < graphHeap->size(); ++i) {
        xmax = qMax(xmax, graphHeap->at(i).maxAngle());
    }

    return xmax;
}

double GraphDataModel::getYmin() const
{
    double xmin = std::numeric_limits<double>::max();

    for (int i = 0; i < graphHeap->size(); ++i) {
        double xm = graphHeap->at(i).minIntensity();
        if (xm >= 0.0) xmin = qMin(xmin, xm);
    }

    return xmin < 0.0 ? 0.0 : xmin;
}

double GraphDataModel::getYmax() const
{
    double xmax = std::numeric_limits<double>::min();

    for (int i = 0; i < graphHeap->size(); ++i) {
        double xm = graphHeap->at(i).maxIntensity();
        if (xm >= 0.0) xmax = qMax(xmax, xm);
    }

    return xmax < 0.0 ? 0.0 : xmax;
}

void GraphDataModel::countFiles()
{
    QSet<QString> fn;

    for (int i = 0; i < graphHeap->size(); ++i) {
        fn.insert(graphHeap->at(i).sourceFileName());
    }

    nFiles = fn.size();
}

QVector<Scan *> GraphDataModel::activeScans()
{
    QVector<Scan *> vec;

    for (int i = 0; i < graphHeap->size(); ++i) {
        Scan *sc = &(*graphHeap)[i];
        if (sc->isActive()) vec.append(sc);
    }

    return vec;
}

void GraphDataModel::setActiveScans(const QList<int> &l)
{
    for (int i = 0; i < graphHeap->size(); ++i) {
        Scan *sc = &(*graphHeap)[i];
        if (sc) sc->setActive(l.contains(i));
    }
}

void GraphDataModel::setActiveScans(const QList<QUuid> &l)
{
    for (int i = 0; i < graphHeap->size(); ++i) {
        Scan *sc = &(*graphHeap)[i];
        if (sc) sc->setActive(l.contains(sc->uid()));
    }
}

QList<int> GraphDataModel::activeScanIndices() const
{
    QList<int> l;

    for (int i = 0; i < graphHeap->size(); ++i) {
        if (graphHeap->at(i).isActive()) l.append(i);
    }

    return l;
}

void GraphDataModel::clearActiveScans() const
{
    for (int i = 0; i < graphHeap->size(); ++i) {
        Scan *sc = &(*graphHeap)[i];
        if (sc) sc->setActive(false);
    }
}

int GraphDataModel::backgroundScanIndex()
{
    for (int i = 0; i < graphHeap->size(); ++i) {
        Scan::ScanTypes flags = graphHeap->at(i).scanTypes();

        if (flags.testFlag(Scan::BACKGROUND)) return i;
    }

    return -1;
}

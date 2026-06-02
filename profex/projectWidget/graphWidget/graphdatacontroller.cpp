/***************************************************************************
                          graphdatacontroller.cpp  -  description
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

#include "graphdatacontroller.h"
#include "abstractgraphview.h"
#include "../libXrdIO/import/importhandler.h"

GraphDataController::GraphDataController(GraphDataModel *m, QObject *parent) :
    QObject(parent),
    graphModel(m)
{
    updateInProgress = false;
    updatesBlocked = false;
}

void GraphDataController::addView(AbstractGraphView *v)
{
    graphViews.append(v);
}

int GraphDataController::count() const
{
    if (!graphModel) return 0;
    return graphModel->count();
}

int GraphDataController::countFiles() const
{
    if (!graphModel) return 0;
    return graphModel->numberOfFiles();
}

int GraphDataController::loadScanFile(const QString &s, const QString &uid, bool min, const QString &sid, bool upd)
{
    if (!graphModel) return -1;

    int n = graphModel->loadScanFile(s, uid, min, sid);
    if (upd) {
        resetViews();
        updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
    }

    return n;
}

int GraphDataController::loadDiaFile(const QString &s, bool min, const QString &sid, bool upd, int &ll, int &tl)
{
    if (!graphModel) return -1;

    int n = graphModel->loadDiaFile(s, min, sid, ll, tl);
    if (upd) {
        resetViews();
        updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
    }

    return n;
}

bool GraphDataController::saveScanFile(const QString &f, const QString &uid, int n, const QMap<QString, QVariant> &flags)
{
    if (!graphModel) return false;
    return graphModel->saveScanFile(f, uid, n, flags);
}

int GraphDataController::addScanFile(const QString &s, const QString &uid, bool min, bool upd)
{
    if (!graphModel) return -1;

    int n = graphModel->addScanFile(s, uid, min);
    // scans read from files are never temporary, thus request full update
    if (upd) updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);

    return n;
}

void GraphDataController::appendScan(const Scan &s, bool upd)
{
    if (!graphModel) return;

    graphModel->appendScan(s);

    if (upd) {
        QSet<global::ViewUpdateMode> updModes;
        updModes << global::ViewUpdateMode::DISPLAY;

        if (!s.scanTypes().testFlag(Scan::TEMPORARY)) {
            updModes << global::ViewUpdateMode::RESULTS;
        }

        updateViews(updModes);
    }
}

void GraphDataController::appendHklScan(const Scan &s, bool upd)
{
    if (!graphModel) return;

    graphModel->appendHklScan(s);

    if (upd) {
        QSet<global::ViewUpdateMode> updModes;
        updModes << global::ViewUpdateMode::DISPLAY;

        if (!s.scanTypes().testFlag(Scan::TEMPORARY)) {
            updModes << global::ViewUpdateMode::RESULTS;
        }

        updateViews(updModes);
    }
}

void GraphDataController::replaceScan(int n, const Scan &s, bool upd)
{
    if (!graphModel) return;

    graphModel->replaceScan(n, s);
    if (upd) updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

void GraphDataController::replaceHklScan(int n, const Scan &s, bool upd)
{
    if (!graphModel) return;

    graphModel->replaceHklScan(n, s);
    if (upd) updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

void GraphDataController::setAllHklDisplayStatus(int i, bool upd)
{
    if (!graphModel) return;

    graphModel->setAllHklDisplayStatus(i);
    if (upd) updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

void GraphDataController::removeScan(int n, bool upd)
{
    if (!graphModel) return;
    graphModel->removeScan(n);
    if (upd) updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
}

void GraphDataController::removeScan(const QUuid &u, bool upd)
{
    if (!graphModel) return;
    graphModel->removeScan(u);
    if (upd) updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
}

int GraphDataController::reloadScanFile(bool min, bool upd)
{
    int n = -1;

    if (!graphModel) return n;

    n = graphModel->reloadScanFile(min);
    if (upd) updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
    return n;
}

int GraphDataController::reloadDiaFile(bool min, bool upd, int &ll, int &tl)
{
    int n = -1;

    if (!graphModel) return n;

    n = graphModel->reloadDiaFile(min, ll, tl);
    if (upd) updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
    return n;
}

/*
 * the sender will be skipped. set it to nullptr if the sender should be updated too
 */
void GraphDataController::updateViews(const QSet<global::ViewUpdateMode> &modes, const AbstractGraphView *sender)
{
    if (updateInProgress || updatesBlocked) return;
    updateInProgress = true;

    for (int i = 0; i < graphViews.size(); ++i) {
        AbstractGraphView *view = dynamic_cast<AbstractGraphView*>(graphViews.at(i));

        if (sender == view) continue;

        // check if view contains any of the view modes to be updated
        for (auto it = modes.cbegin(); it != modes.cend(); ++it) {
            if (view->viewModes().contains(*it)) {
                view->updateView();
                break;
            }
        }
    }

    emit dataUpdated();
    updateInProgress = false;
}

/*
 * the sender will be skipped. set it to nullptr if the sender should be updated too
 */
void GraphDataController::resetViews(const AbstractGraphView *sender)
{
    for (int i = 0; i < graphViews.size(); ++i) {
        if (sender != graphViews.at(i))
            graphViews.at(i)->resetView();
    }
}

void GraphDataController::setViewStatus(const global::RefinementStatus &n)
{
    for (int i = 0; i < graphViews.size(); ++i) {
        if (graphViews.at(i)->setStatus(n)) {
            graphViews.at(i)->updateView();
        }
    }
}

QString GraphDataController::fileName() const
{
    if (!graphModel) return QString();
    return graphModel->fileInfo().absoluteFilePath();
}

QFileInfo GraphDataController::fileInfo() const
{
    if (!graphModel) return QFileInfo();
    return graphModel->fileInfo();
}

bool GraphDataController::hasData() const
{
    if (!graphModel) return false;
    return graphModel->hasData();
}

bool GraphDataController::hasScanData() const
{
    if (!graphModel) return false;
    return graphModel->hasScanData();
}

bool GraphDataController::hasHklData() const
{
    if (!graphModel) return false;
    return graphModel->hasHklData();
}

QList<int> GraphDataController::baseLines() const
{
    if (!graphModel) return QList<int>();
    return graphModel->baseLines();
}

void GraphDataController::setOverrideWavelength(double d, const Scan::WavelengthMode &wlm)
{
    if (!graphModel) return;
    graphModel->setOverrideWavelength(d, wlm);
}

double GraphDataController::getWaveLength(double fallback)
{
    if (!graphModel) return fallback;
    return graphModel->getWaveLength(fallback);
}

double GraphDataController::getTimePerStep(int i)
{
    if (!graphModel) return -1.0;
    if (i >= graphModel->count()) return -1.0;
    return graphModel->at(i)->timePerStep();
}

Scan * GraphDataController::getScan(int i)
{
    if (!graphModel) return nullptr;
    return graphModel->getScan(i);
}

Scan * GraphDataController::getScan(const QUuid &u)
{
    if (!graphModel) return nullptr;
    return graphModel->getScan(u);
}

Scan * GraphDataController::getScan(const QString &s)
{
    if (!graphModel) return nullptr;

    for (int i = 0; i < graphModel->count(); ++i) {
        if (graphModel->at(i)->name() == s) {
            return graphModel->getScan(i);
        }
    }

    return nullptr;
}

QStringList GraphDataController::getAllScanNames() const
{
    if (!graphModel) return QStringList();

    QStringList l;
    const QVector<const Scan *> v = graphModel->allScans();

    for (int i = 0; i < v.size(); ++i) {
        l.append(v.at(i)->name());
    }

    return l;
}

QStringList GraphDataController::getXyScanNames() const
{
    if (!graphModel) return QStringList();

    QStringList l;
    const QVector<const Scan *> v = graphModel->xyScans();

    for (int i = 0; i < v.size(); ++i) {
        l.append(v.at(i)->name());
    }

    return l;
}

QStringList GraphDataController::getHklScanNames() const
{
    if (!graphModel) return QStringList();

    QStringList l;
    const QVector<const Scan *> v = graphModel->hklScans();

    for (int i = 0; i < v.size(); ++i) {
        l.append(v.at(i)->name());
    }

    return l;
}

Scan * GraphDataController::getFirst()
{
    if (!graphModel) return nullptr;
    return graphModel->getFirst();
}

Scan * GraphDataController::getLast()
{
    if (!graphModel) return nullptr;
    return graphModel->getLast();
}

const Scan * GraphDataController::first()
{
    if (!graphModel) return nullptr;
    return graphModel->first();
}

const Scan * GraphDataController::last()
{
    if (!graphModel) return nullptr;
    return graphModel->last();
}

const Scan * GraphDataController::at(int i)
{
    if (!graphModel) return nullptr;
    return graphModel->at(i);
}

Scan * GraphDataController::getFirstXy()
{
    if (!graphModel) return nullptr;
    return graphModel->getFirstXy();
}

Scan * GraphDataController::getFirstHkl()
{
    if (!graphModel) return nullptr;
    return graphModel->getFirstHkl();
}

int GraphDataController::indexOf(const Scan &s) const
{
    if (!graphModel) return -1;
    return graphModel->indexOf(s);
}

int GraphDataController::indexOf(const QUuid &u) const
{
    if (!graphModel) return -1;
    return graphModel->indexOf(u);
}

QUuid GraphDataController::scanUid(int i) const
{
    if (!graphModel) return QUuid();
    return graphModel->scanUid(i);
}

QString GraphDataController::scanName(const QUuid &u)
{
    bool withFileName = countFiles() > 1;
    const Scan *scan = getScan(u);
    if (scan) return scan->name(withFileName);
    return QString();
}

QString GraphDataController::scanName(const Scan &s) const
{
    bool withFileName = countFiles() > 1;
    return s.name(withFileName);
}

QString GraphDataController::scanName(const Scan *s) const
{
    if (!s) return QString();
    bool withFileName = countFiles() > 1;
    return s->name(withFileName);
}

void GraphDataController::updateScanColors(bool force) const
{
    if (!graphModel) return;
    graphModel->updateScanColors(force);
}

void GraphDataController::updateScanStyles(bool force) const
{
    if (!graphModel) return;
    graphModel->updateScanStyles(force);
}

void GraphDataController::setAngularCorrections(double e1, double e2, double e3)
{
    if (!graphModel) return;
    graphModel->setAngularCorrections(e1, e2, e3);
}

double GraphDataController::getAngularCorrEPS1() const
{
    if (!graphModel) return 0.0;
    return graphModel->getAngularCorrEPS1();
}

double GraphDataController::getAngularCorrEPS2() const
{
    if (!graphModel) return 0.0;
    return graphModel->getAngularCorrEPS2();
}

double GraphDataController::getAngularCorrEPS3() const
{
    if (!graphModel) return 0.0;
    return graphModel->getAngularCorrEPS3();
}

QString GraphDataController::getSampleId() const
{
    if (!graphModel) return QString();
    return graphModel->getSampleId();
}

bool GraphDataController::getNearestCharacteristicWaveLength(double &ka1, double &ka2, double &kb, double fixwl)
{
    if (graphModel) return graphModel->getNearestCharacteristicWaveLength(ka1, ka2, kb, fixwl);
    return false;
}

void GraphDataController::setCustomWaveLength(const QMap<QString, double> &m)
{
    if (graphModel) graphModel->setCustomWaveLength(m);
}

void GraphDataController::clearCustomWaveLength()
{
    if (graphModel) graphModel->clearCustomWaveLength();
}

const QMap<QString, double> *GraphDataController::getCustomWaveLength()
{
    if (!graphModel) return nullptr;
    return graphModel->getCustomWaveLength();
}

bool GraphDataController::sortScanOrder(const QVector<QUuid> &v, bool upd, const AbstractGraphView *sender)
{
    if (!v.size())   return true;
    if (!graphModel) return false;

    if (graphModel->sortScanOrder(v)) {
        if (upd) updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS, sender);
        return true;
    }

    return false;
}

double GraphDataController::getXmin() const
{
    if (!graphModel) return 0.0;
    return graphModel->getXmin();
}

double GraphDataController::getXmax() const
{
    if (!graphModel) return 0.0;
    return graphModel->getXmax();
}

double GraphDataController::getYmin() const
{
    if (!graphModel) return 0.0;
    return graphModel->getYmin();
}

double GraphDataController::getYmax() const
{
    if (!graphModel) return 0.0;
    return graphModel->getYmax();
}

void GraphDataController::yOffsetUp(bool firstOnTop)
{
    if (graphModel) graphModel->yOffsetUp(firstOnTop);
}

void GraphDataController::yOffsetDown(bool firstOnTop)
{
    if (graphModel) graphModel->yOffsetDown(firstOnTop);
}

void GraphDataController::xOffsetRight()
{
    if (graphModel) graphModel->xOffsetRight();
}

void GraphDataController::xOffsetLeft()
{
    if (graphModel) graphModel->xOffsetLeft();
}

void GraphDataController::resetOffset()
{
    if (graphModel) graphModel->resetOffset();
}

bool GraphDataController::isStacked() const
{
    if (!graphModel) return false;
    return graphModel->isStacked();
}

void GraphDataController::keepTemporary()
{
    if (graphModel) graphModel->keepTemporary();
}

void GraphDataController::keepTemporary(const QList<QUuid> &l)
{
    if (graphModel) graphModel->keepTemporary(l);
}

void GraphDataController::clearTemporary()
{
    if (graphModel) graphModel->clearTemporary();
}

void GraphDataController::removeScans(const QList<QUuid> &l)
{
    if (graphModel) graphModel->removeScans(l);
}

void GraphDataController::normalizeHklScan(Scan *s)
{
    if (graphModel) graphModel->normalizeHklScan(s);
}

QVector<Scan *> GraphDataController::activeScans()
{
    if (!graphModel) return QVector<Scan*>();
    return graphModel->activeScans();
}

void GraphDataController::setActiveScans(const QList<int> &l)
{
    if (graphModel) graphModel->setActiveScans(l);
}

void GraphDataController::setActiveScans(const QList<QUuid> &l)
{
    if (graphModel) graphModel->setActiveScans(l);
}

QList<int> GraphDataController::activeScanIncides() const
{
    if (!graphModel) return QList<int>();
    return graphModel->activeScanIndices();
}

bool GraphDataController::hasActiveScan() const
{
    if (!graphModel) return false;
    return graphModel->activeScanIndices().size() > 0;
}

Scan * GraphDataController::firstActiveScan()
{
    if (!graphModel) return nullptr;

    QVector<Scan*> aSc = graphModel->activeScans();
    if (aSc.size()) return aSc.first();

    return graphModel->getFirst();
}

void GraphDataController::clearActiveScans() const
{
    if (!graphModel) return;
    graphModel->clearActiveScans();
}

int GraphDataController::backgroundScanIndex() const
{
    if (!graphModel) return -1;
    return graphModel->backgroundScanIndex();
}

Scan * GraphDataController::backgroundScan()
{
    if (!graphModel) return nullptr;
    return graphModel->getScan(graphModel->backgroundScanIndex());
}

void GraphDataController::setScanVisibility(int n, bool v)
{
    if (!graphModel) return;
    Scan *scan = graphModel->getScan(n);
    if (scan) scan->setVisible(v);
}

void GraphDataController::setPhaseVisibility(bool v)
{
    if (!graphModel) return;

    for (int i = 0; i < graphModel->count(); ++i) {
        Scan *scan = graphModel->getScan(i);
        if (!scan) continue;
        if (scan->scanTypes().testFlag(Scan::PHASE)) {
            scan->setVisible(v);
        }
    }
}

void GraphDataController::togglePhaseVisibility()
{
    if (!graphModel) return;

    QList<Scan*> pScans;

    int vis = 0;
    int nvis = 0;

    // check if there are more visible or non-visible phase scans
    for (int i = 0; i < graphModel->count(); ++i) {
        Scan *scan = graphModel->getScan(i);
        if (!scan) continue;
        if (scan->scanTypes().testFlag(Scan::PHASE)) {
            pScans.append(scan);
            if (scan->isVisible()) vis++;
            else                   nvis++;
        }
    }

    bool v = nvis >= vis;

    for (int i = 0; i < pScans.size(); ++i) {
        pScans[i]->setVisible(v);
    }
}

void GraphDataController::setWavelengthMode(const Scan::WavelengthMode &m)
{
    if (graphModel) graphModel->setWavelengthMode(m);
}

Scan::WavelengthMode GraphDataController::getWavelengthMode() const
{
    if (!graphModel) return Scan::WavelengthMode::UNKNOWN;
    return graphModel->getWavelengthMode();
}

const QVector<const Scan *> GraphDataController::xyScans()
{
    if (graphModel) return graphModel->xyScans();
    return QVector<const Scan *>();
}

const QVector<const Scan *> GraphDataController::hklScans()
{
    if (graphModel) return graphModel->hklScans();
    return QVector<const Scan *>();
}

const QVector<const Scan *> GraphDataController::allScans()
{
    if (graphModel) return graphModel->allScans();
    return QVector<const Scan *>();
}

const QVector<const Scan *> GraphDataController::phaseScans()
{
    if (graphModel) return graphModel->phaseScans();
    return QVector<const Scan *>();
}

bool GraphDataController::blockUpdates(bool b)
{
    bool oldState = updatesBlocked;
    updatesBlocked = b;
    return oldState;
}

/*
 * Adds a branch to the domdocument of the following structure:
 *
 * <scans>
 *     <scan Name="Name" sourceFile="fileName" color="#000000" scaling="1.0" vOffset="0.0" hOffset="0.0">
 *         <xAxis start="1.0" end="90.0" count="3000" /> // omitted if sourceFile is in skipList
 *         <yValues>5.0 4.0 7.0 2.0 8.0 ...</yValues> // omitted if sourceFile is in skipList
 *     </scan>
 * </scans>
 */
void GraphDataController::getPreset(QDomDocument &doc, const QStringList &skipList)
{
    if (count() == 0) return;
    QDomElement scEl = doc.createElement("scans");
    scEl.setAttribute("APIversion", "050500");
    doc.documentElement().appendChild(scEl);

    for (int i = 0; i < count(); ++i) {
        const Scan *scan = at(i);
        if (scan->isTemporary()) continue;

        QFileInfo fi(scan->sourceFileName());

        QDomElement elScScan = doc.createElement("scan");
        elScScan.setAttribute("name", scan->name(true));
        elScScan.setAttribute("sourceFile", fi.fileName());  // only file name, no path
        elScScan.setAttribute("color", scan->colorName());
        elScScan.setAttribute("scaling", scan->scaleFactor());
        elScScan.setAttribute("vOffset", scan->yOffset());
        elScScan.setAttribute("hOffset", scan->xOffset());

        scEl.appendChild(elScScan);

        if (!skipList.contains(fi.fileName())) {
            QDomElement elScXaxis   = doc.createElement("xAxis");
            QDomElement elScYvalues = doc.createElement("yValues");

            elScXaxis.setAttribute("start", scan->minAngle());
            elScXaxis.setAttribute("end", scan->maxAngle());
            elScXaxis.setAttribute("count", scan->size());

            elScScan.appendChild(elScXaxis);
            elScScan.appendChild(elScYvalues);

            QStringList yVal;

            for (int j = 0; j < scan->size(); ++j) {
                yVal.append(QString::number(scan->pDataIntensity().at(j), 'f', 6));
            }

            QDomText yValNode = doc.createTextNode(yVal.join(" "));
            elScYvalues.appendChild(yValNode);
        }
    }
}

void GraphDataController::applyPreset(const QDomElement &rootEl, const QString &dir)
{
    if (rootEl.attribute("APIversion") == "050500") {
        qDebug() << QString("GraphDataController::applyPreset(): Reading version 050500 API");
        applyPresetV050500(rootEl, dir);
    } else {
        qDebug() << QString("GraphDataController::applyPreset(): Unsupported API version, exiting");
    }
}

void GraphDataController::applyPresetV050500(const QDomElement &rootEl, const QString &dir)
{
    // there might be scans present already. We need to gather the names in order
    // to access the scans by their names
    QHash<QString, Scan *> presentScans;

    for (int i = 0; i < count(); ++i) {
        presentScans[at(i)->name(true)] = getScan(i);
        qDebug() << QString("GraphDataController::applyPresetV050500(): Scan %1 already present").arg(at(i)->name(true));
    }

    QDomNodeList scans = rootEl.elementsByTagName("scan");

    for (int i = 0; i < scans.size(); ++i) {
        QDomElement el = scans.at(i).toElement();
        if (el.isNull()) {
            qDebug() << QString("GraphDataController::applyPresetV050500(): Element %1 is null, continueing").arg(i);
            continue;
        }

        QString elName = el.attribute("name", QString());
        QString elSource = el.attribute("sourceFile", QString());
        QColor elColor(el.attribute("color", "#000000"));
        double elScale = el.attribute("scaleFactor", "1.0").toDouble();
        double elhOff = el.attribute("hOffset", "0.0").toDouble();
        double elvOff = el.attribute("vOffset", "0.0").toDouble();

        if (presentScans.contains(elName)) {
            // The scan was already loaded from a project file (e.g. *.dia),
            // just set the offsets.
            Scan *sc = presentScans[elName];
            sc->setXoffset(elhOff);
            sc->setYoffset(elvOff);
            sc->setScaleFactor(elScale);
        } else if (!elSource.isEmpty()) {
            // the scan should be loaded from a file, e.g. using "insert scan".
            // load it again and apply the offsets.

            QFileInfo fi(dir + "/" + elSource);

            if (!fi.exists()) {
                qDebug() << QString("GraphDataController::applyPresetV050500(): File %1 doesn't exist. Skipping.").arg(fi.absoluteFilePath());
                continue;
            }

            ImportHandler iHandler;
            QString uid = iHandler.uidByFileName(fi.absoluteFilePath());

            if (addScanFile(fi.absoluteFilePath(), uid, true, i == scans.size() - 1) <= 0) {
                qDebug() << QString("GraphDataController::applyPresetV050500(): Could not read file %1. Skipping.").arg(fi.absoluteFilePath());
                continue;
            }

            Scan *sc = getLast();
            sc->setXoffset(elhOff);
            sc->setYoffset(elvOff);
            sc->setScaleFactor(elScale);
        } else {
            // the scan does not originate from a file, but for instance from a curve fit.
            // read the entire scan from the xml file.
            Scan sc(elName, elColor);
            sc.setSourceFileName(elSource);
            sc.setXoffset(elhOff);
            sc.setYoffset(elvOff);
            sc.setScaleFactor(elScale);

            QDomElement scElXaxis = el.firstChildElement("xAxis");
            QDomElement scElYvalues = el.firstChildElement("yValues");

            if (scElXaxis.isNull() || scElYvalues.isNull()) {
                qDebug() << QString("GraphDataController::applyPresetV050500(): Elements xAxis and/or yValues are null, skipping");
                continue;
            }

            double scStart = scElXaxis.attribute("start", "-1.0").toDouble();
            double scEnd   = scElXaxis.attribute("end", "-1.0").toDouble();
            int    scCount = scElXaxis.attribute("count", "-1").toInt();
            QStringList yVals = scElYvalues.text().split(" ");

            if ((scStart < 0.0) || (scEnd < 0.0) || (scCount <= 0)) continue;
            if (yVals.size() != scCount) continue;

            sc.pDataAngle().resize(scCount);
            sc.pDataIntensity().resize(scCount);

            for (int j = 0; j < scCount; ++j) {
                sc.pDataAngle()[j] = scStart + double(j) * (scEnd - scStart) / double(scCount - 1);
                sc.pDataIntensity()[j] = yVals.at(j).toDouble();
            }

            appendScan(sc, i == scans.size() - 1);
        }
    }
}

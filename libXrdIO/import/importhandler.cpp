/***************************************************************************
                          importhandler.cpp  -  description
                             -------------------
    begin                : Thu June 18, 2013
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

#include "importhandler.h"
#include "fullprofdat10import.h"
#include "bgmndiaimport.h"
#include "texplusovlimport.h"
#include "reynoldspltimport.h"
#include "fullprofprfimport.h"
#include "brukerrawimport.h"
#include "brukerbrmlimport.h"
#include "philipsrdimport.h"
#include "philipsudfimport.h"
#include "seifertvalimport.h"
#include "panalyticalxrdmlimport.h"
#include "asciixyimport.h"
#include "stoeproimport.h"
#include "stoerawimport.h"
#include "rigakudifimport.h"
#include "rigakudatimport.h"
#include "rigakurawimport.h"
#include "rigakubinimport.h"
#include "rigakurasimport.h"
#include "rigakurasximport.h"
#include "rigakuxmlimport.h"
#include "jadexmlimport.h"
#include "jademdiimport.h"
#include "fullprofsubimport.h"
#include "pdcifimport.h"
#include "gsasstdimport.h"
#include "thermorawimport.h"
#include "thermoniimport.h"
#include "thermotxlimport.h"
#include "gnresgimport.h"
#include "chiimport.h"
#include "xyeimport.h"
#include "pyfaidatimport.h"
#include "../parser/bgmnparparser.h"
#include "../export/exporthandler.h"
#include <QTime>

ImportHandler::ImportHandler()
{
    settings = SettingsManager::getInstance();

    BgmnDiaImport *bgmnDiaImport = new BgmnDiaImport();
    formats.insert(bgmnDiaImport->uniqueId(), bgmnDiaImport);
    projectFilterList.append(bgmnDiaImport->uniqueId());
    headerSize[bgmnDiaImport->uniqueId()] = 64;

    FullprofPrfImport *fpPrfImport = new FullprofPrfImport();
    formats.insert(fpPrfImport->uniqueId(), fpPrfImport);
    projectFilterList.append(fpPrfImport->uniqueId());
    headerSize[fpPrfImport->uniqueId()] = 512;

    FullprofSubImport *fpSubImport = new FullprofSubImport();
    formats.insert(fpSubImport->uniqueId(), fpSubImport);
    rawFilterList.append(fpSubImport->uniqueId());
    headerSize[fpSubImport->uniqueId()] = 512;

    PanalyticalXrdmlImport *panXrdmlImport = new PanalyticalXrdmlImport();
    formats.insert(panXrdmlImport->uniqueId(), panXrdmlImport);
    rawFilterList.append(panXrdmlImport->uniqueId());
    headerSize[panXrdmlImport->uniqueId()] = 512;

    PhilipsRdImport *philRdImport = new PhilipsRdImport();
    formats.insert(philRdImport->uniqueId(), philRdImport);
    rawFilterList.append(philRdImport->uniqueId());
    headerSize[philRdImport->uniqueId()] = 32;

    PhilipsUdfImport *philUdfImport = new PhilipsUdfImport();
    formats.insert(philUdfImport->uniqueId(), philUdfImport);
    rawFilterList.append(philUdfImport->uniqueId());
    headerSize[philUdfImport->uniqueId()] = 512;

    BrukerRawImport *bruRawImport = new BrukerRawImport();
    formats.insert(bruRawImport->uniqueId(), bruRawImport);
    rawFilterList.append(bruRawImport->uniqueId());
    headerSize[bruRawImport->uniqueId()] = 4;

    BrukerBrmlImport *bruBrmlImport = new BrukerBrmlImport();
    formats.insert(bruBrmlImport->uniqueId(), bruBrmlImport);
    rawFilterList.append(bruBrmlImport->uniqueId());
    headerSize[bruBrmlImport->uniqueId()] = 128;

    SeifertValImport *seifValImport = new SeifertValImport();
    formats.insert(seifValImport->uniqueId(), seifValImport);
    rawFilterList.append(seifValImport->uniqueId());
    headerSize[seifValImport->uniqueId()] = 8;

    StoeProImport *stoeProImport = new StoeProImport();
    formats.insert(stoeProImport->uniqueId(), stoeProImport);
    rawFilterList.append(stoeProImport->uniqueId());
    headerSize[stoeProImport->uniqueId()] = 512;

    StoeRawImport *stoeRawImport = new StoeRawImport();
    formats.insert(stoeRawImport->uniqueId(), stoeRawImport);
    rawFilterList.append(stoeRawImport->uniqueId());
    headerSize[stoeRawImport->uniqueId()] = 32;

    RigakuDifImport *rigDifImport = new RigakuDifImport();
    formats.insert(rigDifImport->uniqueId(), rigDifImport);
    rawFilterList.append(rigDifImport->uniqueId());
    headerSize[rigDifImport->uniqueId()] = 512;

    RigakuDatImport *rigDatImport = new RigakuDatImport();
    formats.insert(rigDatImport->uniqueId(), rigDatImport);
    rawFilterList.append(rigDatImport->uniqueId());
    headerSize[rigDatImport->uniqueId()] = 8;

    RigakuRawImport *rigRawImport = new RigakuRawImport();
    formats.insert(rigRawImport->uniqueId(), rigRawImport);
    rawFilterList.append(rigRawImport->uniqueId());
    headerSize[rigRawImport->uniqueId()] = 4;

    RigakuBinImport *rigBinImport = new RigakuBinImport();
    formats.insert(rigBinImport->uniqueId(), rigBinImport);
    rawFilterList.append(rigBinImport->uniqueId());
    headerSize[rigBinImport->uniqueId()] = 32;

    RigakuRasImport *rigRasImport = new RigakuRasImport();
    formats.insert(rigRasImport->uniqueId(), rigRasImport);
    rawFilterList.append(rigRasImport->uniqueId());
    headerSize[rigRasImport->uniqueId()] = 32;

    RigakuRasxImport *rigRasxImport = new RigakuRasxImport();
    formats.insert(rigRasxImport->uniqueId(), rigRasxImport);
    rawFilterList.append(rigRasxImport->uniqueId());
    headerSize[rigRasxImport->uniqueId()] = 32;

    RigakuXmlImport *rigXmlImport = new RigakuXmlImport();
    formats.insert(rigXmlImport->uniqueId(), rigXmlImport);
    rawFilterList.append(rigXmlImport->uniqueId());
    headerSize[rigXmlImport->uniqueId()] = 128;

    JadeMdiImport *jadeMdiImport = new JadeMdiImport();
    formats.insert(jadeMdiImport->uniqueId(), jadeMdiImport);
    rawFilterList.append(jadeMdiImport->uniqueId());
    headerSize[jadeMdiImport->uniqueId()] = 32;

    JadeXmlImport *jadeXmlImport = new JadeXmlImport();
    formats.insert(jadeXmlImport->uniqueId(), jadeXmlImport);
    rawFilterList.append(jadeXmlImport->uniqueId());
    headerSize[jadeXmlImport->uniqueId()] = 128;

    ThermoRawImport *thermoRawImport = new ThermoRawImport();
    formats.insert(thermoRawImport->uniqueId(), thermoRawImport);
    rawFilterList.append(thermoRawImport->uniqueId());
    headerSize[thermoRawImport->uniqueId()] = 8;

    ThermoTxlImport *thermoTxlImport = new ThermoTxlImport();
    formats.insert(thermoTxlImport->uniqueId(), thermoTxlImport);
    rawFilterList.append(thermoTxlImport->uniqueId());
    headerSize[thermoTxlImport->uniqueId()] = 128;

    /*
    ThermoNiImport *thermoNiImport = new ThermoNiImport();
    formats.insert(thermoNiImport->uniqueId(), thermoNiImport);
    rawFilterList.append(thermoNiImport->uniqueId());
    headerSize[thermoNiImport->uniqueId()] = 8;
    */

    GnrEsgImport  *gnrEsgImport = new GnrEsgImport();
    formats.insert(gnrEsgImport->uniqueId(), gnrEsgImport);
    rawFilterList.append(gnrEsgImport->uniqueId());
    headerSize[gnrEsgImport->uniqueId()] = 32;

    FullprofDat10Import *fpDat10Import = new FullprofDat10Import();
    formats.insert(fpDat10Import->uniqueId(), fpDat10Import);
    rawFilterList.append(fpDat10Import->uniqueId());
    headerSize[fpDat10Import->uniqueId()] = 512;

    PdCifImport *pdCifImport = new PdCifImport();
    formats.insert(pdCifImport->uniqueId(), pdCifImport);
    rawFilterList.append(pdCifImport->uniqueId());
    headerSize[pdCifImport->uniqueId()] = 512;

    TexplusOvlImport *tpOvlImport = new TexplusOvlImport();
    formats.insert(tpOvlImport->uniqueId(), tpOvlImport);
    rawFilterList.append(tpOvlImport->uniqueId());
    headerSize[tpOvlImport->uniqueId()] = 512;

    ReynoldsPltImport *reyPltImport = new ReynoldsPltImport();
    formats.insert(reyPltImport->uniqueId(), reyPltImport);
    rawFilterList.append(reyPltImport->uniqueId());
    headerSize[reyPltImport->uniqueId()] = 512;

    AsciiXyImport *asciiXyImport = new AsciiXyImport();
    formats.insert(asciiXyImport->uniqueId(), asciiXyImport);
    rawFilterList.append(asciiXyImport->uniqueId());
    headerSize[asciiXyImport->uniqueId()] = 512;

    XyeImport *xyeImport = new XyeImport();
    formats.insert(xyeImport->uniqueId(), xyeImport);
    rawFilterList.append(xyeImport->uniqueId());
    headerSize[xyeImport->uniqueId()] = 512;

    GsasStdImport *gsasStdImport = new GsasStdImport();
    formats.insert(gsasStdImport->uniqueId(), gsasStdImport);
    rawFilterList.append(gsasStdImport->uniqueId());
    headerSize[gsasStdImport->uniqueId()] = 10240;

    ChiImport *chiQImport = new ChiImport();
    formats.insert(chiQImport->uniqueId(), chiQImport);
    rawFilterList.append(chiQImport->uniqueId());
    headerSize[chiQImport->uniqueId()] = 128;

    PyFaiDatImport *pyFaiImport = new PyFaiDatImport();
    formats.insert(pyFaiImport->uniqueId(), pyFaiImport);
    rawFilterList.append(pyFaiImport->uniqueId());
    headerSize[pyFaiImport->uniqueId()] = 32;

    currentUid = QString();

    // store formats that contain multiple scans which must not be averaged or summed
    nonMergeUids << bgmnDiaImport->uniqueId();
    nonMergeUids << fpPrfImport->uniqueId();
}

ImportHandler::~ImportHandler()
{
    while (formats.size()) {
        GenericImport *g = formats.take(formats.firstKey());
        if (g) delete g;
    }
}

QStringList ImportHandler::filters()
{
    QStringList out;

    QMap<QString, GenericImport *>::const_iterator it = formats.constBegin();
    while (it != formats.constEnd()) {
        out.append(it.value()->filter());
        ++it;
    }

    return out;
}

QStringList ImportHandler::rawFilters()
{
    QStringList out("All files (*.*)");

    QMap<QString, GenericImport *>::const_iterator it = formats.constBegin();

    while (it != formats.constEnd()) {
        if (rawFilterList.contains(it.value()->uniqueId())) {
            out.append(it.value()->filter());
        }
        ++it;
    }

    return out;
}

QStringList ImportHandler::projectFilters()
{
    QStringList out;

    QMap<QString, GenericImport *>::const_iterator it = formats.constBegin();
    while (it != formats.constEnd()) {
        if (projectFilterList.contains(it.value()->uniqueId())) {
            out.append(it.value()->filter());
        }
        ++it;
    }

    return out;
}

QStringList ImportHandler::extensions()
{
    QStringList out;

    QMap<QString, GenericImport *>::const_iterator it = formats.constBegin();
    while (it != formats.constEnd()) {
        out.append(it.value()->extensions());
        ++it;
    }

    return out;
}

QStringList ImportHandler::descriptions()
{
    QStringList out;

    QMap<QString, GenericImport *>::const_iterator it = formats.constBegin();
    while (it != formats.constEnd()) {
        out.append(it.value()->description());
        ++it;
    }

    return out;
}

QStringList ImportHandler::uniqueIds()
{
    return formats.keys();
}

int ImportHandler::load(const QString &f, const QString &uid, QVector<Scan> &scanHeap, bool minimal)
{
    // QTime timer;
    // timer.start();

    file = f;

    // check if we find an importer that reports success on parsing the header
    GenericImport * importer = isSupported(file, uid);
    // int t1 = timer.elapsed();

    if (!importer) {
        qDebug() << QString("ImportHandler::load(): No importer found for file %1").arg(f);
        currentUid = QString();
        return -1;
    }

    currentUid = importer->uniqueId();
    importer->setDefaultWl(settings->defaultWavelength());
    int i = importer->load(file, scanHeap, minimal);

    // merge multi-range files if requested
    if ((i > 1) && !(nonMergeUids.contains(currentUid))){
        int mode = settings->value("scans/multiScanMode", 0).toInt();
        if (mode == 1) sumScans(scanHeap);
        if (mode == 2) averageScans(scanHeap);
    }

    // int t2 = timer.elapsed() - t1;
    // qDebug() << QString("ImportHandler::load(): File scanned in %1 ms.").arg(t1);
    // qDebug() << QString("ImportHandler::load(): File loaded in %1 ms.").arg(t2);
//    if (!minimal) {
//        qDebug() << QString("ImportHandler::load(): File %1 loaded in %2 ms.").arg(f).arg(timer.elapsed());
//    }

    return scanHeap.size() ? scanHeap.size() : -1;
}

/*
 * returns a pointer to the importer with uniqueID = uid if it supports file f,
 * or else returns 0
 */
GenericImport * ImportHandler::isSupported(const QString &f, const QString &uid)
{
    if (!formats.contains(uid)) {
        return 0;
    }

    if (formats.value(uid)->isSupported(headerOfFile(f, uid))) {
        return static_cast<GenericImport *>(formats.value(uid));
    }

    return 0;
}

int ImportHandler::saveAs(const QString &in, const QString &uid_in, const QString &out, const QString &uid_out, int snumber, const QString &fsep)
{
    QMap<QString, QVariant> flags;
    flags["fieldSeparator"] = QVariant(fsep);
    flags["fixBgmnZero"]    = QVariant(false);

    // temporary scan heap
    QVector<Scan> *scanHeap = new QVector<Scan>;

    QString uIn = uid_in;

    if (uIn.isEmpty()) {
        uIn = uidByFileName(in);
    }

    int i = load(in, uIn, *scanHeap);

    if (i < 1) {
        delete scanHeap;
        return i;
    }

    ExportHandler eHandler;

    // snumber = -1 means that all scans contained in scanHeap are exported
    // for snumber >= 0 only scan number snumber is exported
    if (snumber < 0) {
        i = eHandler.save(uid_out, out, *scanHeap, flags);
        // qDebug() << QString("ImportHandler::saveAs(): Exported %1 scans").arg(i);
    } else {
        if ((int)scanHeap->size() <= snumber) {
            qDebug() << QString("ImportHandler::saveAs(): Attempting to export scan number %1, but only %2 scans are available").arg(snumber + 1).arg(scanHeap->size());
            i = -1;
        } else {
            Scan scan = scanHeap->at(snumber);
            i = eHandler.save(uid_out, out, scan, flags);
            // qDebug() << QString("ImportHandler::saveAs(): Exported scan number %1").arg(snumber);
        }
    }

    delete scanHeap;
    return i;
}

void ImportHandler::getExclRegs(QVector<double> &v)
{
    getExclRegs(file, v);
}

void ImportHandler::getExclRegs(const QString &f, QVector<double> &v)
{
    QFileInfo fi(f);

    // this is only available for fullprof PRF files
    if (fi.suffix().toLower() != "prf") {
        return;
    }

    QMap<QString, GenericImport *>::const_iterator it = formats.constBegin();
    while (it != formats.constEnd()) {
        if (it.value()->extensions().contains(fi.suffix().toLower())) {
            FullprofPrfImport *pimp = static_cast<FullprofPrfImport*>(it.value());
            // qDebug() << QString("ImportHandler::getExclRegs(): Reading excluded regions from importer %1").arg(pimp->description());
            pimp->getExclRegs(v);
            return;
        }

        ++it;
    }
}

void ImportHandler::getReflections(QVector<Scan> &scanHeap)
{
    getReflections(file, scanHeap);
}

void ImportHandler::getReflections(const QString &f, QVector<Scan> &scanHeap)
{
    QFileInfo fi(f);
    QString ext = fi.suffix().toLower();
    // QTime timer;
    // timer.start();

    // extract hkl reflections from bgmn DIA files
    if (ext == "dia") {
        if (scanHeap.size() < 5) {
            qDebug() << QString("ImportHandler::getReflections(): No sub-phases in graphHeap, nothing to do");
            return;
        }

        QString fn = QDir::fromNativeSeparators(QString("%1/%2.par").arg(fi.absolutePath()).arg(fi.completeBaseName()));
        BgmnParParser pp;

        if (pp.load(fn)) {
            for (int i = 4; i < (int)scanHeap.size(); ++i) {
                QString pname = scanHeap[i].name();
                scanHeap[i].pDataHkl() = pp.reflections()->value(pname);
                scanHeap[i].setHklXunit("dnm");
            }
        } else {
            qDebug() << QString("ImportHandler::getReflection(): Reading file was not possible %1").arg(fn);
        }

        // qDebug() << QString("ImportHandler::getReflections(): Needed %1 ms to parse hkl reflections").arg(timer.elapsed());
        return;
    }
}

int ImportHandler::getNumberOfPhases()
{
    QFileInfo fi(file);

    // this is only available for fullprof PRF files
    if (fi.suffix().toLower() != "prf") {
        return 0;
    }

    QMap<QString, GenericImport *>::const_iterator it = formats.constBegin();
    while (it != formats.constEnd()) {
        if (it.value()->extensions().contains(fi.suffix().toLower())) {
            FullprofPrfImport *pimp = static_cast<FullprofPrfImport*>(it.value());
            return pimp->getNumberOfPhases();
        }

        ++it;
    }

    return 0;
}

/*
 * Calculates the sum of intensities of all scans in scanHeap and replaces
 * them with the summed scan. Afterwards, scanHeap will only contain one scan
 *
 * WARNING: No check for angles is performed. It assumes that start angle, end
 *          angle, and step size of all scans are identical!
 */
void ImportHandler::sumScans(QVector<Scan> &scanHeap)
{
    // qDebug() << QString("ImportHandler::sumScans(): Merging %1 scans.").arg(scanHeap.size());

    if (!scanHeap.size()) {
        qDebug() << QString("ImportHandler::sumScans(): Empty scan list, nothing to do.");
        return;
    }

    // locate the largest scan in scanHeap
    int vecSize = 0;
    int pos = 0;
    for (int i = 0; i < scanHeap.size(); ++i) {
        if (scanHeap[i].pDataIntensity().size() > vecSize) {
            pos = i;
            vecSize = scanHeap[i].pDataIntensity().size();
        }
    }

    // create a copy of the largest scan in scanHeap, so it contains the first set of intensities and
    // all auxilary information. Then remove the largest scan from scanHeap to avoid processing it twice.
    Scan sScan = scanHeap.at(pos);
    scanHeap.erase(scanHeap.begin()+pos);

    // loop over all other scans and add the intensities
    for (int i = 0; i < scanHeap.size(); ++i) {
        QVector<double> &v_source = scanHeap[i].pDataIntensity();
        QVector<double> &v_dest = sScan.pDataIntensity();

        // loop through the vector
        for (int j = 0; j < qMin(v_source.size(), v_dest.size()); ++j) {
            v_dest[j] += v_source[j];
        }
    }

    scanHeap.clear();
    scanHeap.push_back(sScan);
}

/*
 * Calculates the average of intensities of all scans in scanHeap and replaces
 * them with the merged scan. Afterwards, scanHeap will only contain one scan
 *
 * WARNING: No check for angles is performed. It assumes that start angle, end
 *          angle, and step size of all scans are identical!
 */
void ImportHandler::averageScans(QVector<Scan> &scanHeap)
{
    // qDebug() << QString("ImportHandler::averageScans(): Merging %1 scans.").arg(scanHeap.size());
    int n = scanHeap.size();

    // first sum all scans
    sumScans(scanHeap);

    if (!scanHeap.size()) {
        qDebug() << QString("ImportHandler::averageScans(): Empty scan list, nothing to do.");
        return;
    }

    // now divide values by the initial number of scans
    QVector<double> &v = scanHeap[0].pDataIntensity();

    for (int i = 0; i < v.size(); ++i) {
        v[i] /= (double)n;
    }
}

/*
 * returns the uniqueId of the importer whose file extension
 * matches f's suffix. If a match is found, the function
 * ::isSupported() is used to test whether the file format
 * is really supported by the importer.
 */
QString ImportHandler::uidByFileName(const QString &f)
{
    if (f.isNull()) return QString();

    QFileInfo fi(f);
    QString ext = fi.suffix().toLower();

    QMap<QString, GenericImport *>::const_iterator it = formats.constBegin();

    while (it != formats.constEnd()) {
        if (it.value()->extensions().contains(ext)) {
            // the extension matches, lets see if the file is actually supported.
            // remember: it.key() contains the uniqueId.
            if (isSupported(f, it.key())) {
                return it.key();
            }
        }

        ++it;
    }

    return QString();
}

/*
 * returns the uniqueId of the importer whose filter string
 * matches f. This is relative likely to return the correct
 * importer, because filters *should* be unique
 */
QString ImportHandler::uidByFilter(const QString &f)
{
    if (f.isNull()) return QString();

    QMap<QString, GenericImport *>::const_iterator it = formats.constBegin();

    while (it != formats.constEnd()) {
        if (it.value()->filter() == f) {
            // remember: it.key() contains the uniqueId.
            return it.key();
        }

        ++it;
    }

    return QString();
}

/*
 * returns the first few bytes of the file f
 */
QByteArray ImportHandler::headerOfFile(const QString &f, const QString &uid)
{
    QFile fl(f);
    QByteArray header;

    if (fl.open(QIODevice::ReadOnly)) {
        header = fl.read(headerSize[uid]);
        fl.close();
    }

    return header;
}

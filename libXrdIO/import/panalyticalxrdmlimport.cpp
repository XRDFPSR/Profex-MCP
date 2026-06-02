/***************************************************************************
                          panalyticalxrdmlimport.cpp  -  description
                             -------------------
    begin                : Mon Jan 20 14:16:07 CEST 2009
    copyright            : (C) 2005 by Nicola Doebelin
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



#include "panalyticalxrdmlimport.h"
#include <QtXml>

/*
 * Constructor
 */
PanalyticalXrdmlImport::PanalyticalXrdmlImport(QObject *parent) :
    GenericImport(parent)
{
    version = -1.0;
    extens << "xrdml";
    descr = QLatin1String("PANalytical XRDML scan");
}

bool PanalyticalXrdmlImport::isSupported(const QByteArray &ba)
{
    if (ba.contains("xrdMeasurements")) {
        return true;
    }

    return false;
}

int PanalyticalXrdmlImport::load(const QString &filename, QVector<Scan> &scanHeap, bool minimal)
{
    // read the text file
    qDebug() << QString("PanalyticalXrdmlImport::load(): Loading file %1").arg(filename);
    QString content(BgmnFileIO::readTextFile(filename));

    if (content.isEmpty()) {
        qDebug() << QString("PanalyticalXrdmlImport::load(): No data found in file %1").arg(filename);
        return -1;
    }

    // create a domdocument with the name of the file's basename and set the content
    QFileInfo finfo(filename);
    QDomDocument doc(finfo.completeBaseName());
    doc.setContent(content);

    // check if the file is of type "xrdMeasurements", abort if not so
    QDomElement root = doc.documentElement();
    if (root.tagName() != "xrdMeasurements") {
        qDebug() << QString("PanalyticalXrdmlImport::load(): The file is not a XRDML file %1").arg(filename);
        return -1;
    }

    QStringList xmlns = root.toElement().attribute("xmlns", "-1").split("/");
    if (xmlns.size()) version = xmlns.last().toDouble();
    qDebug() << QString("PanalyticalXrdmlImport::load(): Detected format version %1").arg(version, 0, 'f', 1);

    // read all domelements of type "xrdMeasurement"
    QDomNodeList dnl = doc.elementsByTagName("xrdMeasurement");

    // counter for number of successfully read scans
    int nScans = 0;

    // loop over all "xrdMeasurement"s
    for (int n = 0; n < dnl.count(); n++) {
        QDomNode cNode = dnl.item(n);
        QVariantHash auxInfo;

        parseUsedWavelength(cNode, auxInfo);

        if (!minimal) {
            auxInfo.insert(QStringLiteral("measurementType"), QVariant(cNode.toElement().attribute("measurementType")));
            auxInfo.insert(QStringLiteral("status"), QVariant(cNode.toElement().attribute("status")));
            auxInfo.insert(QStringLiteral("sampleMode"), QVariant(cNode.toElement().attribute("sampleMode")));

            parseIncidentBeamPath(cNode, auxInfo);
            parseDiffractedBeamPath(cNode, auxInfo);
        }

        // get a list of all elements of type "scan"
        QDomNodeList scanlist = cNode.toElement().elementsByTagName("scan");

        // loop through all scans
        for (int m = 0; m < scanlist.count(); m++) {
            // create a new scan object
            Scan scan(finfo.completeBaseName(), QColor(), 1);
            scan.setSourceFileName(filename);
            scan.setWaveLength(auxInfo.value("kAlpha1").toDouble());
            scan.setName(QString("%1 %2").arg(finfo.completeBaseName()).arg(m));

            // Read scan data. If successful, store the scan in scanHeap
            if (parseScanDataPoints(scanlist.item(m), scan, minimal)) {
                // gather more auxilary information if requested
                if (!minimal) {
                    // aux info from <scan><header>
                    parseScanHeader(scanlist.item(m).namedItem("header"), scan);

                    QVariantHash::const_iterator it = auxInfo.constBegin();
                    while (it != auxInfo.constEnd()) {
                        // dbg.append(QString("PanalyticalXrdmlImport::load(): Scan aux info: %1 = %2\n").arg(it.key()).arg(it.value().toString()));
                        scan.setAuxInfo(it.key(), it.value());
                        ++it;
                    }
                }

                scan.setTypes(Scan::XY | Scan::MEASURED);
                scanHeap.push_back(scan);
                ++nScans;
            }
        }
    }

    // qDebug() << dbg;

    return nScans;
}

/*
 * parses the <usedWavelength> node and stores ka1, ka2, kb, and ka2/ka1 in the map
 */
void PanalyticalXrdmlImport::parseUsedWavelength(const QDomNode &node, QVariantHash &auxInfo)
{
    bool ok;
    double ka1  = 1.540598;
    double ka2  = 1.544426;
    double kb   = 1.392250;
    double ka21 = 0.5;

    QDomNode cNode = node.namedItem("usedWavelength");

    if (cNode.isNull()) {
        qDebug() << QStringLiteral("PanalyticalXrdmlImport::parseUsedWavelength(): Could not read wavelength of kAlpha1. Assuming CuKa1 with 1.540598 A");
    } else {
        double d = cNode.namedItem("kAlpha1").toElement().text().toDouble(&ok);
        if (ok) ka1 = d;

        d = cNode.namedItem("kAlpha2").toElement().text().toDouble(&ok);
        if (ok) ka2 = d;

        d = cNode.namedItem("kBeta").toElement().text().toDouble(&ok);
        if (ok) kb = d;

        d = cNode.namedItem("ratioKAlpha2KAlpha1").toElement().text().toDouble(&ok);
        if (ok) ka21 = d;
    }

    auxInfo.insert(QStringLiteral("kAlpha1"), QVariant(ka1));
    auxInfo.insert(QStringLiteral("kAlpha2"), QVariant(ka2));
    auxInfo.insert(QStringLiteral("kBeta"), QVariant(kb));
    auxInfo.insert(QStringLiteral("ratioKAlpha2KAlpha1"), QVariant(ka21));
}

/*
 * parses the <incidentBeamPath> node and stores relevant parameters in the map
 */
void PanalyticalXrdmlImport::parseIncidentBeamPath(const QDomNode &node, QVariantHash &auxInfo)
{
    QDomNode cNode = node.namedItem("incidentBeamPath");

    if (cNode.isNull()) {
        qDebug() << QStringLiteral("PanalyticalXrdmlImport::parseIncidentBeamPath(): No valid DOM node, skipping node <incidentBeamPath>");
        return;
    }

    // radius
    QString radius = cNode.namedItem("radius").toElement().text();
    auxInfo.insert(QStringLiteral("incidentBeamPath:radius"), QVariant(radius));

    // divergence slit
    parseDivergenceSlit(cNode.namedItem("divergenceSlit"), auxInfo, "incidentBeamPath");

    // soller slit
    parseSollerSlit(cNode.namedItem("sollerSlit"), auxInfo, "incidentBeamPath");

    // mask
    parseMask(cNode.namedItem("mask"), auxInfo, "incidentBeamPath");

    // xRayMirror
    parseXrayMirror(cNode.namedItem("xRayMirror"), auxInfo, "incidentBeamPath");

    // monochromator
    parseMonochromator(cNode.namedItem("monochromator"), auxInfo, "incidentBeamPath");

    // antiscatter slit
    parseAntiScatterSlit(cNode.namedItem("antiScatterSlit"), auxInfo, "incidentBeamPath");

    // filter
    parseFilter(cNode.namedItem("filter"), auxInfo, "incidentBeamPath");


    // xRayTube
    QDomNode tbNode = cNode.namedItem("xRayTube");

    if (tbNode.isElement()) {
        auxInfo.insert(QStringLiteral("incidentBeamPath:xRayTube:anodeMaterial"), QVariant(tbNode.namedItem("anodeMaterial").toElement().text()));

        QDomNode focNode = tbNode.namedItem("focus");
        if (focNode.isElement()) {
            QString focLen = focNode.namedItem("length").toElement().text();
            QString focWid = focNode.namedItem("width").toElement().text();
            QString focToa = focNode.namedItem("takeOffAngle").toElement().text();
            QString focToaU = focNode.namedItem("takeOffAngle").toElement().attribute("unit");

            auxInfo.insert(QStringLiteral("incidentBeamPath:xRayTube:focus:length"), QVariant(focLen));
            auxInfo.insert(QStringLiteral("incidentBeamPath:xRayTube:focus:width"), QVariant(focWid));
            auxInfo.insert(QStringLiteral("incidentBeamPath:xRayTube:focus:takeOffAngle"), QVariant(focToa));
            auxInfo.insert(QStringLiteral("incidentBeamPath:xRayTube:focus:takeOffAngle:unit"), QVariant(focToaU));
        }
    }

    // TODO:
    // capillaryOptics
    // beamAttenuator
}

/*
 * parses the <diffractedBeamPath> node and stores relevant parameters in the map
 */
void PanalyticalXrdmlImport::parseDiffractedBeamPath(const QDomNode &node, QVariantHash &auxInfo)
{
    QDomNode cNode = node.namedItem("diffractedBeamPath");

    if (cNode.isNull()) {
        qDebug() << QStringLiteral("PanalyticalXrdmlImport::parseDiffractedBeamPath(): No valid DOM node, skipping node <diffractedBeamPath>");
        return;
    }

    QString radius = cNode.namedItem("radius").toElement().text();
    auxInfo.insert(QStringLiteral("diffractedBeamPath:radius"), QVariant(radius));

    // antiscatter slit
    parseAntiScatterSlit(cNode.namedItem("antiScatterSlit"), auxInfo, "diffractedBeamPath");

    // soller slit
    parseSollerSlit(cNode.namedItem("sollerSlit"), auxInfo, "diffractedBeamPath");

    // monochromator
    parseMonochromator(cNode.namedItem("monochromator"), auxInfo, "diffractedBeamPath");

    // xRayMirror
    parseXrayMirror(cNode.namedItem("xRayMirror"), auxInfo, "diffractedBeamPath");

    // filter
    parseFilter(cNode.namedItem("filter"), auxInfo, "diffractedBeamPath");

    // mask
    parseMask(cNode.namedItem("mask"), auxInfo, "diffractedBeamPath");

    // receiving slit
    QDomNode rsNode = cNode.namedItem("receivingSlit");

    if (rsNode.isElement()) {
        QString rsOpen = rsNode.namedItem("height").toElement().text();
        QString rsUnit = rsNode.namedItem("height").toElement().attribute("unit");

        auxInfo.insert(QString("diffractedBeamPath:receivingSlit:height"), QVariant(rsOpen));
        auxInfo.insert(QString("diffractedBeamPath:receivingSlit:unit"), QVariant(rsUnit));
    }

    // detector
    QDomNode dtNode = cNode.namedItem("detector");

    if (dtNode.isElement()) {
        auxInfo.insert(QString("diffractedBeamPath:detector:name"), QVariant(dtNode.toElement().attribute("name")));
        auxInfo.insert(QString("diffractedBeamPath:detector:type"), QVariant(dtNode.toElement().attribute("xsi:type")));

        // mode for rtmsDetector (linear)
        auxInfo.insert(QString("diffractedBeamPath:detector:mode"), QVariant(dtNode.namedItem("mode").toElement().text()));
        auxInfo.insert(QString("diffractedBeamPath:detector:activeLength"), QVariant(dtNode.namedItem("activeLength").toElement().text()));
        auxInfo.insert(QString("diffractedBeamPath:detector:activeLength:unit"), QVariant(dtNode.namedItem("activeLength").toElement().attribute("unit")));

        // additional modes for areaDetector
        auxInfo.insert(QString("diffractedBeamPath:detector:activeChannelsEquatorial"),
                               QVariant(dtNode.namedItem("activeChannelsEquatorial").toElement().text()));
        auxInfo.insert(QString("diffractedBeamPath:detector:activeChannelsAxial"),
                               QVariant(dtNode.namedItem("activeChannelsAxial").toElement().text()));
        auxInfo.insert(QString("diffractedBeamPath:detector:pitch"),
                               QVariant(dtNode.namedItem("pitch").toElement().text()));
        auxInfo.insert(QString("diffractedBeamPath:detector:readOutPeriod"),
                               QVariant(dtNode.namedItem("readOutPeriod").toElement().text()));
    }

    // TODO:
    // parallelPlateCollimator
    // beamAttenuator
}

/*
 * parses the <scan><header> node. Info is stored as AuxInfo directly in the scan.
 * We don't use a QMap<QString, QVariant> here, because the information is scan
 * specific. The map is only used to buffer AuxInfo shared by all scans.
 */
void PanalyticalXrdmlImport::parseScanHeader(const QDomNode &node, Scan &scan)
{
    QDomNode stNode = node.namedItem("startTimeStamp");
    QDomNode etNode = node.namedItem("endTimeStamp");

    scan.setAuxInfo(QString("scan:header:startTimeStamp"), QVariant(stNode.toElement().text()));
    scan.setAuxInfo(QString("scan:header:endTimeStamp"), QVariant(etNode.toElement().text()));
}

/*
 * parses <scan><dataPoints> node.
 */
bool PanalyticalXrdmlImport::parseScanDataPoints(const QDomNode &node, Scan &scan, bool minimal)
{
    // ---------------------------
    // parsing arguments of <scan>
    // ---------------------------

    // Translate the tag <scan "scanAxis"> to the actual x-axis. This information is taken from
    // the official panalytical XRDML Parser v. 1.0 - 1.5
    QString xaxis = node.toElement().attribute("scanAxis");
    scan.setAuxInfo(QStringLiteral("scan:scanAxis"), QVariant(xaxis));

    // if xaxis contains a "-", e.g. "2Theta-Omega", we only read the part left of "-"
    xaxis = xaxis.left(xaxis.indexOf("-", 0));

    // "Gonio" will be replaced with "2Theta"
    xaxis = (xaxis == "Gonio" ? "2Theta" : xaxis);
    scan.setXAxisLabel(QString("Diffraction Angle [%1%2]").arg(global::degree).arg(xaxis));
    scan.setYAxisLabel("Intensity [counts]");

    // --------------------------
    // parsing <scan><dataPoints>
    // --------------------------

    // get the element "dataPoints"
    QDomNode dNode = node.namedItem("dataPoints");

    // not an element?
    if (!dNode.isElement()) {
        qDebug() << QStringLiteral("PanalyticalXrdmlImport::parseScanDataPoints(): No <dataPoints> tag found.");
        return false;
    }

    // -------------------------------------
    // parsing <scan><dataPoints><positions>
    // -------------------------------------

    // start angle, end angle, wavelength, maximum intensity
    double start = 0.0;
    double end = 0.0;

    // get a list of child elements "positions"
    QDomNodeList poslist = dNode.toElement().elementsByTagName("positions");

    // loop through all "positions" elements
    for (int i = 0; i < poslist.count(); i++) {
        // get an element "positions"
        QDomElement pElem = poslist.item(i).toElement();

        // not an element? ignore this position
        if (pElem.isNull()) {
            continue;
        }

        // check if the position axis == scan axis, ignore others
        if (pElem.attribute("axis") == xaxis) {
            bool ok;
            // read the item "startPosition" and check if it was successful
            start = pElem.namedItem("startPosition").toElement().text().toDouble(&ok);

            // not successful? That's a reason to return with an error code!
            if (!ok) {
                qDebug() << QStringLiteral("PanalyticalXrdmlImport::parseScanDataPoints(): Could not read the start angle of scan. Aborting.");
                return false;
            }

            // read the item "endPosition" and check if it was successful
            end = pElem.namedItem("endPosition").toElement().text().toDouble(&ok);

            // not successful? That's a reason to return with an error code!
            if (!ok) {
                qDebug() << QStringLiteral("PanalyticalXrdmlImport::parseScanDataPoints(): Could not read the end angle of scan. Aborting.");
                return false;
            }

            if (!minimal) {
                scan.setAuxInfo(QStringLiteral("scan:dataPoints:positions:startPosition"), QVariant(start));
                scan.setAuxInfo(QStringLiteral("scan:dataPoints:positions:endPosition"), QVariant(end));
            }

            // found the right <positions> block? -> no need to progress
            break;
        }
    }

    // -------------------------------------
    // parsing <scan><dataPoints><commonCountingTime>
    // -------------------------------------


    // get a list of child elements "positions"
    QDomElement ctimeElem = dNode.namedItem("commonCountingTime").toElement();
    if (!ctimeElem.isNull()) {
        bool ok;
        double ctime = ctimeElem.text().toDouble(&ok);

        if (ok) {
            scan.setTimePerStep(ctime);

            if (!minimal) {
                scan.setAuxInfo(QStringLiteral("scan:dataPoints:commonCountingTime"), QVariant(ctime));
                scan.setAuxInfo(QStringLiteral("scan:dataPoints:commonCountingTime:unit"), QVariant(ctimeElem.attribute("unit")));
            }
        }
    }

    QList<double> beamAttn = beamAttenuationFactors(dNode);
    double cBeamAttn = commonBeamAttenuationFactor(dNode);

    QList<double> divCorr = divergenceCorrections(dNode);
    double cDivCorr = commonDivergenceCorrection(dNode);


    // ---------------------------------------
    // parsing <scan><dataPoints><intensities> (version < 2.0) or ...<counts> (version >= 2.0)
    // ---------------------------------------

    // get the element "intensities"
    QString dataPointTagName = version < 2.0 ? "intensities" : "counts";
    QDomElement iElem = dNode.namedItem(dataPointTagName).toElement();

    // not an element?
    if (iElem.isNull()) {
        return false;
    }

    // store the values in a list
    QList<QByteArray> tpts = iElem.text().toLatin1().split(' ');
    QList<double> points;

    bool hasBeamAttn = tpts.size() <= beamAttn.size();
    bool hasDivCorr = tpts.size() <= divCorr.size();

    for (int i = 0; i < tpts.size(); ++i) {
        double pt = tpts.at(i).toDouble();
        double at = cBeamAttn * (hasBeamAttn ? beamAttn.at(i) : 1.0);
        double dc = cDivCorr * (hasDivCorr ? divCorr.at(i) : 1.0);

        points.append(pt * at * dc);
    }

    // --------------------------------------------
    // calculate data points and store data in scan
    // --------------------------------------------

    // check if at least 2 points were read. Otherwise a division by 0 is imminent
    if (points.size() < 2) {
        qDebug() << QStringLiteral("PanalyticalXrdmlImport::parseScanDataPoints(): Not enough data points read from scan.");
        return false;
    }

    // Get pointers to the scan's data vectors
    QVector<double> &vec_a = scan.pDataAngle();
    QVector<double> &vec_i = scan.pDataIntensity();

    vec_i.reserve(points.size());
    vec_a.reserve(points.size());

    vec_i = points.toVector();

    int n = vec_i.size();

    for (int i = 0; i < n; ++i) {
        double f = start + (double(i) / double(n)) * (end - start);
        vec_a.push_back(f);
    }

    return true;
}

void PanalyticalXrdmlImport::parseDivergenceSlit(const QDomNode &node, QVariantHash &auxInfo, const QString &prefix)
{
    if (!node.isElement()) return;

    QString length = node.namedItem("irradiatedLength").toElement().text();
    QString angle = node.namedItem("angle").toElement().text();
    QString height = node.namedItem("height").toElement().text();
    QString dist = node.namedItem("distanceToSample").toElement().text();

    QString lUnit = node.namedItem("irradiatedLength").toElement().attribute("unit");
    QString aUnit = node.namedItem("angle").toElement().attribute("unit");
    QString hUnit = node.namedItem("height").toElement().attribute("unit");
    QString dUnit = node.namedItem("distanceToSample").toElement().attribute("unit");

    auxInfo.insert(QString("%1:divergenceSlit:type").arg(prefix), QVariant(node.toElement().attribute("xsi:type")));
    auxInfo.insert(QString("%1:divergenceSlit:angle").arg(prefix), QVariant(angle));
    auxInfo.insert(QString("%1:divergenceSlit:height").arg(prefix), QVariant(height));
    auxInfo.insert(QString("%1:divergenceSlit:irradiatedLength").arg(prefix), QVariant(length));
    auxInfo.insert(QString("%1:divergenceSlit:distanceToSample").arg(prefix), QVariant(dist));

    auxInfo.insert(QString("%1:divergenceSlit:angle:unit").arg(prefix), QVariant(lUnit));
    auxInfo.insert(QString("%1:divergenceSlit:height:unit").arg(prefix), QVariant(aUnit));
    auxInfo.insert(QString("%1:divergenceSlit:irradiatedLength:unit").arg(prefix), QVariant(hUnit));
    auxInfo.insert(QString("%1:divergenceSlit:distanceToSample:unit").arg(prefix), QVariant(dUnit));
}

void PanalyticalXrdmlImport::parseSollerSlit(const QDomNode &node, QVariantHash &auxInfo, const QString &prefix)
{
    if (!node.isElement()) return;

    QString open = node.namedItem("opening").toElement().text();
    QString oUnit = node.namedItem("opening").toElement().attribute("unit");
    QString dist = node.namedItem("distanceToSample").toElement().text();
    QString dUnit = node.namedItem("distanceToSample").toElement().attribute("unit");

    auxInfo.insert(QString("%1:sollerSlit:opening").arg(prefix), QVariant(open));
    auxInfo.insert(QString("%1:sollerSlit:unit").arg(prefix), QVariant(oUnit));
    auxInfo.insert(QString("%1:sollerSlit:distanceToSample").arg(prefix), QVariant(dist));
    auxInfo.insert(QString("%1:sollerSlit:distanceToSample:unit").arg(prefix), QVariant(dUnit));
}

void PanalyticalXrdmlImport::parseMask(const QDomNode &node, QVariantHash &auxInfo, const QString &prefix)
{
    if (!node.isElement()) return;

    QString width = node.namedItem("width").toElement().text();
    QString wUnit = node.namedItem("width").toElement().attribute("unit");
    QString dist = node.namedItem("distanceToSample").toElement().text();
    QString dUnit = node.namedItem("distanceToSample").toElement().attribute("unit");

    auxInfo.insert(QString("%1:mask:width").arg(prefix), QVariant(width));
    auxInfo.insert(QString("%1:mask:unit").arg(prefix), QVariant(wUnit));
    auxInfo.insert(QString("%1:mask:distanceToSample").arg(prefix), QVariant(dist));
    auxInfo.insert(QString("%1:mask:distanceToSample:unit").arg(prefix), QVariant(dUnit));
}

void PanalyticalXrdmlImport::parseXrayMirror(const QDomNode &node, QVariantHash &auxInfo, const QString &prefix)
{
    if (!node.isElement()) return;

    QString dist = node.namedItem("distanceToSample").toElement().text();
    QString dUnit = node.namedItem("distanceToSample").toElement().attribute("unit");

    auxInfo.insert(QString("%1:xRayMirror:distanceToSample").arg(prefix), QVariant(dist));
    auxInfo.insert(QString("%1:xRayMirror:distanceToSample:unit").arg(prefix), QVariant(dUnit));

    QDomNode mirTNode = node.namedItem("crystal");
    QDomNode mirANode = node.namedItem("acceptanceAngle");
    QDomNode mirLNode = node.namedItem("length");

    if (mirTNode.isElement()) {
        QString mirTt = mirTNode.toElement().attribute("type");
        QString mirTs = mirTNode.toElement().attribute("shape");
        auxInfo.insert(QString("%1:xRayMirror:crystal:type").arg(prefix), QVariant(mirTt));
        auxInfo.insert(QString("%1:xRayMirror:crystal:shape").arg(prefix), QVariant(mirTs));
    }

    if (mirANode.isElement()) {
        QString  mirAv = mirANode.toElement().text();
        QString mirAu = mirANode.toElement().attribute("unit");
        auxInfo.insert(QString("%1:xRayMirror:acceptanceAngle").arg(prefix), QVariant(mirAv));
        auxInfo.insert(QString("%1:xRayMirror:acceptanceAngle:unit").arg(prefix), QVariant(mirAu));
    }

    if (mirLNode.isElement()) {
        QString  mirLv = mirLNode.toElement().text();
        QString mirLu = mirLNode.toElement().attribute("unit");
        auxInfo.insert(QString("%1:xRayMirror:length").arg(prefix), QVariant(mirLv));
        auxInfo.insert(QString("%1:xRayMirror:length:unit").arg(prefix), QVariant(mirLu));
    }
}

void PanalyticalXrdmlImport::parseMonochromator(const QDomNode &node, QVariantHash &auxInfo, const QString &prefix)
{
    if (!node.isElement()) return;

    QString dist = node.namedItem("distanceToSample").toElement().text();
    QString dUnit = node.namedItem("distanceToSample").toElement().attribute("unit");

    auxInfo.insert(QString("%1:monochromator:distanceToSample").arg(prefix), QVariant(dist));
    auxInfo.insert(QString("%1:monochromator:distanceToSample:unit").arg(prefix), QVariant(dUnit));

    QDomNode monCNode = node.namedItem("crystal");

    if (monCNode.isElement()) {
        QString monCt = monCNode.toElement().attribute("type");
        QString monCs = monCNode.toElement().attribute("shape");
        auxInfo.insert(QString("%1:monochromator:crystal:type").arg(prefix), QVariant(monCt));
        auxInfo.insert(QString("%1:monochromator:crystal:shape").arg(prefix), QVariant(monCs));
    }

    QDomNode monRNode = node.namedItem("numberOfReflections");

    if (monRNode.isElement()) {
        auxInfo.insert(QString("%1:monochromator:numberOfReflections").arg(prefix), QVariant(monRNode.toElement().text()));
    }

    QDomNode monHklNode = node.namedItem("hkl");

    if (monHklNode.isElement()) {
        QDomNode monHklH = monHklNode.namedItem("h");
        QDomNode monHklK = monHklNode.namedItem("k");
        QDomNode monHklL = monHklNode.namedItem("l");

        if (monHklH.isElement()) {
            auxInfo.insert(QString("%1:monochromator:hkl:h").arg(prefix), QVariant(monHklH.toElement().text()));
        }

        if (monHklK.isElement()) {
            auxInfo.insert(QString("%1:monochromator:hkl:k").arg(prefix), QVariant(monHklK.toElement().text()));
        }

        if (monHklL.isElement()) {
            auxInfo.insert(QString("%1:monochromator:hkl:l").arg(prefix), QVariant(monHklL.toElement().text()));
        }
    }
}

void PanalyticalXrdmlImport::parseAntiScatterSlit(const QDomNode &node, QVariantHash &auxInfo, const QString &prefix)
{
    if (!node.isElement()) return;

    QString dist = node.namedItem("distanceToSample").toElement().text();
    QString dUnit = node.namedItem("distanceToSample").toElement().attribute("unit");

    auxInfo.insert(QString("%1:antiScatterSlit:distanceToSample").arg(prefix), QVariant(dist));
    auxInfo.insert(QString("%1:antiScatterSlit:distanceToSample:unit").arg(prefix), QVariant(dUnit));

    QString length = node.namedItem("observedLength").toElement().text();
    QString angle = node.namedItem("angle").toElement().text();

    auxInfo.insert(QString("%1:antiScatterSlit:type").arg(prefix), QVariant(node.toElement().attribute("xsi:type")));
    auxInfo.insert(QString("%1:antiScatterSlit:observedLength").arg(prefix), QVariant(length));
    auxInfo.insert(QString("%1:antiScatterSlit:angle").arg(prefix), QVariant(angle));
}

void PanalyticalXrdmlImport::parseFilter(const QDomNode &node, QVariantHash &auxInfo, const QString &prefix)
{
    if (!node.isElement()) return;

    QString dist = node.namedItem("distanceToSample").toElement().text();
    QString dUnit = node.namedItem("distanceToSample").toElement().attribute("unit");

    auxInfo.insert(QString("%1:filter:distanceToSample").arg(prefix), QVariant(dist));
    auxInfo.insert(QString("%1:filter:distanceToSample:unit").arg(prefix), QVariant(dUnit));

    QDomNode fiMatNode = node.namedItem("material");
    QDomNode fiThiNode = node.namedItem("thickness");

    if (fiMatNode.isElement()) {
        auxInfo.insert(QString("%1:filter:material").arg(prefix), QVariant(fiMatNode.toElement().text()));
    }

    if (fiThiNode.isElement()) {
        auxInfo.insert(QString("%1:filter:thickness").arg(prefix), QVariant(fiThiNode.toElement().text()));
        auxInfo.insert(QString("%1:filter:thickness:unit").arg(prefix), QVariant(fiThiNode.toElement().attribute("unit")));
    }
}

/* <scan><dataPoints><commonBeamAttenuationFactor> (version >= 2.0) */
double PanalyticalXrdmlImport::commonBeamAttenuationFactor(const QDomNode &node)
{
    if (version < 2.0) return 1.0;

    QDomElement icBaElem = node.namedItem("commonBeamAttenuationFactor").toElement();

    if (icBaElem.isNull()) return 1.0;
    return icBaElem.text().toDouble();
}

/* <scan><dataPoints><beamAttenuationFactors> (version >= 2.0) */
QList<double> PanalyticalXrdmlImport::beamAttenuationFactors(const QDomNode &node)
{
    QList<double> beamAttn;

    if (version < 2.0) return beamAttn;

    QDomElement iBaElem = node.namedItem("beamAttenuationFactors").toElement();

    if (!iBaElem.isNull()) {
        QList<QByteArray> tdc = iBaElem.text().toLatin1().split(' ');

        for (int i = 0; i < tdc.size(); ++i) {
            beamAttn.append(tdc.at(i).toDouble());
        }
    }

    return beamAttn;
}

/* <scan><dataPoints><commonDivergenceCorrection> (version >= 2.0) */
double PanalyticalXrdmlImport::commonDivergenceCorrection(const QDomNode &node)
{
    if (version < 2.0) return 1.0;
    QDomElement icDcElem = node.namedItem("commonDivergenceCorrection").toElement();

    if (icDcElem.isNull()) return 1.0;
    return icDcElem.text().toDouble();
}

/* <scan><dataPoints><divergenceCorrections> (version >= 2.0) */
QList<double> PanalyticalXrdmlImport::divergenceCorrections(const QDomNode &node)
{
    QList<double> divCorr;

    if (version < 2.0) return divCorr;

    QDomElement iDcElem = node.namedItem("divergenceCorrections").toElement();

    if (!iDcElem.isNull()) {
        QList<QByteArray> tdc = iDcElem.text().toLatin1().split(' ');

        for (int i = 0; i < tdc.size(); ++i) {
            divCorr.append(tdc.at(i).toDouble());
        }
    }

    return divCorr;
}


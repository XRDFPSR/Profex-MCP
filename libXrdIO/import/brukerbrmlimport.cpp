/***************************************************************************
                          brukerbrmlimport.cpp  -  description
                             -------------------
    begin                : Sun Aug 25 19:00:00 CEST 2013
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



#include "brukerbrmlimport.h"
#include <QtXml>
#include <QDomNamedNodeMap>
#include <QList>
#include <QBuffer>
#include "../../quazip/quazip.h"
#include "../../quazip/quazipfile.h"
#include "../../quazip/quaziodevice.h"
#include "../../zlib/zlib.h"

BrukerBrmlImport::BrukerBrmlImport(QObject *parent)
    : GenericImport(parent)
{
    extens << "brml";
    descr = QLatin1String("Bruker BRML archive");
    verbose = false;
}

bool BrukerBrmlImport::isSupported(const QByteArray &ba)
{
    // magic number for zip archives
    if (ba.left(4).toHex() == "504b0304") return true;

    // old format in a single xml file
    if (ba.left(128).contains("ExperimentCollection")) return true;

    // neither of the two above
    return false;
}

int BrukerBrmlImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    QFile f(file);
    f.open(QIODevice::ReadOnly);
    QDataStream in(&f);
    QByteArray buffer;
    buffer.resize(128);

    in.readRawData(buffer.data(), 128);

    // if it is a zip archive, use the corresponding function
    if (buffer.left(4).toHex() == "504b0304") return loadCompressedArchive(file, scanHeap, minimal);

    // if it is a single xml file, use the corresponding function
    if (buffer.left(128).contains("ExperimentCollection")) return loadSingleXmlFile(file, scanHeap, minimal);

    return 0;
}

int BrukerBrmlImport::loadCompressedArchive( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    int n = 0;
    qDebug() << QString("BrukerBrmlImport::loadCompressedArchive(): Loading file %1").arg(file);
    QuaZip zipFile(file);

    if (!zipFile.open(QuaZip::mdUnzip)) {
        qDebug() << QString("BrukerBrmlImport::loadCompressedArchive(): Could not uncompress file %1").arg(file);
        zipFile.close();
        return n;
    }

    QStringList zFiles = zipFile.getFileNameList();

    double wla1 = 0.0;
    double wla2 = 0.0;
    double wlb = 0.0;

    // Parse RawData(n).xml files
    for (int i = 0; i < zFiles.size(); ++i) {
        if (!zFiles.at(i).contains("RawData", Qt::CaseSensitive)) {
            continue;
        }

        if (!zipFile.setCurrentFile(zFiles.at(i))) {
            qDebug() << QString("BrukerBrmlImport::loadCompressedArchive(): Could not open file %1 in archive %2. Skipping it.").arg(zFiles.at(i)).arg(file);
            continue;
        }

        // create a domdocument with the name of the file's basename
        QFileInfo finfo(zFiles.at(i));
        QDomDocument doc(finfo.completeBaseName());

        // opening the file inside the archive and reading the content
        QuaZipFile dataFile(file, zFiles.at(i), QuaZip::csSensitive, this);
        dataFile.open(QIODevice::ReadOnly);
        doc.setContent(dataFile.readAll());
        dataFile.close();

        // check if the file is of type "RawData", skip if not so
        QDomElement root = doc.documentElement();
        if (root.tagName() != "RawData") {
            qDebug() << QString("BrukerBrmlImport::loadCompressedArchive(): The file is not a BRML RawData file %1").arg(zFiles.at(i));
            continue;
        }

        // get the wavelength, first try from the current RawData.xml file, if not successful, try MeasurementContainer.xml
        if (qFuzzyIsNull(wla1)) {
            parseRawDataWavelength(doc, wla1, wla2, wlb);
            if (qFuzzyIsNull(wla1)) {
                parseMeasurementContainerWavelength(file, wla1, wla2, wlb);
            }
        }

        /* the files can either have a data block in the root element:
         * <RawData>
         *   <ScanInformation>...
         *   <Datum>...</Datum>
         *   <Datum>...</Datum>
         * </RawData>
         *
         * or capsuled in DataRoutes (seems to be a newer format):
         *
         * <RawData>
         *   <DataRoutes>
         *     <DataRoute>
         *       <ScanInformation>...
         *       <Datum>...</Datum>
         *       <Datum>...</Datum>
         *     </DataRoute>
         *     <DataRoute>
         *       <ScanInformation>...
         *       <Datum>...</Datum>
         *       <Datum>...</Datum>
         *     </DataRoute>
         *   </DataRoutes>
         * </RawData>
         */

        QDomElement docElement = doc.documentElement();
        QDomElement dataRouteElement;

        QDomNodeList dnlsDataRoutes = doc.elementsByTagName("DataRoute");

        if (dnlsDataRoutes.size()) {
            dataRouteElement = dnlsDataRoutes.at(0).toElement();
            if (verbose) qDebug() << QString("BrukerBrmlImport::loadCompressedArchive(): Found %1 DataRoutes").arg(dnlsDataRoutes.size());

            for (int j = 0; j < dnlsDataRoutes.size(); ++j) {
                if (dnlsDataRoutes.at(j).toElement().attribute("RouteFlag") == "Final") {
                    if (verbose) qDebug() << QString("BrukerBrmlImport::loadCompressedArchive(): DataRoute %1 is flagged as final").arg(j);

                    dataRouteElement = dnlsDataRoutes.at(j).toElement();
                    break;
                }
            }
        } else {
            if (verbose) qDebug() << QString("BrukerBrmlImport::loadCompressedArchive(): Has no DataRoutes");
            dataRouteElement = docElement;
        }

        QDomNodeList dnlsinfo = dataRouteElement.elementsByTagName("ScanInformation");
        QDomNode infoNode = dnlsinfo.at(0);

        QFileInfo fi(file);
        QString label(fi.fileName()); // fallback label

        QDomNodeList dnliitem = docElement.elementsByTagName("InfoItem");

        for (int j = 0; j < dnliitem.size(); ++j) {
            if (verbose) qDebug() << QString("BrukerBrmlImport::loadCompressedArchive(): Found element %1").arg(dnliitem.at(j).toElement().attribute("Name"));
            if (dnliitem.at(j).toElement().attribute("Name") == "SampleName") {
                    label = dnliitem.at(j).toElement().attribute("Value");
            }
        }

        // read the "TimePerStepEffective" and store in "tpc"
        double tpc = -1.0;
        QDomElement tpsElement = infoNode.namedItem("TimePerStepEffective").toElement();
        if (!tpsElement.isNull()) {
            bool ok;
            tpc = tpsElement.text().toDouble(&ok);
            if (!ok) tpc = -1.0;
        }

        // create scan
        Scan scan(label, QColor(), 1);
        scan.setSourceFileName(file);

        QVector<double> vec_a;
        QVector<double> vec_i;

        // read the tag "ScanAxisInfo", only the first one
        QDomNodeList axisList = infoNode.toElement().elementsByTagName("ScanAxisInfo");
        int axisCount = axisList.size();
        int axisTtNumber = 0;
        QString axisName;

        for (int i = 0; i < axisCount; ++i) {
            if (!axisList.at(i).isElement()) {
                continue;
            }

            QDomElement axisEl = axisList.at(i).toElement();

            if (axisList.at(i).toElement().attribute("AxisId") == "TwoTheta") {
                axisTtNumber = i;
                axisName = axisList.at(i).toElement().attribute("VisibleName");
                break;
            }
        }

        scan.setXAxisLabel(QString("Diffraction Angle [%1%2]").arg(global::degree).arg(axisName));

        // prepare domelements for dataPoints
        QDomNodeList datumNodes = dataRouteElement.elementsByTagName("Datum");

        double twoTheta = 0.0;
        double counts   = 0.0;

        // loop over all datum nodes
        for (int j = 0; j < datumNodes.size(); ++j) {
            if (!datumNodes.at(j).isElement()) {
                continue;
            }

            QDomElement datum = datumNodes.at(j).toElement();
            QStringList lst = datum.text().split(",");

            // assuming format "MeasuredTimePerStep, scan number?, twotheta, (theta), Intensity".
            // axis1 is usually twotheta, for coupled twotheta/theta scans, axis 2 is theta.
            // for detector scans, axis2 is missing!
            if (lst.size() < 4) {
                continue;
            }

            // reading Axis1 and Intensity from the above assumed formatted line
            twoTheta = lst.at(2 + axisTtNumber).toDouble();
            counts   = lst.at(2 + axisCount).toDouble();

            if ((twoTheta > 0.0) && (twoTheta < 180.0) && (counts >= 0.0)) {
                vec_a.push_back(twoTheta);
                vec_i.push_back(counts);
            }
        }

        scan.setTimePerStep(tpc);
        scan.setDataAng(vec_a);
        scan.setDataInt(vec_i);
        scan.setWaveLength(wla1);
        scan.setWaveLength2(wla2);
        scan.setWaveLength3(wlb);

        if (!minimal) {
            if (verbose) qDebug() << QString("BrukerBrmlImport::loadCompressedArchive(): Parsing aux info");
            QVariantHash auxInfo;
            QDomNodeList detectors = dataRouteElement.elementsByTagName("Detectors");

            for (int j = 0; j < detectors.size(); ++j) {
                parseDetector(detectors.at(j), auxInfo);
            }

            QDomNodeList pTracks = dataRouteElement.elementsByTagName("PrimaryTracks");
            QDomNodeList sTracks = dataRouteElement.elementsByTagName("SecondaryTracks");

            if (pTracks.size()) {
                parseTrack(pTracks.at(0), auxInfo, "PrimaryTracks");
            }

            if (sTracks.size()) {
                parseTrack(sTracks.at(0), auxInfo, "SecondaryTracks");
            }

            QVariantHash::const_iterator it = auxInfo.constBegin();
            while (it != auxInfo.constEnd()) {
                if (verbose) qDebug() << QString("BrukerBrmlImport::loadCompressedArchive(): Adding %1 = %2").arg(it.key()).arg(it.value().toString());
                scan.setAuxInfo(it.key(), it.value());
                ++it;
            }
        }

        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);
        ++n;
    }

    zipFile.close();
    return n;
}

void BrukerBrmlImport::parseRawDataWavelength(const QDomDocument &d, double &ka1, double &ka2, double &kb)
{
    QDomNodeList primTracks = d.elementsByTagName("PrimaryTracks");
    if (!primTracks.size()) return;

    QDomNodeList wlKa1 = primTracks.at(0).toElement().elementsByTagName("WaveLengthAlpha1");
    QDomNodeList wlKa2 = primTracks.at(0).toElement().elementsByTagName("WaveLengthAlpha2");
    QDomNodeList wlKb  = primTracks.at(0).toElement().elementsByTagName("WaveLengthBeta");

    if (wlKa1.size()) ka1 = wlKa1.at(0).toElement().attribute("Value").toDouble();
    if (wlKa2.size()) ka2 = wlKa2.at(0).toElement().attribute("Value").toDouble();
    if (wlKb.size())  kb  = wlKb.at(0).toElement().attribute("Value").toDouble();
}

void BrukerBrmlImport::parseMeasurementContainerWavelength(const QString &f, double &ka1, double &ka2, double &kb)
{
    QuaZipFile measurementContainerFile(f, "MeasurementContainer.xml", QuaZip::csSensitive, this);
    measurementContainerFile.open(QIODevice::ReadOnly);
    QByteArray ba = measurementContainerFile.readAll();
    measurementContainerFile.close();

    QDomDocument doc("MeasurementContainer.xml");
    doc.setContent(QString(ba));

    QDomNodeList lstMountedTube = doc.elementsByTagName("MountedTube");
    if (lstMountedTube.size()) {
        ka1 = lstMountedTube.at(0).firstChildElement("WaveLengthAlpha1").attribute("Value", "0").toDouble();
        ka2 = lstMountedTube.at(0).firstChildElement("WaveLengthAlpha2").attribute("Value", "0").toDouble();
        kb  = lstMountedTube.at(0).firstChildElement("WaveLengthBeta").attribute("Value", "0").toDouble();
    }
}

void BrukerBrmlImport::parseDetector(const QDomNode &node, QVariantHash &auxInfo)
{
    if (verbose) qDebug() << QString("BrukerBrmlImport::parseDetector(): Parsing detector");
    QDomNodeList iDatas = node.toElement().elementsByTagName("InfoData");

    for (int j = 0; j < iDatas.size(); ++j) {
        QDomNamedNodeMap attr = iDatas.at(j).toElement().attributes();

        for (int i = 0; i < attr.length(); ++i) {
            QString name = attr.item(i).toAttr().name();
            QString val = attr.item(i).toAttr().value();

            if (name == "xsi:type") name = QStringLiteral("type");

            QString prefix = QString("Detector:InfoData:%1").arg(name);
            auxInfo.insert(prefix, QVariant(val));
            if (verbose) qDebug() << QString("BrukerBrmlImport::parseDetector(): %1 = %2").arg(prefix).arg(auxInfo.value(prefix).toString());
        }

        QDomNodeList cnodes = iDatas.at(j).childNodes();

        for (int k = 0; k < cnodes.size(); ++k) {
            QDomNamedNodeMap attr = cnodes.at(k).toElement().attributes();

            for (int i = 0; i < attr.length(); ++i) {
                QString name = attr.item(i).toAttr().name();
                QString val = attr.item(i).toAttr().value();

                if (name == "xsi:type") name = QStringLiteral("type");

                QString prefix = QString("Detector:InfoData:%1:%2").arg(cnodes.at(k).nodeName()).arg(name);
                auxInfo.insert(prefix, QVariant(val));
                if (verbose) qDebug() << QString("BrukerBrmlImport::parseDetector(): %1 = %2").arg(prefix).arg(auxInfo.value(prefix).toString());
            }
        }
    }
}

void BrukerBrmlImport::parseTrack(const QDomNode &node, QVariantHash &auxInfo, const QString &prefix)
{
    //if (verbose)
    qDebug() << QString("BrukerBrmlImport::parseTrack(): Parsing %1").arg(prefix);
    QDomNodeList tiDatas = node.toElement().elementsByTagName("TrackInfoData");

    if (!tiDatas.size()) {
        //if (verbose)
        qDebug() << QString("BrukerBrmlImport::parseTrack(): No tag TrackInfoData found");
        return;
    }

    QDomNode rad = tiDatas.at(0).namedItem("Radius");

    if (rad.isElement()) {
        QDomNamedNodeMap radAttr = rad.attributes();
        for (int i = 0; i < radAttr.length(); ++i) {
            auxInfo.insert(QString("%1:Radius:%2")
                           .arg(prefix)
                           .arg(radAttr.item(i).toAttr().name()),
                           radAttr.item(i).toAttr().value());
        }
    }

    QDomNode mOptics = tiDatas.at(0).namedItem("MountedOptics");
    QDomNodeList optElements = mOptics.toElement().elementsByTagName("InfoData");

    for (int i = 0; i < optElements.size(); ++i) {
        // extract all attributes of "InfoData"
        QDomNamedNodeMap oeAttr = optElements.at(i).toElement().attributes();
        QString pf = QString("%1:%2").arg(prefix).arg(oeAttr.namedItem("LogicName").toAttr().value());

        for (int j = 0; j < oeAttr.length(); ++j) {
            QString name = oeAttr.item(j).toAttr().name();
            QString val = oeAttr.item(j).toAttr().value();

            if (name == "xsi:type") name = QStringLiteral("type");

            auxInfo.insert(QString("%1:%2").arg(pf).arg(name), QVariant(val));
        }

        // now parse all child nodes
        QDomNodeList cnodes = optElements.at(i).childNodes();

        for (int k = 0; k < cnodes.size(); ++k) {
            QDomNamedNodeMap attr = cnodes.at(k).toElement().attributes();

            for (int l = 0; l < attr.length(); ++l) {
                QString name = attr.item(l).toAttr().name();
                QString val = attr.item(l).toAttr().value();

                if (name == "xsi:type") name = QStringLiteral("type");

                QString tag = QString("%1:%2:%3").arg(pf).arg(cnodes.at(k).nodeName()).arg(name);
                auxInfo.insert(tag, QVariant(val));
                if (verbose) qDebug() << QString("BrukerBrmlImport::parseTrack(): %1 = %2").arg(tag).arg(auxInfo.value(tag).toString());
            }
        }
    }
}

int BrukerBrmlImport::loadSingleXmlFile(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    QDomDocument doc("ExperimentCollection");
    QFile f(file);

    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << QString("BrukerBrmlImport::loadSingleXmlUncompressed(): Could not open file for reading %1").arg(file);
        return 0;
    }

    if (!doc.setContent(&f)) {
        qDebug() << QString("BrukerBrmlImport::loadSingleXmlUncompressed(): Could not access XML content of file %1").arg(file);
        f.close();
        return 0;
    }

    f.close();

    int nScans = 0;
    QList<double> wl;
    wl << 0.0 << 0.0 << 0.0;

    QString label = file;
    QDomNodeList lstSampleId = doc.elementsByTagName("SampleId");

    if (lstSampleId.size()) {
        if (lstSampleId.at(0).isElement()) {
            label = lstSampleId.at(0).toElement().attribute("Value", file);
        }
    }

    // the mounted tube can be stored in <MountedOptics><Tube> or <MountedTube>
    // lets try <MountedOptics><Tube> first:
    QDomNodeList lstMountedOptics = doc.elementsByTagName("MountedOptics");
    if (lstMountedOptics.size()) {
        QDomNodeList lstMountedTube = lstMountedOptics.at(0).toElement().elementsByTagName("Tube");
        if (lstMountedTube.size()) {
            wl[0] = lstMountedTube.at(0).firstChildElement("WaveLengthAlpha1").attribute("Value", "0").toDouble();
            wl[1] = lstMountedTube.at(0).firstChildElement("WaveLengthAlpha2").attribute("Value", "0").toDouble();
            wl[2] = lstMountedTube.at(0).firstChildElement("WaveLengthBeta").attribute("Value", "0").toDouble();
        }
    } else { // <MountedOptics><Tube> failed, so lets try <MountedTube>:
        QDomNodeList lstMountedTube = doc.elementsByTagName("MountedTube");
        if (lstMountedTube.size()) {
            wl[0] = lstMountedTube.at(0).firstChildElement("WaveLengthAlpha1").attribute("Value", "0").toDouble();
            wl[1] = lstMountedTube.at(0).firstChildElement("WaveLengthAlpha2").attribute("Value", "0").toDouble();
            wl[2] = lstMountedTube.at(0).firstChildElement("WaveLengthBeta").attribute("Value", "0").toDouble();
        }
    }

    QDomNodeList lstContainers = doc.elementsByTagName("Containers");

    // loop over all containers and find the one of type ...Data.DataContainer
    for (int i = 0; i < lstContainers.size(); ++i) {
        QDomElement eTypeDesc = lstContainers.at(i).firstChildElement("TypeDesc");
        QDomElement eRepr = lstContainers.at(i).firstChildElement("Representation");

        if ((eTypeDesc.text() == "BrukerAXS.Common.ExperimentV5.Experiment.Data.DataContainer") && (eRepr.text() == "Serialized")) {
            nScans += parseUncompressedContainer(lstContainers.at(i), scanHeap, label, wl, file);
        }

        if ((eTypeDesc.text() == "BrukerAXS.Common.ExperimentV5.Experiment.Data.DataContainer") && (eRepr.text() == "Base64")) {
            nScans += parseCompressedContainer(lstContainers.at(i), scanHeap, label, wl, file);
        }
    }

    return nScans;
}

int BrukerBrmlImport::parseUncompressedContainer(const QDomNode &container, QVector<Scan> &scanHeap, const QString &label, const QList<double> &wl, const QString &file)
{
    int nScans = 0;
    QList<int> subScanRanges;

    QDomNodeList lstSubScans = container.toElement().elementsByTagName("SubScanInfo");

    // find the list of subscans (it should be present even if only one scan was measured)
    // store the start step position and the number of steps in a list
    for (int j = 0; j < lstSubScans.size(); ++j) {
        subScanRanges.append(lstSubScans.at(j).toElement().attribute("StartStepNo", 0).toInt());
        subScanRanges.append(lstSubScans.at(j).toElement().attribute("Steps", 0).toInt());
    }

    if (subScanRanges.size() < 2) return nScans;

    QDomNodeList datumNodes = container.toElement().elementsByTagName("Datum");
    QDomNodeList axisInfoNodes = container.toElement().elementsByTagName("ScanAxisInfo");

    // check if the number of datum nodes is at least "last subscan start + last subscan steps"
    if (datumNodes.size() < subScanRanges.at(subScanRanges.size() - 2) + subScanRanges.at(subScanRanges.size() - 1)) return nScans;

    // determine the unit of the x-axis
    QString visName = "2theta";
    if (axisInfoNodes.size()) {
        if (axisInfoNodes.at(0).isElement()) {
            visName = axisInfoNodes.at(0).toElement().attribute("VisibleName", "2theta");
        }
    }

    int pos = subScanRanges.at(0);

    // loop over all subscan ranges
    for (int ss = 0; ss < subScanRanges.size() - 1; ss += 2) {
        // set up a new scan
        Scan scan(label, QColor(), 1);
        scan.setSourceFileName(file);
        scan.setXAxisLabel(QString("Diffraction Angle [%1%2]").arg(global::degree).arg(visName));
        QVector<double> vec_a;
        QVector<double> vec_i;

        // loop over steps
        for (int ssstp = 0; ssstp < subScanRanges.at(ss + 1); ++ssstp) {
            if (!datumNodes.at(ssstp + pos).isElement()) continue;

            QDomElement datum = datumNodes.at(ssstp + pos).toElement();
            QStringList lst = datum.text().split(",");

            // reading Axis1 and Intensity from the above assumed formatted line
            vec_a.push_back(lst.at(2).toDouble());
            vec_i.push_back(lst.last().toDouble());
        }

        // scan.setTimePerStep(tpc);
        scan.setDataAng(vec_a);
        scan.setDataInt(vec_i);
        scan.setWaveLength(wl[0]);
        scan.setWaveLength2(wl[1]);
        scan.setWaveLength3(wl[2]);

        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);

        pos += subScanRanges.at(ss + 1);
        ++nScans;
    }

    return nScans;
}

/*
 * Extracts the data container from binary format. The process works as follows:
 *
 * The BLOB object embedded in the XML file actually is a string with XML content, too.
 * It can be retrieved as follows:
 *
 * 1. read the binary Serialized Object form the XML file into a QByteArray
 * 2. decode it from Base64 to RAW using QByteArray::fromBase64()
 * 3. inflate it from gzip format using zlib, store the result in a QByteArray
 * 4. convert the QByteArray to a QString, it now contains pure uncompressed XML data
 * 5. parse the XML data using the function parseUncompressedContainer
 */
int BrukerBrmlImport::parseCompressedContainer(const QDomNode &container, QVector<Scan> &scanHeap, const QString &label, const QList<double> &wl, const QString &file)
{
    QByteArray data(container.firstChildElement("SerializedObject").text().toUtf8());
    //data.append(container.firstChildElement("SerializedObject").text());
    //data = gUncompress(QByteArray::fromBase64(data));
    data = QByteArray::fromBase64(data);
    data = gUncompress(data);

    QString xmlStr(data);
    QDomDocument doc("DataContainer");
    doc.setContent(xmlStr);

    if (doc.documentElement().nodeName() != "DataContainer") {
        qDebug() << QString("BrukerBrmlImport::parseCompressedContainer(): Binary block could not be decoded to XML.");
        return 0;
    }

    return parseUncompressedContainer(doc.documentElement(), scanHeap, label, wl, file);
}

/*
 * uncompresses a gzip compressed QByteArray
 */
QByteArray BrukerBrmlImport::gUncompress(const QByteArray &data)
{
    QByteArray result;

    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;

    char out[CHUNK_SIZE];

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    ret = inflateInit2(&strm, 15 + 32);
    if (ret != Z_OK) return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);

        switch (ret) {
            case Z_NEED_DICT: {
                ret = Z_DATA_ERROR;
            }
            case Z_DATA_ERROR: {
                case Z_MEM_ERROR: {
                    (void)inflateEnd(&strm);
                }
            }

                return QByteArray();
        }

        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    inflateEnd(&strm);

    return result;
}

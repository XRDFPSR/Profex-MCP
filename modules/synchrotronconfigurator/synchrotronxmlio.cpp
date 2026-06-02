/***************************************************************************
                          synchrotronxmlio.cpp  -  description
                             -------------------
    begin                : Wed Feb 19 18:00:00 CEST 2025
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

#include "synchrotronxmlio.h"
#include <QFile>
#include <QXmlStreamWriter>
#include <QMap>
#include <QVariant>
#include <QDebug>

SynchrotronXmlIO::SynchrotronXmlIO(const QString &f)
    : _file(f)
{}

void SynchrotronXmlIO::writeFile()
{
    // Open the file for writing.
    QFile file(_file);
    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << QString("Could not open file for writing.").arg(_file);
        return;
    }

    // Set up the XML stream writer with auto-formatting for readability.
    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);

    // Start the XML document.
    xmlWriter.writeStartDocument();

    xmlWriter.writeStartElement("ProfexSynchrotronConfiguration");
    xmlWriter.writeAttribute("model", "Chernyshov");
    xmlWriter.writeAttribute("version", "5.5");

    xmlWriter.writeStartElement("Range");
    xmlWriter.writeAttribute("unit", "degrees");
    xmlWriter.writeAttribute("start", QString::number(_parameters->getParameter(synchro::RANGE_MEASURED_START, -1.0)));
    xmlWriter.writeAttribute("end",   QString::number(_parameters->getParameter(synchro::RANGE_MEASURED_END,   -1.0)));
    xmlWriter.writeAttribute("step",  QString::number(_parameters->getParameter(synchro::RANGE_MEASURED_STEP,  -1.0)));
    xmlWriter.writeAttribute("shape", QString::number(_parameters->getParameter(synchro::CHERNYSHOV_SHAPE,  0.5)));
    xmlWriter.writeEndElement(); // ends <Range>

    xmlWriter.writeStartElement("BeamlineParameters");

    if (_parameters->contains(synchro::GEOMETRY)) {
        QString geo = _parameters->getParameter(synchro::GEOMETRY, QString("CAPILLARY"));
        xmlWriter.writeStartElement("Sample");
        xmlWriter.writeAttribute("geometry", geo);
        xmlWriter.writeEndElement(); // ends <Sample>
    }

    if (_parameters->contains(synchro::CHERNYSHOV_D)) {
        double param = _parameters->getParameter(synchro::CHERNYSHOV_D, -1.0);
        bool   ref   = _parameters->getParameter(synchro::CHERNYSHOV_D_CHECKED, false);
        xmlWriter.writeStartElement("DetectorDistance");
        xmlWriter.writeAttribute("unit", "mm");
        xmlWriter.writeAttribute("fit", ref ? "true" : "false");
        xmlWriter.writeCharacters(QString::number(param));
        xmlWriter.writeEndElement(); // ends <DetectorDistance>
    }

    if (_parameters->contains(synchro::CHERNYSHOV_P)) {
        double param = _parameters->getParameter(synchro::CHERNYSHOV_P, -1.0);
        bool   ref   = _parameters->getParameter(synchro::CHERNYSHOV_P_CHECKED, false);
        xmlWriter.writeStartElement("PixelSize");
        xmlWriter.writeAttribute("unit", "mm");
        xmlWriter.writeAttribute("fit", ref ? "true" : "false");
        xmlWriter.writeCharacters(QString::number(param));
        xmlWriter.writeEndElement(); // ends <PixelSize>
    }

    if (_parameters->contains(synchro::CHERNYSHOV_C)) {
        double param = _parameters->getParameter(synchro::CHERNYSHOV_C, -1.0);
        bool   ref   = _parameters->getParameter(synchro::CHERNYSHOV_C_CHECKED, false);
        xmlWriter.writeStartElement("SampleSize");
        xmlWriter.writeAttribute("unit", "mm");
        xmlWriter.writeAttribute("fit", ref ? "true" : "false");
        xmlWriter.writeCharacters(QString::number(param));
        xmlWriter.writeEndElement(); // ends <SampleSize>
    }

    if (_parameters->contains(synchro::CHERNYSHOV_T)) {
        double param = _parameters->getParameter(synchro::CHERNYSHOV_T, -1.0);
        bool   ref   = _parameters->getParameter(synchro::CHERNYSHOV_T_CHECKED, false);
        xmlWriter.writeStartElement("SensitiveLayer");
        xmlWriter.writeAttribute("unit", "mm");
        xmlWriter.writeAttribute("fit", ref ? "true" : "false");
        xmlWriter.writeCharacters(QString::number(param));
        xmlWriter.writeEndElement(); // ends <SensitiveLayer>
    }

    if (_parameters->contains(synchro::CHERNYSHOV_PHI)) {
        double param = _parameters->getParameter(synchro::CHERNYSHOV_PHI, -1.0);
        bool   ref   = _parameters->getParameter(synchro::CHERNYSHOV_PHI_CHECKED, false);
        bool   foc   = _parameters->getParameter(synchro::CHERNYSHOV_PHI_FOCUSED, false);
        xmlWriter.writeStartElement("BeamDivergence");
        xmlWriter.writeAttribute("unit", "degrees");
        xmlWriter.writeAttribute("fit", ref ? "true" : "false");
        xmlWriter.writeAttribute("focused", foc ? "true" : "false");
        xmlWriter.writeCharacters(QString::number(param));
        xmlWriter.writeEndElement(); // ends <BeamDivergence>
    }

    if (_parameters->contains(synchro::CHERNYSHOV_ALPHA)) {
        double alpha = _parameters->getParameter(synchro::CHERNYSHOV_ALPHA, 0.0);
        xmlWriter.writeStartElement("DetectorTilt");
        xmlWriter.writeAttribute("unit", "degrees");
        xmlWriter.writeCharacters(QString::number(alpha));
        xmlWriter.writeEndElement(); // ends <DetectorTilt>
    }

    if (_parameters->contains(synchro::POSITION_CORRECTION_MODE)) {
        int mode = _parameters->getParameter(synchro::POSITION_CORRECTION_MODE, 0);
        bool cutoff = _parameters->getParameter(synchro::DETECTOR_TRANSPARENCY_CUTOFF, false);
        xmlWriter.writeStartElement("PositionalCorrections");
        xmlWriter.writeAttribute("mode", QString::number(mode));
        xmlWriter.writeAttribute("cutoff", cutoff ? "true" : "false");
        writeDetector(xmlWriter);
        xmlWriter.writeEndElement(); // ends <PositionalCorrections>
    }

    xmlWriter.writeEndElement(); // ends <BeamlineParameters>

    xmlWriter.writeStartElement("SupportPeaks");
    for (int i = 0; i < _supportPeaks.size(); ++i) {
        writeSupportPeak(xmlWriter, _supportPeaks.at(i));
    }
    xmlWriter.writeEndElement(); // ends <SupportPeaks>

    xmlWriter.writeStartElement("Profiles");
    for (int i = 0; i < _profiles.size(); ++i) {
        writeProfile(xmlWriter, _profiles.at(i));
    }
    xmlWriter.writeEndElement(); // ends <Profiles>

    xmlWriter.writeEndElement(); // ends <ProfexSynchrotronConfiguration>
    xmlWriter.writeEndDocument();

    file.close();
}

void SynchrotronXmlIO::writeSupportPeak(QXmlStreamWriter &xmlWriter, const synchro::SupportPeak &sp)
{
    xmlWriter.writeStartElement("SupportPeak");
    xmlWriter.writeAttribute("number", QString::number(sp.number));
    xmlWriter.writeAttribute("position", QString::number(sp.position));
    xmlWriter.writeAttribute("unit", "degrees");
    xmlWriter.writeAttribute("fwhm", QString::number(sp.fwhm));
    xmlWriter.writeAttribute("shape", QString::number(sp.shape));
    xmlWriter.writeEndElement(); // ends <SupportPeak>
}

void SynchrotronXmlIO::writeProfile(QXmlStreamWriter &xmlWriter, const synchro::Profile &pr)
{
    xmlWriter.writeStartElement("Profile");
    xmlWriter.writeAttribute("number", QString::number(pr.number));
    xmlWriter.writeAttribute("position", QString::number(pr.position));
    xmlWriter.writeAttribute("unit", "degrees");
    xmlWriter.writeAttribute("fwhm", QString::number(pr.fwhm));
    xmlWriter.writeAttribute("shape", QString::number(pr.shape));
    xmlWriter.writeAttribute("area", QString::number(pr.area));

    xmlWriter.writeStartElement("PseudoVoigtX");
    xmlWriter.writeAttribute("unit", "degrees");
    xmlWriter.writeAttribute("format", "IEEE754");
    xmlWriter.writeAttribute("encoding", "base64");
    xmlWriter.writeCharacters(valuesToBase64(pr.pseudoVoigtX));
    xmlWriter.writeEndElement(); // ends <PseudoVoigtX>

    xmlWriter.writeStartElement("PseudoVoigtY");
    xmlWriter.writeAttribute("unit", "intensity");
    xmlWriter.writeAttribute("format", "IEEE754");
    xmlWriter.writeAttribute("encoding", "base64");
    xmlWriter.writeCharacters(valuesToBase64(pr.pseudoVoigtY));
    xmlWriter.writeEndElement(); // ends <PseudoVoigtY>

    xmlWriter.writeStartElement("ConvolvedX");
    xmlWriter.writeAttribute("unit", "degrees");
    xmlWriter.writeAttribute("format", "IEEE754");
    xmlWriter.writeAttribute("encoding", "base64");
    xmlWriter.writeCharacters(valuesToBase64(pr.convolvedX));
    xmlWriter.writeEndElement(); // ends <ConvolvedX>

    xmlWriter.writeStartElement("ConvolvedY");
    xmlWriter.writeAttribute("unit", "intensity");
    xmlWriter.writeAttribute("format", "IEEE754");
    xmlWriter.writeAttribute("encoding", "base64");
    xmlWriter.writeCharacters(valuesToBase64(pr.convolvedY));
    xmlWriter.writeEndElement(); // ends <ConvolvedY>

    for (int i = 0; i < pr.l2curves.size(); ++i) {
        xmlWriter.writeStartElement("L2Curve");
        xmlWriter.writeAttribute("g", QString::number(pr.l2curves.at(i).g));
        xmlWriter.writeAttribute("e", QString::number(pr.l2curves.at(i).e));
        xmlWriter.writeAttribute("q", QString::number(pr.l2curves.at(i).q));
        xmlWriter.writeEndElement(); // ends <L2Curve>
    }

    xmlWriter.writeEndElement(); // ends <Profile>
}

void SynchrotronXmlIO::writeDetector(QXmlStreamWriter &xmlWriter)
{
    if (_parameters->contains(synchro::WAVELENGTH_NM)) {
        double par1 = _parameters->getParameter(synchro::WAVELENGTH_NM, -1.0);
        xmlWriter.writeStartElement("Wavelength");
        xmlWriter.writeAttribute("unit", "nm");
        xmlWriter.writeCharacters(QString::number(par1));
        xmlWriter.writeEndElement(); // ends <Wavelength>
    }

    xmlWriter.writeStartElement("Detector");

    if (_parameters->contains(synchro::DETECTOR_MATERIAL)) {
        QString parMat = _parameters->getParameter(synchro::DETECTOR_MATERIAL, QString());
        xmlWriter.writeStartElement("DetectorMaterial");
        xmlWriter.writeCharacters(parMat);
        xmlWriter.writeEndElement(); // ends <DetectorMaterial>
    }

    if (_parameters->contains(synchro::DETECTOR_DENSITY)) {
        double parDens = _parameters->getParameter(synchro::DETECTOR_DENSITY, -1.0);
        xmlWriter.writeStartElement("DetectorDensity");
        xmlWriter.writeAttribute("unit", "g/cm3");
        xmlWriter.writeCharacters(QString::number(parDens));
        xmlWriter.writeEndElement(); // ends <DetectorDensity>
    }

    if (_parameters->contains(synchro::DETECTOR_LAC_MM)) {
        double parLac = _parameters->getParameter(synchro::DETECTOR_LAC_MM, -1.0);
        xmlWriter.writeStartElement("DetectorAbsorptionCoefficient");
        xmlWriter.writeAttribute("unit", "cm-1");
        xmlWriter.writeCharacters(QString::number(10.0 * parLac));
        xmlWriter.writeEndElement(); // ends <DetectorAbsorptionCoefficient>
    }

    xmlWriter.writeEndElement(); // ends <Detector>
}

QString SynchrotronXmlIO::valuesToBase64(const QList<double> &val)
{
    QByteArray byteArray;
    QDataStream out(&byteArray, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);           // Set the version for consistency.
    out.setByteOrder(QDataStream::BigEndian);       // Force BigEndian (network byte order).

    for (double d : val) {
        out << d;
    }

    return QString::fromLatin1(byteArray.toBase64());
}

QList<double> SynchrotronXmlIO::base64ToValues(const QString &str)
{
    QByteArray byteArray = QByteArray::fromBase64(str.toLatin1());
    QDataStream in(&byteArray, QIODevice::ReadOnly);
    in.setByteOrder(QDataStream::BigEndian);

    QList<double> list;
    // Read doubles until the end of the stream.
    while (!in.atEnd()) {
        double d;
        in >> d;
        list.append(d);
    }
    return list;
}

bool SynchrotronXmlIO::readFile(ParameterStorage *parameters,
                                QList<synchro::SupportPeak> &supportPeaks,
                                QList<synchro::Profile> &profiles)
{
    QFile file(_file);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open file for reading:" << _file;
        return false;
    }

    // Warning: When only attributes are read, xml.skipCurrentElement() must be called afterwards
    //          to advance the reader past the element.
    //          When xml.readElementText() is called, the reader is advanced automatically.

    QXmlStreamReader xml(&file);

    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.readNextStartElement()) {
            // Look for the root element.
            if (xml.name() == "ProfexSynchrotronConfiguration") {
                QString version = xml.attributes().value("version").toString();
                QString model   = xml.attributes().value("model").toString();

                Q_UNUSED(version);
                Q_UNUSED(model);

                // Process each top-level child element.
                while (xml.readNextStartElement()) {
                    if (xml.name() == "Range") {
                        double rStart = xml.attributes().value("start").toDouble();
                        double rEnd   = xml.attributes().value("end").toDouble();
                        double rStep  = xml.attributes().value("step").toDouble();
                        double rShape = xml.attributes().value("shape").toDouble();
                        parameters->setValue(synchro::RANGE_MEASURED_START, rStart);
                        parameters->setValue(synchro::RANGE_MEASURED_END,   rEnd);
                        parameters->setValue(synchro::RANGE_MEASURED_STEP,  rStep);
                        parameters->setValue(synchro::CHERNYSHOV_SHAPE, rShape);
                        xml.skipCurrentElement();
                    }
                    else if (xml.name() == "BeamlineParameters") {
                        // Read beamline parameters.
                        while (xml.readNextStartElement()) {
                            if (xml.name() == "Sample") {
                                QString geo = xml.attributes().value("geometry").toString();
                                parameters->setValue(synchro::GEOMETRY, geo);
                                xml.skipCurrentElement();
                            }
                            else if (xml.name() == "DetectorDistance") {
                                bool fit = (xml.attributes().value("fit").toString() == "true");
                                double val = xml.readElementText().toDouble();
                                parameters->setValue(synchro::CHERNYSHOV_D, val);
                                parameters->setValue(synchro::CHERNYSHOV_D_CHECKED, fit);
                            }
                            else if (xml.name() == "PixelSize") {
                                bool fit = (xml.attributes().value("fit").toString() == "true");
                                double val = xml.readElementText().toDouble();
                                parameters->setValue(synchro::CHERNYSHOV_P, val);
                                parameters->setValue(synchro::CHERNYSHOV_P_CHECKED, fit);
                            }
                            else if (xml.name() == "SampleSize") {
                                bool fit = (xml.attributes().value("fit").toString() == "true");
                                double val = xml.readElementText().toDouble();
                                parameters->setValue(synchro::CHERNYSHOV_C, val);
                                parameters->setValue(synchro::CHERNYSHOV_C_CHECKED, fit);
                            }
                            else if (xml.name() == "SensitiveLayer") {
                                bool fit = (xml.attributes().value("fit").toString() == "true");
                                double val = xml.readElementText().toDouble();
                                parameters->setValue(synchro::CHERNYSHOV_T, val);
                                parameters->setValue(synchro::CHERNYSHOV_T_CHECKED, fit);
                            }
                            else if (xml.name() == "BeamDivergence") {
                                bool fit = (xml.attributes().value("fit").toString() == "true");
                                bool foc = (xml.attributes().value("focused").toString() == "true");
                                double val = xml.readElementText().toDouble();
                                parameters->setValue(synchro::CHERNYSHOV_PHI, val);
                                parameters->setValue(synchro::CHERNYSHOV_PHI_CHECKED, fit);
                                parameters->setValue(synchro::CHERNYSHOV_PHI_FOCUSED, foc);
                            } else if (xml.name() == "DetectorTilt") {
                                double val = xml.readElementText().toDouble();
                                parameters->setValue(synchro::CHERNYSHOV_ALPHA, val);
                            }
                            else if (xml.name() == "PositionalCorrections") {
                                int mode = xml.attributes().value("mode").toInt();
                                bool cutoff = xml.attributes().value("cutoff").toString() == "true";
                                parameters->setValue(synchro::POSITION_CORRECTION_MODE, mode);
                                parameters->setValue(synchro::DETECTOR_TRANSPARENCY_CUTOFF, cutoff);
                                // Process children of PositionalCorrections.
                                while (xml.readNextStartElement()) {
                                    if (xml.name() == "Wavelength") {
                                        double val = xml.readElementText().toDouble();
                                        parameters->setValue(synchro::WAVELENGTH_NM, val);
                                    }
                                    else if (xml.name() == "Detector") {
                                        while (xml.readNextStartElement()) {
                                            if (xml.name() == "DetectorMaterial") {
                                                QString text = xml.readElementText();
                                                parameters->setValue(synchro::DETECTOR_MATERIAL, text);
                                            }
                                            else if (xml.name() == "DetectorDensity") {
                                                double val = xml.readElementText().toDouble();
                                                parameters->setValue(synchro::DETECTOR_DENSITY, val);
                                            }
                                            else if (xml.name() == "DetectorAbsorptionCoefficient") {
                                                double val = xml.readElementText().toDouble();
                                                // Reverse the multiplication by 10 applied during writing.
                                                parameters->setValue(synchro::DETECTOR_LAC_MM, val / 10.0);
                                            }
                                            else {
                                                xml.skipCurrentElement();
                                            }
                                        }
                                    }
                                    else {
                                        xml.skipCurrentElement();
                                    }
                                }
                            }
                            else {
                                xml.skipCurrentElement();
                            }
                        }
                    }
                    else if (xml.name() == "SupportPeaks") {
                        // Read all SupportPeak entries.
                        while (xml.readNextStartElement()) {
                            if (xml.name() == "SupportPeak") {
                                QXmlStreamAttributes attrs = xml.attributes();
                                int number = attrs.value("number").toInt();
                                double position = attrs.value("position").toDouble();
                                double fwhm = attrs.value("fwhm").toDouble();
                                double shape = attrs.value("shape").toDouble();
                                synchro::SupportPeak sp(number, position, fwhm, shape);
                                supportPeaks.append(sp);
                                xml.skipCurrentElement();
                            }
                            else {
                                xml.skipCurrentElement();
                            }
                        }
                    }
                    else if (xml.name() == "Profiles") {
                        // Read all Profile entries.
                        while (xml.readNextStartElement()) {
                            if (xml.name() == "Profile") {
                                QXmlStreamAttributes attrs = xml.attributes();
                                synchro::Profile pr;
                                pr.number = attrs.value("number").toInt();
                                pr.position = attrs.value("position").toDouble();
                                pr.fwhm = attrs.value("fwhm").toDouble();
                                pr.shape = attrs.value("shape").toDouble();
                                pr.area = attrs.value("area").toDouble();

                                // Process children of Profile.
                                while (xml.readNextStartElement()) {
                                    if (xml.name() == "PseudoVoigtX") {
                                        QXmlStreamAttributes pAttrs = xml.attributes();
                                        QString base64 = xml.readElementText();
                                        pr.pseudoVoigtX = base64ToValues(base64);
                                    }
                                    else if (xml.name() == "PseudoVoigtY") {
                                        QXmlStreamAttributes pAttrs = xml.attributes();
                                        QString base64 = xml.readElementText();
                                        pr.pseudoVoigtY = base64ToValues(base64);
                                    }
                                    else if (xml.name() == "ConvolvedX") {
                                        QXmlStreamAttributes pAttrs = xml.attributes();
                                        QString base64 = xml.readElementText();
                                        pr.convolvedX = base64ToValues(base64);
                                    }
                                    else if (xml.name() == "ConvolvedY") {
                                        QXmlStreamAttributes pAttrs = xml.attributes();
                                        QString base64 = xml.readElementText();
                                        pr.convolvedY = base64ToValues(base64);
                                    }
                                    else if (xml.name() == "L2Curve") {
                                        QXmlStreamAttributes lAttrs = xml.attributes();
                                        double g = lAttrs.value("g").toDouble();
                                        double e = lAttrs.value("e").toDouble();
                                        double q = lAttrs.value("q").toDouble();
                                        pr.l2curves.append(synchro::L2Curve(g, e, q));
                                        xml.skipCurrentElement();
                                    }
                                    else {
                                        xml.skipCurrentElement();
                                    }
                                }
                                profiles.append(pr);
                            }
                            else {
                                xml.skipCurrentElement();
                            }
                        }
                    }
                    else {
                        xml.skipCurrentElement();
                    }
                }
            }
            else {
                xml.skipCurrentElement();
            }
        }
    }

    file.close();
    if (xml.hasError()) {
        qDebug() << "Error parsing XML:" << xml.errorString();
        return false;
    }
    return true;
}

/***************************************************************************
                          bgmninstrumentsavparser.h  -  description
                             -------------------
    begin                : Thu June 01, 2013
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

#include "bgmninstrumentsavparser.h"
#include "bgmnfileio.h"
#include "structs.h"
#include <QDebug>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

BgmnInstrumentSavParser::BgmnInstrumentSavParser(const QMap<QString, QString> &par, const QString &header, const QString &fn)
    : fileName(fn)
{
    compose(par, header);
}

BgmnInstrumentSavParser::BgmnInstrumentSavParser(const QString &str, const QString &name)
    : fileName(name)
{
    setContent(str);
}

BgmnInstrumentSavParser::BgmnInstrumentSavParser(const QString &file, bool * ok)
{
    *ok = loadFile(file);
}

bool BgmnInstrumentSavParser::loadFile(const QString &file)
{
    fileName = file;
    QString s =BgmnFileIO::readTextFile(file);

    if (!s.isEmpty()) {
        setContent(s);
        return true;
    }

    return false;
}

void BgmnInstrumentSavParser::setContent(const QString &s)
{
    content = s.split(global::rxLineEnding);
}

QString BgmnInstrumentSavParser::getText()
{
    return content.join("\n");
}

bool BgmnInstrumentSavParser::checkOutputNames(const QString &f)
{
    QRegularExpression rxVZ("VERZERR=(.+)(?:\\.ger|\\.GER)");
    QRegularExpression rxGE("GEQ=(.+)(?:\\.geq|\\.GEQ)");

    QFileInfo fi(f);
    QString bn = fi.completeBaseName();
    int checked = 0;
    bool namesMatching = true;

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch mVZ = rxVZ.match(content.at(i));
        QRegularExpressionMatch mGE = rxGE.match(content.at(i));

        if (mVZ.hasMatch()) {
            if (mVZ.captured(1) != bn) namesMatching = false;
            ++checked;
        }

        if (mGE.hasMatch()) {
            if (mGE.captured(1) != bn) namesMatching = false;
            ++checked;
        }

        if (checked >= 2) break;
    }

    return namesMatching;
}

void BgmnInstrumentSavParser::fixOutputNames(const QString &f)
{
    QRegularExpression rxVZ("VERZERR=.+(?:\\.ger|\\.GER)");
    QRegularExpression rxGE("GEQ=.+(?:\\.geq|\\.GEQ)");

    QFileInfo fi(f);
    QString bn = fi.completeBaseName();
    int replaced = 0;

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch mVZ = rxVZ.match(content.at(i));
        QRegularExpressionMatch mGE = rxGE.match(content.at(i));

        if (mVZ.hasMatch()) {
            content[i].replace(rxVZ, QString("VERZERR=%1.ger").arg(bn));
            ++replaced;
        }

        if (mGE.hasMatch()) {
            content[i].replace(rxGE, QString("GEQ=%1.geq").arg(bn));
            ++replaced;
        }

        if (replaced >= 2) return;
    }
}

QString BgmnInstrumentSavParser::getHeader()
{
    QStringList h;
    bool hasData = false;

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).left(1) != "%") {
            if (hasData) break;
            else         continue;
        }

        h << content.at(i);
        hasData = true;
    }

    return h.join("\n");
}

QMap<QString, QString> BgmnInstrumentSavParser::getParameters()
{
    QRegularExpression rx("^(?<!%)\\s*([A-Za-z\\[\\]\\d]+)=(\\S+)");
    QRegularExpressionMatch rm;
    QMap<QString, QString> out;

    for (int i = 0; i < content.size(); ++i) {
        rm = rx.match(content.at(i));
        if (rm.hasMatch()) {
            out.insert(rm.captured(1), rm.captured(2));
        }
    }

    // if an area detector is configured, we replace the keywords DetW, DetH, DetArrayW
    // with ADetW, ADetH, ADetArrayW to be able to separate receiving slits from area detectors
    if (out.contains("DetArrayW")) {
        QString daw = out.value("DetArrayW");
        QString dw  = out.value("DetW");
        QString dh  = out.value("DetH");

        out.remove("DetArrayW");
        out.remove("DetW");
        out.remove("DetH");

        out["ADetArrayW"] = daw;
        out["ADetW"]      = dw;
        out["ADetH"]      = dh;
    }

    return out;
}

void BgmnInstrumentSavParser::compose(const QMap<QString, QString> &p, const QString &h)
{
    QString geo = p.value("GEOMETRY", "REFLEXION");
    QFileInfo fi(fileName);

    content = h.split("\n");
    content << "";
    content << QString("SAVE=%1").arg(p.value("SAVE", "N"));
    content << "";

    content << "%-----------------------------------------------------------------------------------------------";
    content << "% Output files for Geomet and MakeGeq";
    content << "%-----------------------------------------------------------------------------------------------";
    content << "";
    content << QString("VERZERR=%1").arg(p.value("VERZERR", QString("%1.ger").arg(fi.completeBaseName())));
    content << QString("GEQ=%1").arg(p.value("GEQ", QString("%1.geq").arg(fi.completeBaseName())));
    content << "";

    content << "%-----------------------------------------------------------------------------------------------";
    content << "% Goniometer";
    content << "%-----------------------------------------------------------------------------------------------";
    content << "";
    content << "% Instrument geometry";
    content << QString("GEOMETRY=%1").arg(geo);
    content << "";
    content << "% Goniometer radius (mm)";
    content << QString("R=%1").arg(p.value("R"));
    content << "";

    content << "%-----------------------------------------------------------------------------------------------";
    content << "% X-ray tube";
    content << "%-----------------------------------------------------------------------------------------------";
    content << "";
    content << "% Axial dimension (length, mm)";
    content << QString("FocusH=%1").arg(p.value("FocusH", "12.0"));
    content << "";
    content << "% Optical breadth (1/10 of the nominal) of the line focus (mm)";
    content << QString("FocusW=%1").arg(p.value("FocusW", "0.04"));

    if (p.contains("TubeTails")) {
        content << "";
        content << "% Tube tails scan file";
        content << QString("TubeTails=%1").arg(p.value("TubeTails"));

        if (p.contains("LAMBDA")) {
            content << QString("LAMBDA=%1").arg(p.value("LAMBDA", "CU"));
        } else if (p.contains("SYNCHROTRON")) {
            content << QString("SYNCHROTRON=%1").arg(p.value("SYNCHROTRON", "0.07000"));
        }
    }

    content << "";

    if (p.contains("TSlitR")) {
        content << "%-----------------------------------------------------------------------------------------------";
        content << "% Axial beam slit";
        content << "%-----------------------------------------------------------------------------------------------";
        content << "";
        content << "% Distance from sample (mm)";
        content << QString("TSlitR=%1").arg(p.value("TSlitR"));
        content << "";
        content << "% Axial slit opening (mm)";
        content << QString("TSlitH=%1").arg(p.value("TSlitH"));
        content << "";
    }

    if (p.contains("HSlitR")) {
        content << "%-----------------------------------------------------------------------------------------------";
        content << "% Divergence slit";
        content << "%-----------------------------------------------------------------------------------------------";
        content << "";

        if (p.contains("div")) {
            content << "% Beam divergence (°)";
            content << QString("div=%1").arg(p.value("div"));
            content << "";
        } else if (p.contains("irr")) {
            content << "% Irradiated length (mm)";
            content << QString("irr=%1").arg(p.value("irr"));
            content << "";
        }

        content << "% Distance from sample (mm)";
        content << QString("HSlitR=%1").arg(p.value("HSlitR"));
        content << "";
        content << "% Divergence slit width (mm)";
        content << QString("HSlitW=%1").arg(p.value("HSlitW"));
        content << "";
    }

    if (p.contains("PColl")) {
        content << "%-----------------------------------------------------------------------------------------------";
        content << "% Primary collimator (Soller slit)";
        content << "%-----------------------------------------------------------------------------------------------";
        content << "";
        content << "% Beam divergence (radians)";
        content << QString("PColl=%1").arg(p.value("PColl"));
        content << "";
    }

    if (p.contains("VSlitH")) {
        content << "%-----------------------------------------------------------------------------------------------";
        content << "% Incident beam mask";
        content << "%-----------------------------------------------------------------------------------------------";
        content << "";
        content << "% Distance form sample (mm)";
        content << QString("VSlitR=%1").arg(p.value("VSlitR"));
        content << "";
        content << "% Axial beam width (mm)";
        content << QString("VSlitH=%1").arg(p.value("VSlitH"));
        content << "";
    }

    if (p.contains("RoundSlitR")) {
        content << "%-----------------------------------------------------------------------------------------------";
        content << "% Pinhole aperture";
        content << "%-----------------------------------------------------------------------------------------------";
        content << "";
        content << "% Distance form sample (mm)";
        content << QString("RoundSlitR=%1").arg(p.value("RoundSlitR"));
        content << "";
        content << "% Pinhole diameter (mm)";
        content << QString("RoundSlitD=%1").arg(p.value("RoundSlitD"));
        content << "";
    }

    content << "%-----------------------------------------------------------------------------------------------";
    content << "% Sample";
    content << "%-----------------------------------------------------------------------------------------------";
    content << "";

    if (geo == "REFLEXION") {
        if (p.contains("SamplD")) {
            content << "% Diameter of round sample (mm)";
            content << QString("SamplD=%1").arg(p.value("SamplD"));
        } else {
            content << "% Length of rectangular sample (mm)";
            content << QString("SamplW=%1").arg(p.value("SamplW"));
            content << "";
            content << "% Axial width of rectangular sample (mm)";
            content << QString("SamplH=%1").arg(p.value("SamplH"));
        }
        if (!qFuzzyIsNull(p.value("T", "0.0").toDouble())) {
            content << "";
            content << "% Thickness (mm)";
            content << QString("T=%1").arg(p.value("T"));
        }
        if (!qFuzzyIsNull(p.value("D", "0.0").toDouble())) {
            content << "";
            content << "% Inverse linear absorption coefficient (mm)";
            content << QString("D=%1").arg(p.value("D"));
        }
        if (p.contains("DeltaOmega")) {
            static QRegularExpression rx("0.5\\*zweiTheta-(\\d+\\.?\\d*)");
            QRegularExpressionMatch rm = rx.match(p.value("DeltaOmega"));
            content << "";
            content << (rm.hasMatch() ? "% Constant grazing angle theta (deg)" : "% Omega twist (deg)");
            content << QString("DeltaOmega=%1").arg(p.value("DeltaOmega"));
        }
    } else if (geo == "CAPILLARY") {
        content << "% Length of capillary (mm)";
        content << QString("SamplH=%1").arg(p.value("SamplH"));
        content << "";
        content << "% Inner diameter of capillary (mm)";
        content << QString("T=%1").arg(p.value("T"));
        content << "";
        content << "% Inverse linear absorption coefficient (mm)";
        content << QString("D=%1").arg(p.value("D"));
    } else if (geo == "TRANSMISSION") {
        content << "% Diameter (mm)";
        content << QString("SamplD=%1").arg(p.value("SamplD"));
        content << "";
        content << "% Thickness (mm)";
        content << QString("T=%1").arg(p.value("T"));
        content << "";
        content << "% Inverse linear absorption coefficient (mm)";
        content << QString("D=%1").arg(p.value("D"));
    }

    content << "";

    if ((p.contains("AirScat")) && (geo == "REFLEXION")) {
        content << "%-----------------------------------------------------------------------------------------------";
        content << "% Beam knife";
        content << "%-----------------------------------------------------------------------------------------------";
        content << "";
        content << "% Height over sample (mm)";
        content << QString("AirScat=%1").arg(p.value("AirScat"));
        content << "";
    }

    if (p.contains("SSlitR")) {
        content << "%-----------------------------------------------------------------------------------------------";
        content << "% Anti-scatter slit";
        content << "%-----------------------------------------------------------------------------------------------";
        content << "";

        if (p.contains("adiv")) {
            content << "% Beam divergence (°)";
            content << QString("adiv=%1").arg(p.value("adiv"));
            content << "";
        } else if (p.contains("airr")) {
            content << "% Irradiated length (mm)";
            content << QString("airr=%1").arg(p.value("airr"));
            content << "";
        }

        content << "% Distance from sample (mm)";
        content << QString("SSlitR=%1").arg(p.value("SSlitR"));
        content << "";
        content << "% Anti-scatter slit width (mm)";
        content << QString("SSlitW=%1").arg(p.value("SSlitW"));
        content << "";
    }

    if (p.contains("SColl")) {
        content << "%-----------------------------------------------------------------------------------------------";
        content << "% Secondary collimator (Soller slit)";
        content << "%-----------------------------------------------------------------------------------------------";
        content << "";
        content << "% Beam divergence (radians)";
        content << QString("SColl=%1").arg(p.value("SColl"));
        content << "";
    }

    if (p.contains("MonR")) {
        content << "%-----------------------------------------------------------------------------------------------";
        content << "% Secondary beam monochromator crystal";
        content << "%-----------------------------------------------------------------------------------------------";
        content << "";
        content << "% Distance from sample (mm)";
        content << QString("MonR=%1").arg(p.value("MonR"));
        content << "";
        content << "% Polyrization";
        content << QString("POL=%1").arg(p.value("POL"));
        content << "";
    }

    content << "%-----------------------------------------------------------------------------------------------";
    content << "% Detector";
    content << "%-----------------------------------------------------------------------------------------------";
    content << "";

    if (p.contains("ADetArrayW")) {
        content << "% Total sensor height (mm)";
        content << QString("DetArrayW=%1").arg(p.value("ADetArrayW"));
        content << "";
        content << "% Height of one strip (mm)";
        content << QString("DetW=%1").arg(p.value("ADetW"));
        content << "";
        content << "% Total axial detector width (mm)";
        content << QString("DetH=%1").arg(p.value("ADetH"));
        content << "";
    } else {
        content << "% Receiving slit opening (mm)";
        content << QString("DetW=%1").arg(p.value("DetW"));
        content << "";
        content << "% Total axial detector width (mm)";
        content << QString("DetH=%1").arg(p.value("DetH"));
        content << "";
    }

    content << "%-----------------------------------------------------------------------------------------------";
    content << "% Parameters for the simulation of the profile function";
    content << "%-----------------------------------------------------------------------------------------------";
    content << "";
    content << "% angular positions for the MonteCarlo simulation (deg 2theta)";

    int n = 1;
    double wmin = p.value("WMIN", "2.0").toDouble();
    double wmax = p.value("WMAX", "150.0").toDouble();

    content << QString("zweiTheta[%1]=%2").arg(n).arg(wmin, 0, 'f', 0);
    ++n;

    for (int i = 1; i < 18; ++i) {
        double w = 180.0 * qPow((qSin(M_PI * 5.0 * double(i) / 180.0)), 2.0);

        if ((round(w) > wmin) && (round(w) < wmax)) {
            content << QString("zweiTheta[%1]=%2").arg(n).arg(round(w), 0, 'f', 0);
            ++n;
        }
    }

    content << QString("zweiTheta[%1]=%2").arg(n).arg(wmax, 0, 'f', 0);

    content << "";
    content << "% angular range (deg 2theta)";
    content << QString("WMIN=%1").arg(wmin, 0, 'f', 0);
    content << QString("WMAX=%1").arg(wmax, 0, 'f', 0);
    content << "";
    content << "% step width for the interpolation of the geometric profiles (deg 2theta)";
    content << "WSTEP=2*sin(pi*zweiTheta/180)";
    content << "";
    content << "% switch for applying the intensity correction for beam overflow resp. ADS function";
    content << "GSUM=Y";
    content << "";
    content << "% Use multithreaded calculation";
    content << "NTHREADS=8";
    content << "";
    content << "% Convenience function: Calculate PI for use in other angle-dependent calculations";
    content << "pi=2*acos(0)";
    content << "";
    content << "%-----------------------------------------------------------------------------------------------";
    content << "% End of file";
    content << "%-----------------------------------------------------------------------------------------------";

}

QString BgmnInstrumentSavParser::getTubeTailsFile()
{
    static QRegularExpression rx("^[^%#]*TubeTails=(.+)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        rm = rx.match(content.at(i));
        if (rm.hasMatch()) return rm.captured(1).trimmed();
    }

    return QString();
}

bool BgmnInstrumentSavParser::hasTubeTails()
{
    static QRegularExpression rx("^[^%#]*TubeTails=.+");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        rm = rx.match(content.at(i));
        if (rm.hasMatch()) return true;
    }

    return false;
}

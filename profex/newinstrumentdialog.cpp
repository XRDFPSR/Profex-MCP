/***************************************************************************
                          newinstrumentdialog.cpp  -  description
                             -------------------
    begin                : Sun Nov 30 2014
    copyright            : (C) 2003-2014 by Nicola Doebelin
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

#include "newinstrumentdialog.h"
#include "ui_newinstrumentdialog.h"

#include "../libXrdIO/scan.h"

#include <QFileDialog>
#include <QVector>
#include <QVariantHash>
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <math.h>

#define PI 3.14159265359

NewInstrumentDialog::NewInstrumentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewInstrumentDialog)
{
    ui->setupUi(this);

    settings = SettingsManager::getInstance();

    restoreGeometry(settings->value("newInstrumentDialog/geometry", QByteArray()).toByteArray());

    bruRawImport = new BrukerRawImport();
    panXrdmlImport = new PanalyticalXrdmlImport();
    bruBrmlImport = new BrukerBrmlImport();

    separ = QStringLiteral("%----------------------------------------------------\n");
    dsType = 0;

    QMessageBox::information(this, tr("Hardware information"), tr("This function is experimental. "
                                                                  "Results may be inaccurate, incomplete, or wrong.\n\n"
                                                                  "Currently supported file formats:\n\n"
                                                                  "PANalytical XRDML\nBruker RAW V4\nBruker BRML"));
}

NewInstrumentDialog::~NewInstrumentDialog()
{
    if (bruRawImport)   delete bruRawImport;
    if (panXrdmlImport) delete panXrdmlImport;
    if (bruBrmlImport)  delete bruBrmlImport;

    delete ui;
}

void NewInstrumentDialog::accept()
{
    savFile = QFileDialog::getSaveFileName(this, tr("Save file as..."), workingDir, tr("Instrument configuration (*.sav *.SAV)"));

    QFileInfo fname(savFile);
    QFile file(fname.absoluteFilePath());
    file.open(QIODevice::WriteOnly | QFile::Text);

    QTextStream str(&file);
    str << generateFileString(fname.baseName());

    file.close();

    settings->setValue("newInstrumentDialog/geometry", geometry());
    QDialog::accept();
}

void NewInstrumentDialog::reject()
{
    settings->setValue("newInstrumentDialog/geometry", geometry());
    QDialog::reject();
}

bool NewInstrumentDialog::openFile()
{
    ui->tableWidget->clear();
    ui->plainTextEdit->clear();
    initVarList();

    QStringList filters;

    filters.append(bruRawImport->filter());
    filters.append(panXrdmlImport->filter());
    filters.append(bruBrmlImport->filter());

    scanFile = QFileDialog::getOpenFileName(this, tr("Scan File"), workingDir, filters.join(";;"));
    if (!QFile::exists(scanFile)) return false;

    QFileInfo scanFileInfo(scanFile);

    if (scanFileInfo.suffix().toLower() == "raw") parseBrukerRaw(scanFileInfo.absoluteFilePath());
    if (scanFileInfo.suffix().toLower() == "brml") parseBrukerBRML(scanFileInfo.absoluteFilePath());
    if (scanFileInfo.suffix().toLower() == "xrdml") parsePanalyticalXRDML(scanFileInfo.absoluteFilePath());

    return true;
}

void NewInstrumentDialog::parseBrukerRaw(const QString &file)
{
    QVector<Scan> scanHeap;
    bruRawImport->load(file, scanHeap, false);

    if (!scanHeap.size()) return;

    QVariantHash auxInfo = scanHeap[0].allAuxInfo();

    if (!auxInfo.contains("FormatVersion")) {
        qDebug() << QString("NewInstrumentDialog::parseBrukerRaw(): Not a Bruker Raw file: %1").arg(file);
        return;
    }

    QVariantHash::ConstIterator it = auxInfo.constBegin();
    while (it != auxInfo.constEnd()) {
        ui->plainTextEdit->appendPlainText(QString("%1 = %2").arg(it.key()).arg(it.value().toString()));
        ++it;
    }

    QString format = auxInfo.value("FormatVersion").toString();

    if (format == "RAW4") {
        parseBrukerRawV4(auxInfo);
        return;
    }

    if (format == "RAW1.01") {
//TODO        parseBrukerRawV3(auxInfo);
        return;
    }

    qDebug() << QString("NewInstrumentDialog::parseBrukerRaw(): No format version info found in file %1").arg(file);
}

void NewInstrumentDialog::parseBrukerRawV4(const QVariantHash &auxInfo)
{
    varList["SamplD"].value = QStringLiteral("25");
    varList["FocusW"].value = QStringLiteral("%0.4/10");
    varList["FocusH"].value = QStringLiteral("%12");

    double radius = auxInfo.value("fGoniomDiameter", 0.0).toDouble() / 2.0;
    if (radius > 0.0) varList["R"].value = QString::number(radius, 'f', 1);

    double fpsoller = auxInfo.value("fPrimSollerSlit", 0.0).toDouble() * PI / 360.0;
    if (fpsoller > 0.0) varList["PColl"].value = QString::number(fpsoller, 'f', 5);

    double fssoller = auxInfo.value("fSecondSollerSlit", 0.0).toDouble() * PI / 360.0;
    if (fssoller > 0.0) varList["SColl"].value = QString::number(fssoller, 'f', 5);

    if (auxInfo.contains("iAdditionalDetectorFlags")) {
        QStringList det = auxInfo.value("iAdditionalDetectorFlags").toStringList();
        if (det.contains("PSD_SET")) {
            varList["DetArrayW"].value = QStringLiteral("14.4 % LynxEye (XE), 192 Channels\n% DetArrayW=12.0 % LynxEye SSE, 160 Channels");
            varList["DetW"].value = QStringLiteral("0.075");
            varList["DetH"].value = QStringLiteral("16.0");
        } else {
            varList["DetW"].value = auxInfo.value("fDetSlit").toString();
        }
    }

    if (auxInfo.value("iGoniomStage") == "PHI_STAGE") varList["GEOMETRY"].value = QStringLiteral("REFLEXION");
    if (auxInfo.value("iGoniomStage") == "STANDARD_STAGE") varList["GEOMETRY"].value = QStringLiteral("REFLEXION");
    if (auxInfo.value("iGoniomStage") == "ROT_REFLECTION") varList["GEOMETRY"].value = QStringLiteral("REFLEXION");
    if (auxInfo.value("iGoniomStage") == "UNKNOWN") varList["GEOMETRY"].value = QStringLiteral("REFLEXION");
    if (auxInfo.value("iGoniomStage") == "ROT_TRANSMISSION") varList["GEOMETRY"].value = QStringLiteral("CAPILLARY");

    if (auxInfo.contains("fDivSlit")) {
        if (auxInfo.value("fDivSlit") == "9999") {
            // probably automatic
            varList.remove("dsdiv");
            varList["HSlitW"].value = QStringLiteral("(2*(R-HSlitR)*irr*sin(pi*zweiTheta/360))/(2*R+irr*cos(pi*zweiTheta/360))");
        } else {
            varList.remove("irr");
            varList["dsdiv"].value = auxInfo.value("fDivSlit").toString();
            varList["HSlitW"].value = QString("%1\n% HSlitW=2*tan(dsdiv*pi/360)*(R-HSlitR)").arg(varList["dsdiv"].value);
        }
    }

    if (auxInfo.contains("fAntiScSlit")) {
        if (auxInfo.value("fAntiScSlit") == "9999") {
            // probably automatic
            varList.remove("asdiv");
            varList["SSlitW"].value = QStringLiteral("(2*(R-HSlitR)*irr*sin(pi*zweiTheta/360))/(2*R+irr*cos(pi*zweiTheta/360))");
        } else {
            varList.remove("irr");
            varList["asdiv"].value = auxInfo.value("fDivSlit").toString();
            varList["SSlitW"].value = QString("%1\n% SSlitW=2*tan(asdiv*pi/360)*(R-HSlitR)").arg(varList["asdiv"].value);
        }
    }

    QString mon = auxInfo.value("iAnalyzer").toString();
    if (mon.isNull() || (mon == "NONE") || (mon == "UNKNOWN")) {
        varList.remove("MonR");
    }

    createTable();
}

void NewInstrumentDialog::parseBrukerBRML(const QString &file)
{
    QVector<Scan> scanHeap;
    bruBrmlImport->load(file, scanHeap, false);

    if (!scanHeap.size()) return;

    QVariantHash auxInfo = scanHeap[0].allAuxInfo();
    dumpToText(auxInfo);

    createTable();
}

void NewInstrumentDialog::parsePanalyticalXRDML(const QString &file)
{
    QVector<Scan> scanHeap;
    panXrdmlImport->load(file, scanHeap, false);

    if (!scanHeap.size()) return;

    QVariantHash auxInfo = scanHeap[0].allAuxInfo();
    dumpToText(auxInfo);

    // Goniometer radius
    varList["R"].value = auxInfo.value("incidentBeamPath:radius", "").toString();

    // Divergence slit
    varList["HSlitR"].value = auxInfo.value("incidentBeamPath:divergenceSlit:distanceToSample", "").toString();

    if (auxInfo.value("incidentBeamPath:divergenceSlit:type").toString() == "automaticDivergenceSlitType") {
        dsType = 1;
        varList.remove("dsdiv");
        varList["irr"].value = auxInfo.value("incidentBeamPath:divergenceSlit:irradiatedLength", "").toString();
        varList["HSlitW"].value = QStringLiteral("(2*(R-HSlitR)*irr*sin(pi*zweiTheta/360))/(2*R+irr*cos(pi*zweiTheta/360))");
    } else {
        dsType = 0;
        varList.remove("irr");
        varList["dsdiv"].value = auxInfo.value("incidentBeamPath:divergenceSlit:angle", "").toString();

        if (!auxInfo.value("incidentBeamPath:divergenceSlit:height", "").toString().isEmpty()) {
            // use the opening in mm if available
            varList["HSlitW"].value = auxInfo.value("incidentBeamPath:divergenceSlit:height", "").toString();
        } else {
            // else use the formula to calculate the opening in mm from "div" in degrees
            varList["HSlitW"].value = QStringLiteral("2*tan(dsdiv*pi/360)*(R-HSlitR)");
        }
    }

    // Anti-scatter slit
    varList["SSlitR"].value = auxInfo.value("diffractedBeamPath:antiScatterSlit:distanceToSample", "").toString();

    if (auxInfo.value("diffractedBeamPath:antiScatterSlit:type").toString() == "automaticAntiScatterSlitType") {
        varList.remove("asdiv");
        varList["irr"].value = auxInfo.value("diffractedBeamPath:antiScatterSlit:observedLength", "").toString();
        varList["SSlitW"].value = QStringLiteral("(2*(R-HSlitR)*irr*sin(pi*zweiTheta/360))/(2*R+irr*cos(pi*zweiTheta/360))");
    } else {
        varList.remove("irr");
        varList["asdiv"].value = auxInfo.value("diffractedBeamPath:antiScatterSlit:angle", "").toString();

        if (!auxInfo.value("diffractedBeamPath:antiScatterSlit:height", "").toString().isEmpty()) {
            // use the opening in mm if available
            varList["SSlitW"].value = auxInfo.value("diffractedBeamPath:antiScatterSlit:height", "").toString();
        } else {
            // else use the formula to calculate the opening in mm from "div" in degrees
            varList["SSlitW"].value = QStringLiteral("2*tan(asdiv*pi/360)*(R-SSlitR)");
        }
    }

    // Soller slit (XRDML only uses RAD unit)
    varList["PColl"].value = auxInfo.value("incidentBeamPath:sollerSlit:opening").toString();
    varList["SColl"].value = auxInfo.value("diffractedBeamPath:sollerSlit:opening").toString();

    // Mask
    varList["VSlitH"].value = auxInfo.value("incidentBeamPath:mask:width").toString();
    varList["VSlitR"].value = auxInfo.value("incidentBeamPath:mask:distanceToSample").toString();

    // Focus
    if (auxInfo.value("incidentBeamPath:xRayTube:focus:length").isNull()) {
        varList["FocusH"].value = QStringLiteral("12.0 % guessed");
    } else {
        QString flength = auxInfo.value("incidentBeamPath:xRayTube:focus:length").toString();
        varList["FocusH"].value = flength;
    }

    if (auxInfo.value("incidentBeamPath:xRayTube:focus:width").isNull()) {
        varList["FocusW"].value = QStringLiteral("0.04 % guessed");
    } else {
        double foc = auxInfo.value("incidentBeamPath:xRayTube:focus:width").toDouble();

        if (auxInfo.value("incidentBeamPath:xRayTube:focus:takeOffAngle").isNull()) {
            foc /= 10.0;
        } else {
            double toa = auxInfo.value("incidentBeamPath:xRayTube:focus:takeOffAngle").toDouble() * PI / 180.0;
            foc *= sin(toa);
        }

        varList["FocusW"].value = QString::number(foc, 'f', 4);
    }

    varList["SamplD"].value = QStringLiteral("25.4 % guessed");

    // Detector
    QString detType = auxInfo.value("diffractedBeamPath:detector:type").toString();

    if (detType == "pointDetectorType") {
        varList["DetW"].value = auxInfo.value("diffractedBeamPath:receivingSlit:height").toString();
        varList["DetH"].value = QStringLiteral("15 % guessed");
        varList.remove("DetArrayW");
    }

    if (detType == "rtmsDetectorType") {
        // X'Celerator resolution: 128 lines, line size 70 um
        varList["DetArrayW"].value = QStringLiteral("127 * DetW");
        varList["DetW"].value = QStringLiteral("0.070");
        varList["DetH"].value = QStringLiteral("15");
    }

    if (detType == "areaDetectorType") {
        // PIXCel resolution: 256 X 256 pixel, pixel size 55 x 55 um
        varList["DetArrayW"].value = QStringLiteral("256 * DetW");
        varList["DetW"].value = QStringLiteral("0.055");
        varList["DetH"].value = QStringLiteral("256 * DetW");
    }

    // Monochromator
    if (!auxInfo.value("diffractedBeamPath:monochromator:crystal:type").isNull()) {
        // seems to have a monochromator
        varList["MonR"].value = QStringLiteral("R + 50 % guessed");
        varList["MonH"].value = QStringLiteral("15 % guessed");
    } else {
        varList.remove("MonR");
        varList.remove("MonH");
    }

    // geometry
    if (auxInfo.value("sampleMode").toString() == "Reflection") {
        varList["GEOMETRY"].value = QStringLiteral("REFLEXION");
    }

    if (auxInfo.value("sampleMode").toString() == "Transmission") {
        varList["GEOMETRY"].value = QStringLiteral("TRANSMISSION");
    }

    if (auxInfo.value("sampleMode").toString() == "Capillary") {
        varList["GEOMETRY"].value = QStringLiteral("CAPILLARY");
    }

    if (auxInfo.value("sampleMode").isNull()) {
        varList["GEOMETRY"].value = QStringLiteral("REFLEXION % Guessed");
    }

    createTable();
}

void NewInstrumentDialog::createTable()
{
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setRowCount(varList.size());

    QStringList headers;
    headers << QString(tr("BGMN Variable")) << QString(tr("Value"));
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    QMap<QString, Variable>::ConstIterator it = varList.constBegin();
    int n = 0;
    while (it != varList.constEnd()) {
        QTableWidgetItem *itN = new QTableWidgetItem(it.value().name);
        QTableWidgetItem *itV = new QTableWidgetItem(it.value().value);

        itN->setToolTip(it.value().description);
        itV->setToolTip(it.value().description);

        ui->tableWidget->setItem(n, 0, itN);
        ui->tableWidget->setItem(n, 1, itV);
        ++it;
        ++n;
    }
}

QString NewInstrumentDialog::generateFileString(const QString &basename)
{
    QString out;

    out += "%***********************************************************************************************\n%\n";
    out += "%    BGMN Device Configuration\n";
    out += "%    -------------------------\n%\n";
    out += QString("%    Created by Profex from file %1\n").arg(scanFile);
    out += QString("%    %1 \n").arg(QDateTime::currentDateTime().toString("MMMM dd, yyyy"));
    out += "%\n";
    out += "%***********************************************************************************************\n\n";

    out += box("Output files for Geomet and MakeGeq");
    out += QString("VERZERR=%1.ger\n").arg(basename);
    out += QString("GEQ=%1.geq\n\n").arg(basename);

    out += box("X-ray tube");
    out += QString("% axial dimension (length, mm)\nFocusH=%1\n\n").arg(varList["FocusH"].value);
    out += QString("% optical breadth of the line focus (mm)\nFocusW=%1\n\n").arg(varList["FocusW"].value);

    out += box("Goniometer radius (mm)");
    out += QString("R=%1\n\n").arg(varList["R"].value);

    out += box("Divergence slit");
    if (varList.contains("dsdiv")) out += QString("% Beam divergence (deg)\ndsdiv=%1\n\n").arg(varList["dsdiv"].value);
    if (varList.contains("irr")) out += QString("% Irradiated length (mm)\nirr=%1\n\n").arg(varList["irr"].value);
    out += QString("% Distance from sample (mm)\nHSlitR=%1\n\n").arg(varList["HSlitR"].value);
    out += QString("% Slit width (mm)\nHSlitW=%1\n\n").arg(varList["HSlitW"].value);

    out += box("Incicent Beam Mask");
    out += QString("% Distance from sample (mm)\nVSlitR=%1\n\n").arg(varList["VSlitR"].value);
    out += QString("% Mask height (mm)\nVSlitH=%1\n\n").arg(varList["VSlitH"].value);

    if (varList.contains("DetArrayW")) {
        out += box("Detector");
        out += QString("% Total detector width (mm)\nDetArrayW=%1\n\n").arg(varList["DetArrayW"].value);
        out += QString("% Width of one strip (mm)\nDetW=%1\n\n").arg(varList["DetW"].value);
        out += QString("% Total detector height (mm)\nDetH=%1\n\n").arg(varList["DetH"].value);
    } else {
        out += box("Receiving slit");
        out += QString("% Slit width (mm)\nDetW=%1\n\n").arg(varList["DetW"].value);
        out += QString("% Slit height (mm)\nDetH=%1\n\n").arg(varList["DetH"].value);
    }

    out += box("Soller slits");
    out += QString("% primary soller slit (radians)\nPColl=%1\n\n").arg(varList["PColl"].value);
    out += QString("% secondary soller slit (radians)\nPColl=%1\n\n").arg(varList["SColl"].value);

    out += box("Anti-scatter slit");
    if (varList.contains("asdiv")) out += QString("% Beam divergence (deg)\nasdiv=%1\n\n").arg(varList["asdiv"].value);
    if (varList.contains("irr")) out += QString("% Irradiated length (mm)\nirr=%1\n\n").arg(varList["irr"].value);
    out += QString("% Distance from sample (mm)\nSSlitR=%1\n\n").arg(varList["SSlitR"].value);
    out += QString("% Slit width (mm)\nSSlitW=%1\n\n").arg(varList["SSlitW"].value);

    if (varList.contains("MonR")) {
        out += box("Monochromator");
        out += QString("% Distance from sample (mm)\n%1\n\n").arg(varList["MonR"].value);
    }

    out += box("Sample holder");
    out += QString("% Diameter (mm)\nSamplD=%1\n\n").arg(varList["SamplD"].value);

    out += box("Parameters for the simulation of the profile function");

    out += ("% angular positions for the MonteCarlo simulation (deg 2theta)\n");
    out += ("zweiTheta[1]=4\nzweiTheta[2]=8\nzweiTheta[3]=13\n");
    out += ("zweiTheta[4]=20\nzweiTheta[5]=30\nzweiTheta[6]=42\n");
    out += ("zweiTheta[7]=56\nzweiTheta[8]=76\nzweiTheta[9]=90\n");
    out += ("zweiTheta[10]=105\nzweiTheta[11]=120\nzweiTheta[12]=135\n");
    out += ("zweiTheta[13]=150\n\n");

    out += ("% angular range (deg 2theta)\nWMIN=4\nWMAX=150\n\n");

    out += ("% step width for the interpolation of the geometric profiles (deg 2theta)\n");
    out += ("WSTEP=3*sin(pi*zweiTheta/180)\n\n");

    out += ("% switch for applying the intensity correction for beam overflow resp. ADS function\n");
    out += ("GSUM=Y\n\n");

    out += ("% Use multithreaded calculation\n");
    out += ("NTHREADS=4\n\n");

    out += ("% Convenience function: Calculate PI for use in other angle-dependent calculations\n");
    out += ("pi=2*acos(0)\n\n");

    out += box("End of file");
    return QString(out);
}

void NewInstrumentDialog::initVarList()
{
    varList.clear();

    /*
    varList.insert("TubeTails", Variable("TubeTails", "%unknown", "File with pattern data for Tube Tails\n"
                                                          "correction. Implementation restriction:\n"
                                                          "This file may contain only one scan of\n"
                                                          "equidistant data. Possible file formats \n"
                                                          "are identic to the VAL[i] entry.", ""));
    */

    varList.insert("R", Variable("R", "%unknown", "Gomiometer radius.", ""));

    varList.insert("FocusH", Variable("FocusH", "%unknown", "Axial (FocusH) and equatorial (FocusW)\n"
                                                    "dimension of the X-ray tube's focus.\n"
                                                    "FocusW means the optical focus width.\n"
                                                    "Usually, the thermal focus dimensions\n"
                                                    "are printed on the X-ray tube. The optical\n"
                                                    "equatorial focus dimension is reduced by\n"
                                                    "the take-off angle of X-rays from the\n"
                                                    "anode surface (usually 6°). In following,\n"
                                                    "the optical equatorial focus dimension equals\n"
                                                    "the 10th part of the thermal one.", ""));

    varList.insert("FocusW", Variable("FocusW", "%unknown", "Axial (FocusH) and equatorial (FocusW)\n"
                                                    "dimension of the X-ray tube's focus.\n"
                                                    "FocusW means the optical focus width.\n"
                                                    "Usually, the thermal focus dimensions\n"
                                                    "are printed on the X-ray tube. The optical\n"
                                                    "equatorial focus dimension is reduced by\n"
                                                    "the take-off angle of X-rays from the\n"
                                                    "anode surface (usually 6°). In following,\n"
                                                    "the optical equatorial focus dimension equals\n"
                                                    "the 10th part of the thermal one.", ""));

    varList.insert("HSlitR", Variable("HSlitR", "%unknown", "Radius (distance from device axis) of the\n"
                                                    "equatorial divergence slit. May depend from\n"
                                                    "the variable zweiTheta, which means variable\n"
                                                    "divergence slit. In this case GSUM=Y should\n"
                                                    "be used.", ""));

    varList.insert("HSlitW", Variable("HSlitW", "%unknown", "Width of the equatorial divergence slit. May\n"
                                                    "depend from the variable zweiTheta, which\n"
                                                    "means variable divergence slit. In this case\n"
                                                    "GSUM=Y should be used.", ""));

    /*
    varList.insert("RoundSlitR", Variable("RoundSlitR", "%unknown", "Radius (distance from device axis) \n"
                                                            "of a round slit (plate with a\n"
                                                            "circular hole) in the primary beam.", ""));

    varList.insert("RoundSlitD", Variable("RoundSlitD", "%unknown", "Diameter of a round slit (plate with a\n"
                                                            "circular hole) in the primary beam.", ""));
    */

    varList.insert("PColl", Variable("PColl", "%unknown", "Divergency angle of the primary soller slit,\n"
                                                  "e.g. 0.5/25 (unit is radian). Default is no\n"
                                                  "collimator.", ""));

    /*
    varList.insert("PCollA", Variable("PCollA", "%unknown", "Out-of-plane-angle of primary soller slit.", ""));
    */

    varList.insert("VSlitR", Variable("VSlitR", "%unknown", "Radius (distance from device axis) of\n"
                                                    "the axial divergence slit.", ""));

    varList.insert("VSlitH", Variable("VSlitH", "%unknown", "Width of the axial divergence slit.", ""));

    varList.insert("irr", Variable("irr", "%unknown", "Irradiated length (mm) on the sample surface.\n"
                                              "Used for variable divergence and anti-scatter slits.\n\n"
                                              "Profex convenience variable.", ""));

    varList.insert("dsdiv", Variable("dsdiv", "%unknown", "Beam divergence angle in degrees.\n"
                                              "Used for fixed divergence slits.\n\n"
                                              "Profex convenience variable.", ""));

    varList.insert("asdiv", Variable("asdiv", "%unknown", "Beam divergence angle in degrees.\n"
                                              "Used for fixed anti-scatter slits.\n\n"
                                              "Profex convenience variable.", ""));

    varList.insert("SamplD", Variable("SamplD", "%unknown", "Diameter of a round specimen. Default value\n"
                                                    "is infinite (sufficient large). Using a small\n"
                                                    "value, you should give GSUM=Y.", ""));

    /*
    varList.insert("DeltaOmega", Variable("DeltaOmega", "%unknown", "Omega twist of the specimen, valid\n"
                                                            "only for GEOMTRY=REFLEXION. Changes\n"
                                                            "profile shapes plus intensities (GEOMET)\n"
                                                            "and changes effective penetration depths\n"
                                                            "depending from zweiTheta (MAKEGEQ). May\n"
                                                            "depend from zweiTheta. Therefore, may be\n"
                                                            "used for simulating grazing incidence, too.", ""));

    varList.insert("AirScat", Variable("AirScat", "%unknown", "May be set for defining an air scatter blocker\n"
                                                      "(a metal plate perpendicular to the sample surface,\n"
                                                      "the goniometer axis within the plane of the plate).\n"
                                                      "AirScat should be set to the height of the blocker\n"
                                                      "above sample surface. Valid only for\n"
                                                      "GEOMETRY=REFLEXION.", ""));

    varList.insert("SamplW", Variable("SamplW", "%unknown", "Length and equatorial dimension of a rectangular,\n"
                                                    "not rotated specimen. Default values are infinite\n"
                                                    "(sufficient large).", ""));

    varList.insert("SamplH", Variable("SamplH", "%unknown", "Length and equatorial dimension of a rectangular,\n"
                                                    "not rotated specimen. Default values are infinite\n"
                                                    "(sufficient large).", ""));
    */

    varList.insert("SSlitW", Variable("SSlitW", "%unknown", "Width of the anti-scatter slit. May depend from the\n"
                                                    "variable zweiTheta, which means variable anti-\n"
                                                    "scatter slit. If used in conjunction with TubeTails,\n"
                                                    "you should use the switch GSUM=Y.", ""));

    varList.insert("SSlitR", Variable("SSlitR", "%unknown", "Radius (distance from device axis)\n"
                                                    "of the anti-scatter slit. May depend from the\n"
                                                    "variable zweiTheta, which means variable anti-\n"
                                                    "scatter slit. If used in conjunction with TubeTails,\n"
                                                    "you should use the switch GSUM=Y.", ""));

    varList.insert("SColl", Variable("SColl", "%unknown", "Divergency angle of the secondary\n"
                                                  "soller slit.", ""));

    /*
    varList.insert("SCollA", Variable("SCollA", "%unknown", "Out-of-plane-angle of secondary\n"
                                                    "soller slit.", ""));
    */

    varList.insert("DetW", Variable("DetW", "%unknown", "Equatorial dimension of the receiving slit.\n"
                                                "DetW may depend on Bragg angle zweiTheta in\n"
                                                "cases of variable receiving slit, as\n"
                                                "described for variable divergence slits.", ""));

    varList.insert("DetH", Variable("DetH", "%unknown", "Axial dimension of the receiving slit.", ""));

    varList.insert("DetArrayW", Variable("DetArrayW", "%unknown", "Total equatorial dimension (in mm)\n"
                                                          "of a 1D array of detectors such as Vantec-1,\n"
                                                          "Lynxeye (Bruker) or X'Celerator (Panalytical).\n"
                                                          "In such cases, DetW should be assigned the\n"
                                                          "equatorial dimension of a single detection\n"
                                                          "unit and DetH the axial dimension of the\n"
                                                          "detection units.", ""));

    varList.insert("MonR", Variable("MonR", "%unknown", "Radius of a secondary monochromator (beam path\n"
                                                "length from device axis). It's axial dimension\n"
                                                "is smaller than those of the detection slit,\n"
                                                "in most cases. Therefore give MonR and use the\n"
                                                "monochromator crystal's axial dimension as MonH.", ""));

    varList.insert("MonH", Variable("MonH", "%unknown", "Radius of a secondary monochromator (beam path\n"
                                                "length from device axis). It's axial dimension\n"
                                                "is smaller than those of the detection slit,\n"
                                                "in most cases. Therefore give MonR and use the\n"
                                                "monochromator crystal's axial dimension as MonH.", ""));

    /*
    varList.insert("EPSG", Variable("EPSG", "%unknown", "Computational accuracy.\n"
                                                "Default value: 0.7 per cent.", ""));

    varList.insert("TSlitR", Variable("TSlitR", "%unknown", "The usage of an additional axial\n"
                                                    "slit near the X-ray tube is supported.\n"
                                                    "These variables describe it's distance\n"
                                                    "from device axis and axial dimension.\n"
                                                    "Optional.", ""));

    varList.insert("TSlitH", Variable("TSlitH", "%unknown", "The usage of an additional axial\n"
                                                    "slit near the X-ray tube is supported.\n"
                                                    "These variables describe it's distance\n"
                                                    "from device axis and axial dimension.\n"
                                                    "Optional.", ""));

    varList.insert("FocusS", Variable("FocusS", "%unknown", "These variables describe a axial shift\n"
                                                    "and an rotation angle (around the line\n"
                                                    "tube-specimen) of the X-ray focus.\n"
                                                    "Optional.", ""));

    varList.insert("FocusA", Variable("FocusA", "%unknown", "These variables describe a axial shift\n"
                                                    "and an rotation angle (around the line\n"
                                                    "tube-specimen) of the X-ray focus.\n"
                                                    "Optional.", ""));
    */

    varList.insert("GEOMETRY", Variable("GEOMETRY", "%unknown", "Measuring geometry. Default value\n"
                                                        "is REFLEXION. The value TRANSMISSION\n"
                                                        "describes a planar, thin transmission\n"
                                                        "specimen. The value CAPILLARY describes\n"
                                                        "a thin, wire-like specimen in the\n"
                                                        "goniometer axis. In the case CAPILLARY,\n"
                                                        "SamplD or SamplH describes the length\n"
                                                        "(heigth) of the specimen, and T desribes\n"
                                                        "the diameter of the wire (capillary).", ""));

    /*
    varList.insert("D", Variable("D", "%unknown", "Reciprocal value of the linear\n"
                                          "absorption's coefficient of the\n"
                                          "specimen. Default value: zero (no\n"
                                          "correction of the profile shape for\n"
                                          "penetration depth)", ""));

    varList.insert("T", Variable("T", "%unknown", "Thickness of specimen\n"
                                          "(GEOMETRY=REFLEXION/TRANSMISSION)\n"
                                          "or capillary/wire diameter\n"
                                          "(GEOMETRY=CAPILLARY).\n"
                                          "Default value: Infinity (sufficient\n"
                                          "thick) in the case REFLEXION, zero\n"
                                          "(thin specimen or capillary) in the\n"
                                          "cases TRANSMISSION/CAPILLARY.", ""));
    */
}

QString NewInstrumentDialog::box(const QString &s)
{
    QString line("%-----------------------------------------------------------------------------------------------");
    return QString("%1\n% %2\n%3\n\n").arg(line).arg(s).arg(line);
}

/*
 * writes the auxInfo to the textEdit and applies some sorting and formatting
 */
void NewInstrumentDialog::dumpToText(const QVariantHash &auxInfo)
{
    QMap<QString, QString> auxData;

    // store the auxInfo data in a map to sort alphabetically
    QVariantHash::const_iterator hit = auxInfo.constBegin();

    while (hit != auxInfo.constEnd()) {
        // Here we re-format the output a little bit for the text output.
        // Whenever a separate key for ":Value" and ":Unit" is available, we
        // merge them, just to make the output a bit tidier.
        //
        // Example: Instead of adding to the list:
        //
        // SecondaryTracks:Slit_open_1:Width:Value = 14
        // SecondaryTracks:Slit_open_1:Width:Unit = mm
        //
        // we only add:
        //
        // SecondaryTracks:Slit_open_1:Width = 14 mm


        // ignore entries ending with ":Unit"
        if ((hit.key().right(5) == ":Unit") || (hit.key().right(5) == ":unit")) {
            ++hit;
            continue;
        }

        // merge entries ending with ":Value" with the corresponding ":Unit" entry
        if (hit.key().right(6) == ":Value") {
            QString key = hit.key().left(hit.key().length() - 6);
            QString ukey = QString("%1:%2").arg(key).arg("Unit");

            auxData.insert(key, QString("%1 %2").arg(hit.value().toString()).arg(auxInfo.value(ukey).toString()));

            ++hit;
            continue;
        }

        QString val = hit.value().toString();

        if (auxInfo.contains(QString("%1:unit").arg(hit.key()))) {
            val += " " + auxInfo.value(QString("%1:unit").arg(hit.key())).toString();
        }

        // entries not ending on ":Value" or ":Unit" will be added without modification
        auxData.insert(hit.key(), val);

        ++hit;
    }

    // now write data to gui element
    QMap<QString, QString>::const_iterator mit = auxData.constBegin();
    while (mit != auxData.constEnd()) {
        ui->plainTextEdit->appendPlainText(QString("%1 = %2").arg(mit.key()).arg(mit.value()));
        ++mit;
    }
}

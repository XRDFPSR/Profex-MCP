/***************************************************************************
                          instrumentraytracer.cpp  -  description
                             -------------------
    begin                : Sat Jul 25 11:00:00 CEST 2020
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

#include "instrumentraytracermt.h"
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/parser/bgmngerparser.h"
#include <QRegularExpressionMatch>
#include <QCoreApplication>
#include <QtMath>
#include <QJSEngine>
#include <limits>

InstrumentRayTracerMT::InstrumentRayTracerMT(QObject *parent)
    : QObject(parent)
{
    settings = SettingsManager::getInstance();
    progressBar = nullptr;
    outputEditor = nullptr;
    wmin = 0.0;
    wmax = 0.0;

    initProcesses();
}

InstrumentRayTracerMT::~InstrumentRayTracerMT()
{
    if (progressBar) progressBar = nullptr;
    if (outputEditor) outputEditor = nullptr;
    if (makegeq) delete makegeq;

    while (geometLst.size()) {
        ProcessHandler *g = geometLst.takeFirst();
        if (g) delete g;
    }
}

void InstrumentRayTracerMT::initProcesses()
{
    int nProc = settings->value("bgmnProject/nThreads", QThread::idealThreadCount()).toInt();
    if (nProc <= 0) nProc = QThread::idealThreadCount();

    for (int i = 0; i < nProc; ++i)     {
        ProcessHandler *g = new ProcessHandler;

        connect(g, SIGNAL(pollOutput()), this, SLOT(pollOutput()));
        connect(g, SIGNAL(allComplete()), this, SLOT(geometComplete()));
        connect(g, SIGNAL(aborted()), this, SLOT(abortProcess()));
        geometLst.append(g);
    }

    makegeq = new ProcessHandler(this);

    connect(makegeq, SIGNAL(pollOutput()), this, SLOT(pollOutput()));
    connect(makegeq, SIGNAL(allComplete()), this, SLOT(makegeqComplete()));
    connect(makegeq, SIGNAL(aborted()), this, SLOT(abortProcess()));
}

void InstrumentRayTracerMT::setProgressBar(QProgressBar *p)
{
    progressBar = p;
}

void InstrumentRayTracerMT::setOutputEditor(QPlainTextEdit *e)
{
    outputEditor = e;
}

void InstrumentRayTracerMT::setConfigFile(const QString &cont, const QString &fn)
{
    savFileContent = cont;
    savFileName = fn;
}

void InstrumentRayTracerMT::prepareTempFiles(const QString &sfc)
{
    QString tdir = settings->getTempLocation();
    QString bn = QUuid::createUuid().toString();

    tempSavFiles.clear();
    tempSavAngles = getTempAngles(sfc);
    QString _tempFileContent = getTempSavFileContent(sfc);

    for (int i = 0; i < tempSavAngles.size(); ++i) {
        QFileInfo fi(tdir + "/" + bn + "-" + tempSavAngles.at(i) + ".sav");
        tempSavFiles.append(fi);
        BgmnFileIO::writeTextFile(fi.absoluteFilePath(), _tempFileContent);
    }

    tempTubeTails = getTubeTailFile(sfc);

    if (!tempTubeTails.isEmpty()) {
        QFileInfo fiSrc(savFileName);
        BgmnFileIO::copyFile(fiSrc.absolutePath() + "/" + tempTubeTails, tdir + "/" + tempTubeTails);
    }
}

QStringList InstrumentRayTracerMT::getTempAngles(const QString &sfc)
{
    QStringList oct = sfc.split("\n");
    QStringList ang;
    static QRegularExpression rx("^zweiTheta\\[\\d+\\]=(\\d+\\.?\\d*).*$");

    for (int i = 0; i < oct.size(); ++i) {
        QRegularExpressionMatch rm = rx.match(oct.at(i));
        if (rm.hasMatch()) ang.append(rm.captured(1));
    }

    return ang;
}

QString InstrumentRayTracerMT::getTempSavFileContent(const QString &sfc)
{
    QStringList oct = sfc.split("\n");
    QStringList tct;
    static QRegularExpression rx("^zweiTheta\\[\\d+\\]=\\d+\\.?\\d*.*$");

    for (int i = 0; i < oct.size(); ++i) {
        if (oct.at(i).left(8) == "VERZERR=") continue;
        if (oct.at(i).contains(rx))          continue;
        tct.append(oct.at(i));
    }

    return tct.join("\n");
}

QString InstrumentRayTracerMT::getTubeTailFile(const QString &sfc)
{
    static QRegularExpression rx("TubeTails=(\\S+)");
    QRegularExpressionMatch rm = rx.match(sfc);
    QString s;

    if (rm.hasMatch()) {
        s = rm.captured(1);
    }

    return s;
}

void InstrumentRayTracerMT::mergeTempGerFiles(const QFileInfoList &tgf, const QString &)
{
    QStringList out;
    QMap<double, QStringList> pks;

    for (int i = 0; i < tgf.size(); ++i) {
        BgmnGerParser gparser(tgf.at(i).absoluteFilePath());
        if (out.isEmpty()) out = gparser.getHeader();

        QMap<double, QStringList> tpks = gparser.getPeaks();
        QMapIterator<double, QStringList> it(tpks);

        while (it.hasNext()) {
            it.next();
            pks[it.key()] = it.value();
        }
    }

    QMapIterator<double, QStringList> it(pks);

    while (it.hasNext()) {
        it.next();
        out.append(it.value());
    }

    QString outFile(getGerName());
    BgmnFileIO::writeTextFile(outFile, out.join("\n"));
}

void InstrumentRayTracerMT::removeTempFiles(const QFileInfoList &fil)
{
    for (int i = 0; i < fil.size(); ++i) {
        QFileInfo fi(fil.at(i));
        QFile::remove(fi.absoluteFilePath());
        QFile::remove(fi.absolutePath() + "/" + fi.completeBaseName() + ".sav");
    }

    QString ttt = settings->getTempLocation() + "/" + tempTubeTails;
    if (QFile::exists(ttt)) QFile::remove(ttt);
}

void InstrumentRayTracerMT::runProcess()
{
    if (savFileContent.isEmpty() || savFileName.isEmpty()) {
        outputEditor->setPlainText(tr("No sav file available. Exiting."));
        return;
    }

    geometExec = settings->value("bgmnProject/geometExec", "").toString();
    makeGeqExec = settings->value("bgmnProject/makegeqExec", "").toString();

    if (!QFile::exists(geometExec) || !QFile::exists(makeGeqExec)) {
        if (!QFile::exists(geometExec)) qDebug()
                << QString("InstrumentRayTracerMT::runProcess(): geomet executable not found at: %1")
                   .arg(geometExec);
        if (!QFile::exists(geometExec)) qDebug()
                << QString("InstrumentRayTracerMT::runProcess(): geomet executable not found at: %1")
                   .arg(geometExec);

        outputEditor->appendPlainText("Error running the profile calculation. Please check the configuration "
                                      "of GEOMET and MakeGEQ in the preferences.");
        outputEditor->appendPlainText("Current configuration:");
        outputEditor->appendPlainText(QString("GEOMET: %1").arg(geometExec));
        outputEditor->appendPlainText(QString("MakeGEQ: %1").arg(makeGeqExec));
        return;
    }

    abort = false;

    perfTimer.start();
    getAngularRange(wmin, wmax);
    progressBar->setMaximum(2*int(wmax - wmin));

    prepareTempFiles(savFileContent);
    completedGerFiles.clear();
    nPeaks = tempSavAngles.size();
    nCompletedPeaks = 0;

    for (int i = 0; i < geometLst.size(); ++i) {
        if (!launchGeometProcess(geometLst.at(i))) break;
    }
}

bool InstrumentRayTracerMT::launchGeometProcess(ProcessHandler *geomet)
{
    if (!tempSavAngles.size() || !tempSavFiles.size()) return false;

    QFileInfo fi = tempSavFiles.takeFirst();
    QString ang = tempSavAngles.takeFirst();
    QString fGer = fi.absolutePath() + "/" + fi.completeBaseName() + ".ger";

    QStringList args;
    args << QString("VERZERR=%1").arg(fGer);
    args << QString("zweiTheta[1]=%1").arg(ang);

    geomet->init(fi.absoluteFilePath(), geometExec, false, QString(), args);
    bool b = geomet->run();
    completedGerFiles.append(QFileInfo(fGer));

    if (b) {
        qDebug() << QString("InstrumentRayTracer::launchGeometProcess(): Process started: %1 %2 %3")
                    .arg(geomet->getExec(), geomet->getFile(), geomet->getArguments().join(" "));
    } else {
        qDebug() << QString("InstrumentRayTracer::launchGeometProcess(): Process failed: %1 %2 %3")
                    .arg(geomet->getExec(), geomet->getFile(), geomet->getArguments().join(" "));
    }

    return b;
}

void InstrumentRayTracerMT::pollOutput()
{
    QString output;

    ProcessHandler *process = qobject_cast<ProcessHandler*>(sender());
    if (!process) return;

    if (process->isRunning()) {
        output = process->readOutput().trimmed();

        if (process->getExec() == makeGeqExec) {
            int currentStep = int(wmax + getCurrentAngle(output) - 2.0 * wmin);
            progressBar->setValue(currentStep);
        }
    }

    static QRegularExpression rx("unable to write config-file (?:geomet|makegeq|GEOMET|MAKEGEQ)\\.(?:cfg|CFG)");

    output = output.replace(rx, QString());
    if (!output.isEmpty()) outputEditor->appendPlainText(output);
    qApp->processEvents();
}

void InstrumentRayTracerMT::abortProcess()
{
    abort = true;
    abortRunningGeomet();
    if (makegeq->isRunning()) makegeq->abort();

    progressBar->setValue(progressBar->maximum());
}

void InstrumentRayTracerMT::abortRunningGeomet()
{
    for (int i = 0; i < geometLst.size(); ++i) {
        if (geometLst.at(i)->isRunning()) geometLst.at(i)->abort();
    }
}

void InstrumentRayTracerMT::geometComplete()
{
    if (abort) {
        /* todo: delete the already created sav and ger files in the temporary directory */
        return;
    }

    ++nCompletedPeaks;
    double fracProgress = double(nCompletedPeaks) / double(nPeaks);
    progressBar->setValue(int(fracProgress * (wmax - wmin)));
    qApp->processEvents();

    if (tempSavAngles.size()) {
        launchGeometProcess(qobject_cast<ProcessHandler*>(sender()));
        return;
    }

    // do nothing if some processes haven't terminated yet
    for (int i = 0; i < geometLst.size(); ++i) {
        if (geometLst.at(i)->isRunning()) return;
    }

    QString tGeomet = elapsedTimeFormatted(perfTimer.elapsed());
    qDebug() << QString("InstrumentRayTracerMT::geometComplete(): Geomet completed after %1").arg(tGeomet);
    outputEditor->appendPlainText(QString("Profile raytracing (GEOMET) completed in %1").arg(tGeomet));
    perfTimer.restart();

    mergeTempGerFiles(completedGerFiles, savFileName);
    removeTempFiles(completedGerFiles);
    applyDistortionCorrection();

    makegeq->init(savFileName, makeGeqExec);

    if (makegeq->run()) {
        qDebug() << QString("InstrumentRayTracer::geometComplete(): Process started: %1 %2 %3")
                    .arg(makegeq->getExec(), makegeq->getFile(), makegeq->getArguments().join(" "));
    } else {
        qDebug() << QString("InstrumentRayTracer::geometComplete(): Process failed: %1 %2 %3")
                    .arg(makegeq->getExec(), makegeq->getFile(), makegeq->getArguments().join(" "));
    }
}

void InstrumentRayTracerMT::makegeqComplete()
{
    if (abort) return;

    QString tMakeGeq = elapsedTimeFormatted(perfTimer.elapsed());
    qDebug() << QString("InstrumentRayTracerMT::makegeqComplete(): MakeGEQ completed after %1").arg(tMakeGeq);
    outputEditor->appendPlainText(QString("Profile interpolation (MAKEGEQ) completed in %1").arg(tMakeGeq));

    progressBar->setValue(progressBar->maximum());
    emit processComplete();
}

void InstrumentRayTracerMT::getAngularRange(double &wmin, double &wmax)
{
    static QRegularExpression rxWmin("WMIN=(-?\\d+\\.?\\d*)");
    static QRegularExpression rxWmax("WMAX=(-?\\d+\\.?\\d*)");

    QRegularExpressionMatch rmWmin = rxWmin.match(savFileContent);
    QRegularExpressionMatch rmWmax = rxWmax.match(savFileContent);

    wmin = rmWmin.hasMatch() ? rmWmin.captured(1).toDouble() : 0.0;
    wmax = rmWmax.hasMatch() ? rmWmax.captured(1).toDouble() : 0.0;
}

double InstrumentRayTracerMT::getCurrentAngle(const QString &output)
{
    double d = 0.0;

    static QRegularExpression rx("^zweiTheta=(\\d+\\.\\d+)\\s+");
    QRegularExpressionMatchIterator ri = rx.globalMatch(output);
    QRegularExpressionMatch rm;

    while (ri.hasNext()) {
        rm = ri.next();

        if (rm.hasMatch()) {
            d = rm.captured(1).toDouble();
        }
    }

    return d;
}

void InstrumentRayTracerMT::applyDistortionCorrection()
{
    QString ztStretch = getSavVariable("ZTSTRETCH=(.+)", QString()).trimmed();

    if (ztStretch.isEmpty()) return;
    ztStretch = bgmnMathToJS(ztStretch);

    QString fGer(getGerName());
    QStringList gerContent = BgmnFileIO::readTextFileLines(fGer);

    static QRegularExpression rxTt("\\s*THETA=(\\d+\\.?\\d*)\\s+N=\\d+");
    static QRegularExpression rxVal("\\s*(-?\\d+\\.?\\d*)\\s+(-?\\d+\\.?\\d*)\\s+(-?\\d+\\.?\\d*)");

    QJSEngine jsEngine;
    QJSValue jsValue = jsEngine.evaluate(QString("(function(zweiTheta) {return %1;} )").arg(ztStretch));
    outputEditor->appendPlainText(QString("Scaling peak width with f(zweiTheta) = %1").arg(ztStretch));

    double ztScale = 1.0;

    for (int i = 0; i < gerContent.size(); ++i) {
        QRegularExpressionMatch rmTt = rxTt.match(gerContent.at(i));
        QRegularExpressionMatch rmVal = rxVal.match(gerContent.at(i));

        if (rmTt.hasMatch()) {
            double theta = rmTt.captured(1).toDouble();

            QJSValueList jsArgs;
            jsArgs << 2.0 * theta;

            ztScale = jsValue.call(jsArgs).toNumber();

            outputEditor->appendPlainText(QString("Stretch factor at theta=%1 is %2").arg(theta).arg(ztScale, 0, 'f', 6));

            continue;
        }

        if (rmVal.hasMatch()) {
            double g = rmVal.captured(1).toDouble();
            double e = rmVal.captured(2).toDouble();
            double q = rmVal.captured(3).toDouble();

            gerContent[i] = QString("%1 %2 %3").arg(g, 0, 'f', 5).arg(e * ztScale, 0, 'f', 7).arg(q * ztScale, 0, 'f', 8);
        }
    }

    BgmnFileIO::writeTextFile(fGer, gerContent.join("\n"));
}

QString InstrumentRayTracerMT::getGerName()
{
    static QRegularExpression rxGer("VERZERR=(.+\\.(ger|GER))");
    QRegularExpressionMatch rmGer = rxGer.match(savFileContent);

    if (!rmGer.hasMatch()) return QString();

    QFileInfo fiSav(savFileName);
    QFileInfo fiGer(fiSav.absolutePath() + "/" + rmGer.captured(1));
    qDebug() << QString("EditInstrumentDialog::getGerName(): returning file %1").arg(fiGer.absoluteFilePath());
    return fiGer.absoluteFilePath();
}

QString InstrumentRayTracerMT::getSavVariable(const QString &s, const QString &def)
{
    QRegularExpression rx(s);
    QRegularExpressionMatch rm = rx.match(savFileContent);

    if (rm.hasMatch()) return rm.captured(1);
    return def;
}

QString InstrumentRayTracerMT::bgmnMathToJS(const QString &istr)
{
    QString ostr(istr);

    QStringList bgPatterns;
    QStringList jsPatterns;

    bgPatterns << "abs" << "sqrt" << "sin";
    jsPatterns << "Math.abs" << "Math.sqrt" << "Math.sin";

    bgPatterns << "cos" << "tan" << "asin" << "acos";
    jsPatterns << "Math.cos" << "Math.tan" << "Math.asin" << "Math.acos";

    bgPatterns << "atan" << "log" << "exp" << "power";
    jsPatterns << "Math.atan" << "Math.log" << "Math.exp" << "Math.pow";

    bgPatterns << "min" << "max" << "pi";
    jsPatterns << "Math.min" << "Math.max" << "Math.PI";

    for (int i = 0; i < qMin(bgPatterns.size(), jsPatterns.size()); ++i) {
        QRegularExpression rx(QString("(?<!Math\\.)%1(?=[^a-zA-Z0-9])").arg(bgPatterns.at(i)));
        ostr.replace(rx, jsPatterns.at(i));
    }

    return ostr;
}

QString InstrumentRayTracerMT::elapsedTimeFormatted(int mstotal)
{
    QTime t(0, 0, 0, 0);
    t = t.addMSecs(mstotal);
    return t.toString("hh:mm:ss.zzz");
}

bool InstrumentRayTracerMT::isRunning()
{
    for (int i = 0; i < geometLst.size(); ++i) {
        if (geometLst.at(i)->isRunning()) return true;
    }

    return makegeq->isRunning();
}

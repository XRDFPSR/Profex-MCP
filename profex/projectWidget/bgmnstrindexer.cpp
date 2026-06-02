/***************************************************************************
                          bgmnstrindexer.cpp  -  description
                             -------------------
    begin                : Sun Oct 31 13:14:00 CEST 2021
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

#include <QDir>
#include "bgmnstrindexer.h"
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/functions.h"
#include "../libXrdIO/parser/bgmnparparser.h"
#include "../libXrdIO/parser/bgmnstrparser.h"

BgmnStrIndexer::BgmnStrIndexer(QObject *parent)
    : QObject(parent)
{
    settings = SettingsManager::getInstance();
    bgmnIndexHandler = nullptr;
}

BgmnStrIndexer::~BgmnStrIndexer()
{
    if (bgmnIndexHandler) delete bgmnIndexHandler;
}

void BgmnStrIndexer::setHklPhase(const HklPhaseData &hklPd)
{
    QFileInfo fiStr(hklPd.file());

    indexedStructure.reset();
    indexedStructure.setHklPhase(hklPd);
    indexedStructure.setStrSourceFile(hklPd.file());
    indexedStructure.setStrFile(QDir::fromNativeSeparators(settings->getTempLocation() + "/" + fiStr.completeBaseName() + ".str"));
    indexedStructure.setParFile(QDir::fromNativeSeparators(settings->getTempLocation() + "/" + fiStr.completeBaseName() + ".par"));
    indexedStructure.setLstFile(QDir::fromNativeSeparators(settings->getTempLocation() + "/" + fiStr.completeBaseName() + ".lst"));
    indexedStructure.setRemove(true);
}

void BgmnStrIndexer::runStrIndexing()
{
    if (!BgmnFileIO::copyFile(indexedStructure.strSourceFile().absoluteFilePath(), indexedStructure.strFile().absoluteFilePath())) {
        emit signalIndexingComplete();
    } else {
        runIndexing();
    }
}

void BgmnStrIndexer::runIndexing()
{
    if (settings->verboseLevel() > 1) qDebug() << QString("BgmnStrIndexer::runIndexing(): Indexing file: %1").arg(indexedStructure.strFile().absoluteFilePath());
    double wl = 0.1 * settings->defaultWavelength();
    double dmax = settings->value("config/hklUpperRangeD", 0.1540598).toDouble();
    double wmax = global::Functions::dToTwoTheta(dmax, wl);

    lastLogMessage = QString();

    if (!bgmnIndexHandler) {
        bgmnIndexHandler = new BgmnHandler(QUuid::createUuid());
        connect(bgmnIndexHandler, SIGNAL(allComplete(QUuid,global::RefinementStatus)), this, SLOT(indexProcessComplete()));
        connect(bgmnIndexHandler, SIGNAL(pollOutput()), this, SLOT(pollProcessOutput()));
    }

    if (bgmnIndexHandler->init()) {
        QStringList args;

        args << indexedStructure.strFile().completeBaseName();
        args << "SAVE=N";
        args << QString("SYNCHROTRON=%1").arg(wl);
        args << QString("NTHREADS=%1").arg(QThread::idealThreadCount());
        args << QString("WMAX=%1").arg(wmax, 0, 'f', 2);
        args << QString("STRUC[1]=%1").arg(indexedStructure.strFile().absoluteFilePath());
        args << QString("OUTPUT=%1").arg(indexedStructure.parFile().absoluteFilePath());
        args << QString("LIST=%1").arg(indexedStructure.lstFile().absoluteFilePath());

        bgmnIndexHandler->run(settings->getTempLocation(), args);
    } else {
        if (settings->verboseLevel() > 0) qDebug() << QString("BgmnStrIndexer::runIndexing(): Error, could not initialize bgmnHandler, exiting");
        indexProcessComplete();
    }
}

void BgmnStrIndexer::indexProcessComplete()
{
    pollProcessOutput();

    if (!indexedStructure.parFile().exists()) {
        indexedStructure.reset();
        if (settings->verboseLevel() > 0) qDebug() << QString("BgmnStrIndexer::indexProcessComplete(): Error: *.par file not created");
    } else {
        QVector<Hkl> vec = parseParFile(indexedStructure.parFile().absoluteFilePath());

        if (vec.size()) {
            indexedStructure.hklPhase().setComment(getComment(indexedStructure.hklPhase().file()));
            indexedStructure.hklPhase().setPhase(vec.first().phase());
            indexedStructure.hklPhase().setColor(vec.first().color().name());
            indexedStructure.hklPhase().setHklData(vec);
            indexedStructure.hklPhase().setDensity(parseLstFile(indexedStructure.lstFile().absoluteFilePath()).value("XrayDensity", -1.0).toDouble());

            BgmnStrParser strParser(indexedStructure.strFile().absoluteFilePath());
            indexedStructure.hklPhase().setElements(strParser.getElements());
        } else {
            indexedStructure.reset();
            if (settings->verboseLevel() > 0) qDebug() << QString("BgmnStrIndexer::indexProcessComplete(): Error: no hkl lines found");
        }
    }

    if (indexedStructure.doRemove()) {
        if (indexedStructure.strFile().exists()) QFile::remove(indexedStructure.strFile().absoluteFilePath());
        if (indexedStructure.lstFile().exists()) QFile::remove(indexedStructure.lstFile().absoluteFilePath());
        if (indexedStructure.parFile().exists()) QFile::remove(indexedStructure.parFile().absoluteFilePath());
    }

    emit signalIndexingComplete();
}

void BgmnStrIndexer::pollProcessOutput()
{
    QString output("Indexing not requested");

    if (bgmnIndexHandler) {
        output = bgmnIndexHandler->readOutput().trimmed();
        lastLogMessage = output;
    }

    emit signalProcessOutput(output);
}

void BgmnStrIndexer::cancel()
{
    if (bgmnIndexHandler) {
        if (bgmnIndexHandler->isRunning()) {
            bgmnIndexHandler->abort();
        }
    }

    indexedStructure.reset();
    emit signalIndexingComplete();
}

QVector<Hkl> BgmnStrIndexer::parseParFile(const QString &parFile)
{
    bool b;
    BgmnParParser pparser(parFile, b);

    if (!b) {
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnStrIndexer::parseParFile: BgmnParParser reported an error parsing file %1").arg(parFile);
        return QVector<Hkl>();
    }

    QVector<Hkl> vec = pparser.getReflections(); // only of first phase?
    if (settings->verboseLevel() > 0) qDebug() << QString("BgmnStrIndexer::parseParFile: Read %1 hkl indices from %2").arg(vec.size()).arg(parFile);

    return vec;
}

QMap<QString, QVariant> BgmnStrIndexer::parseLstFile(const QString &lstFile)
{
    QMap<QString, QVariant> data;

    QString lstFileData(BgmnFileIO::readTextFile(lstFile));

    if (!lstFileData.isEmpty()) {
        static QRegularExpression rxDensity("XrayDensity=(\\d+\\.?\\d*)");
        QRegularExpressionMatch rmDensity = rxDensity.match(lstFileData);
        if (rmDensity.hasMatch()) {
            data["XrayDensity"] = rmDensity.captured(1).toDouble();
        } else {
            data["XrayDensity"] = -1.0;
        }
    }

    return data;
}

QString BgmnStrIndexer::getComment(const QString &file)
{
    static QRegularExpression rx("^PHASE=([^/]+)//(.*)$");

    QFile f(file);

    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);

        while (!in.atEnd()) {
            QRegularExpressionMatch rm = rx.match(in.readLine());

            if (rm.hasMatch()) {
                return rm.captured(2).trimmed();
            }
        }
    }

    f.close();
    return QString("");
}

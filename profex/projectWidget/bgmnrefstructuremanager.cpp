/***************************************************************************
                          bgmnrefstructuremanager.cpp  -  description
                             -------------------
    begin                : Sat Feb 27 13:08:00 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#include <QThread>
#include <QStandardPaths>
#include "bgmnrefstructuremanager.h"
#include "bgmnbackendconfig.h"
#include "../../libXrdIO/parser/bgmnparparser.h"
#include "../../libXrdIO/parser/bgmngerparser.h"
#include "../../libXrdIO/parser/bgmnstrparser.h"
#include "../../libXrdIO/bgmnfileio.h"
#include "../../libXrdIO/functions.h"
#include <QMessageBox>

bool BgmnRefStructureManager::instanceFlag = false;
BgmnRefStructureManager* BgmnRefStructureManager::instance = nullptr;

BgmnRefStructureManager* BgmnRefStructureManager::getInstance()
{
    if (!instanceFlag) {
        instance = new BgmnRefStructureManager(nullptr);
        instanceFlag = true;
        return instance;
    } else {
        return instance;
    }
}

BgmnRefStructureManager::BgmnRefStructureManager(QObject *parent) :
    QObject(parent)
{
    settings = SettingsManager::getInstance();
    projectID = "BGMN";
    hklBufferDb = QSqlDatabase::addDatabase("QSQLITE", projectID);
    strIndexer = new BgmnStrIndexer;
    connect(strIndexer, SIGNAL(signalIndexingComplete()), this, SLOT(indexProcessComplete()));

    bufferedFileCountAll = -1;
    bufferedFavFileCountAll = -1;
    bufferedFileCountIndexed = -1;
    bufferedFavFileCountIndexed = -1;
}

BgmnRefStructureManager::~BgmnRefStructureManager()
{
    if (hklBufferDb.isOpen()) hklBufferDb.close();
    if (strIndexer)       delete strIndexer;
}

void BgmnRefStructureManager::destroy()
{
    if (instance) delete instance;
}

/*************************************************
 *            public functions                   *
 *************************************************/

void BgmnRefStructureManager::init()
{
    QString hklBufferFile("hklbufferV5.db3");
    QString hklBufferPath(settings->getAppDataLocation());
    QDir bPath(hklBufferPath);

    if (!bPath.exists()) {
        bPath.mkpath(".");
    }

    QString hklBufferFilePath = hklBufferPath + "/" + hklBufferFile;
    qDebug() << QString("BgmnRefStructureManager::init(): Setting reference database to %1").arg(hklBufferFilePath);

    // immediately write back the location, in case it was changed. So other calls to "config/hklBufferFile" get the new location
    settings->setValue("config/hklBufferFile", hklBufferPath);
    hklBufferDb.setDatabaseName(hklBufferFilePath);

    if (openDb("init")) {
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::init(): DB contains %1 phases").arg(countAllPhases());
    } else {
        qDebug() << QString("BgmnRefStructureManager::init(): Could not open reference structure database");
    }
}

void BgmnRefStructureManager::sync()
{
    if (hklBufferDb.isOpen()) hklBufferDb.close();
    init();
}

void BgmnRefStructureManager::closeDb()
{
    if (hklBufferDb.isOpen()) hklBufferDb.close();
}

QString BgmnRefStructureManager::getHklBufferFileName()
{
    return hklBufferDb.databaseName();
}

void BgmnRefStructureManager::update()
{
    if (settings->verboseLevel() > 2) qDebug() << QString("BgmnRefStructureManager::update(): invoked");

    QMap<QString, HklPhaseData> toRemove;
    QMap<QString, HklPhaseData> toIndex;
    QMap<QString, HklPhaseData> toInsert;

    diffFilesWithDb(toRemove, toIndex, toInsert);

    filesToRemove = toRemove.values();
    filesToIndex = toIndex.values();
    filesToInsert = toInsert.values();

    errorFiles.clear();
    cancelled = false;

    removeObsoleteEntries();
    insertFiles();
    indexFiles();
}

void BgmnRefStructureManager::clearDbBuffers()
{
    bufferedFileNames.clear();
    bufferedFavFileNames.clear();
    bufferedDirFileCount.clear();
    bufferedDirFileCountIndexed.clear();
    bufferedFileCountAll = -1;
    bufferedFileCountIndexed = -1;
    bufferedFavFileCountAll = -1;
    bufferedFavFileCountIndexed = -1;
}

void BgmnRefStructureManager::updateComplete()
{
    emit signalUpdateComplete(errorFiles);

    if (openDb("cleanUp")) {
        QSqlQuery query(hklBufferDb);
        query.exec("VACUUM");
    }

    if (cancelled && (settings->verboseLevel() > 1)) {
        qDebug() << QStringLiteral("BgmnRefStructureManager::indexFiles(): Indexing was cancelled. Exiting");
    }
}

int BgmnRefStructureManager::countAllPhases()
{
    if (bufferedFileCountAll < 0) {
        if (!openDb("countPhases")) {
            if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::countAllPhases(): Database is closed. Returning 0.");
            return 0;
        }

        QSqlQuery query("SELECT (COUNT(*)) FROM referenceStructures", hklBufferDb);

        if (!query.first()) return 0;
        bufferedFileCountAll = query.value(0).toInt();
    }

    return bufferedFileCountAll;
}

int BgmnRefStructureManager::countAllPhasesIndexed()
{
    if (bufferedFileCountIndexed < 0) {
        if (!openDb("countPhases")) {
            if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::countAllPhasesIndexed(): Database is closed. Returning 0.");
            return 0;
        }

        QSqlQuery queryIdx("SELECT (COUNT(*)) FROM referenceStructures WHERE LENGTH(data) != 0", hklBufferDb);

        if (!queryIdx.first()) return 0;
        bufferedFileCountIndexed = queryIdx.value(0).toInt();
    }

    return bufferedFileCountIndexed;
}

int BgmnRefStructureManager::countFavPhases()
{
    if (bufferedFavFileCountAll < 0) {
        if (!openDb("countPhases")) {
            if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::countFavPhases(): Database is closed. Returning 0.");
            return 0;
        }

        QSqlQuery query("SELECT (COUNT(*)) FROM referenceStructures WHERE favorite = 1", hklBufferDb);

        if (!query.first()) return 0;
        bufferedFavFileCountAll = query.value(0).toInt();
    }

    return bufferedFavFileCountAll;
}

int BgmnRefStructureManager::countFavPhasesIndexed()
{
    if (bufferedFavFileCountIndexed < 0) {
        if (!openDb("countPhases")) {
            if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::countFavPhasesIndexed(): Database is closed. Returning 0.");
            return 0;
        }

        QSqlQuery query("SELECT (COUNT(*)) FROM referenceStructures WHERE favorite = 1 AND LENGTH(data) != 0", hklBufferDb);

        if (!query.first()) return 0;
        bufferedFavFileCountIndexed = query.value(0).toInt();
    }

    return bufferedFavFileCountIndexed;
}

int BgmnRefStructureManager::countPhasesInDirectory(const QString &dir)
{
    if (!bufferedDirFileCount.contains(dir)) {
        if (!openDb("countPhases")) {
            if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::countPhasesInDirectory(): Database is closed. Returning 0.");
            return 0;
        }

        QFileInfo fi(dir + "/test");

        // we must use value binding, because a direct query causes errors with dir separators
        QSqlQuery query(hklBufferDb);
        query.prepare("SELECT (COUNT(*)) FROM referenceStructures WHERE sourcedir = :sourcedir");
        query.bindValue(":sourcedir", fi.absolutePath());
        query.exec();

        if (query.first()) bufferedDirFileCount.insert(dir, query.value(0).toInt());
    }

    return bufferedDirFileCount.value(dir, 0);
}

int BgmnRefStructureManager::countPhasesInDirectoryIndexed(const QString &dir)
{
    if (!bufferedDirFileCountIndexed.contains(dir)) {
        if (!openDb("countPhases")) {
            if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::countPhasesInDirectoryIndexed(): Database is closed. Returning 0.");
            return 0;
        }

        QFileInfo fi(dir + "/test");

        // we must use value binding, because a direct query causes errors with dir separators
        QSqlQuery query(hklBufferDb);
        query.prepare("SELECT (COUNT(*)) FROM referenceStructures WHERE sourcedir = :sourcedir AND LENGTH(data) != 0");
        query.bindValue(":sourcedir", fi.absolutePath());
        query.exec();

        if (query.first()) bufferedDirFileCountIndexed.insert(dir, query.value(0).toInt());
    }

    return bufferedDirFileCountIndexed.value(dir, 0);
}

HklPhaseData BgmnRefStructureManager::queryToPhase(const QSqlQuery &q)
{
    HklPhaseData data;
    if (!q.isValid()) return data;

    int idxFile    = q.record().indexOf("file");
    int idxComment = q.record().indexOf("comment");
    int idxSrc     = q.record().indexOf("sourcedir");
    int idxMd5     = q.record().indexOf("md5hash");
    int idxPhase   = q.record().indexOf("phase");
    int idxCol     = q.record().indexOf("color");
    int idxFav     = q.record().indexOf("favorite");
    int idxStrong  = q.record().indexOf("strongest");
    int idxStrong2 = q.record().indexOf("strongest2");
    int idxStrong3 = q.record().indexOf("strongest3");
    int idxData    = q.record().indexOf("data");
    int idxXyData  = q.record().indexOf("xyData");

    data.setFile(idxFile         >= 0 ? q.value(idxFile).toString()      : QString());
    data.setMd5Hash(idxMd5       >= 0 ? q.value(idxMd5).toString()       : QString());
    data.setSourceDir(idxSrc     >= 0 ? q.value(idxSrc).toString()       : QString());
    data.setComment(idxComment   >= 0 ? q.value(idxComment).toString()   : QString());
    data.setPhase(idxPhase       >= 0 ? q.value(idxPhase).toString()     : QString());
    data.setColor(idxCol         >= 0 ? q.value(idxCol).toString()       : QString());
    data.setFavorite(idxFav      >= 0 ? q.value(idxFav).toInt()          : 0);
    data.setStrongest(idxStrong  >= 0 ? q.value(idxStrong).toDouble()    : 0.0);
    data.setStrongest2(idxStrong >= 0 ? q.value(idxStrong2).toDouble()   : 0.0);
    data.setStrongest3(idxStrong >= 0 ? q.value(idxStrong3).toDouble()   : 0.0);
    data.setHklData(idxData      >= 0 ? q.value(idxData).toByteArray()   : QByteArray());
    data.setXyData(idxXyData     >= 0 ? q.value(idxXyData).toByteArray() : QByteArray());

    return data;
}

QList<HklPhaseData> BgmnRefStructureManager::getAllPhases(const QStringList &elAll, const QStringList &elOne, const QStringList &elNone)
{
    QList<HklPhaseData> phases;
    if (!openDb("getAllPhases")) return phases;

    QString conditions = getConditionsString(elAll, elOne, elNone);
    QString queryStr("SELECT * FROM referenceStructures");
    if (!conditions.isEmpty()) queryStr += " WHERE " + conditions;

    QSqlQuery query(queryStr, hklBufferDb);

    while (query.next()) {
        phases << queryToPhase(query);
    }

    return phases;
}

QList<HklPhaseData> BgmnRefStructureManager::getSubDirPhases(const QString &d, const QStringList &elAll, const QStringList &elOne, const QStringList &elNone)
{
    QList<HklPhaseData> phases;
    if (!openDb("getSubDirPhases")) return phases;

    // we use a QFileInfo::absolutePath() of a fake file in directory d
    // to make sure that the format of the path is exactly the same as
    // used to insert the database entry.
    QFileInfo fi(d + "/test");

    QString conditions = getConditionsString(elAll, elOne, elNone);
    QString queryStr("SELECT * FROM referenceStructures WHERE sourcedir = :dir");
    if (!conditions.isEmpty()) queryStr += " AND " + conditions;

    QSqlQuery query(hklBufferDb);
    query.prepare(queryStr);
    query.bindValue(":dir", fi.absolutePath());
    query.exec();

    while (query.next()) {
        phases << queryToPhase(query);
    }

    return phases;
}

QList<HklPhaseData> BgmnRefStructureManager::getFavPhases(const QStringList &elAll, const QStringList &elOne, const QStringList &elNone)
{
    QList<HklPhaseData> phases;
    if (!openDb("getFavPhases")) return phases;

    QString conditions = getConditionsString(elAll, elOne, elNone);
    QString queryStr("SELECT * FROM referenceStructures WHERE favorite = 1");
    if (!conditions.isEmpty()) queryStr += " AND " + conditions;

    QSqlQuery query(queryStr, hklBufferDb);

    while (query.next()) {
        phases << queryToPhase(query);
    }

    return phases;
}

QSet<QString> BgmnRefStructureManager::getAllFileNames(const QStringList &elAll, const QStringList &elOne, const QStringList &elNone)
{
    QSet<QString> names;
    if (!openDb("getFavPhases")) return names;

    QString conditions = getConditionsString(elAll, elOne, elNone);
    QString queryStr("SELECT file FROM referenceStructures");
    if (!conditions.isEmpty()) queryStr += " WHERE " + conditions;

    QSqlQuery query(queryStr, hklBufferDb);
    int i = query.record().indexOf("file");

    while (query.next()) {
        names.insert(query.value(i).toString());
    }

    return names;
}

QSet<QString> BgmnRefStructureManager::getFavFileNames(const QStringList &elAll, const QStringList &elOne, const QStringList &elNone)
{
    QSet<QString> names;
    if (!openDb("getFavPhases")) return names;

    QString conditions = getConditionsString(elAll, elOne, elNone);
    QString queryStr("SELECT file FROM referenceStructures WHERE favorite = 1");
    if (!conditions.isEmpty()) queryStr += " AND " + conditions;

    QSqlQuery query(queryStr, hklBufferDb);
    int i = query.record().indexOf("file");

    while (query.next()) {
        names.insert(query.value(i).toString());
    }

    return names;
}

QVector<Hkl> BgmnRefStructureManager::getReferenceLines(const QString &s)
{
    if (!openDb("getReferenceStructure - open ")) return QVector<Hkl>();

    QSqlQuery query(hklBufferDb);
    query.prepare("SELECT phase, color, data FROM referenceStructures WHERE file = :file");
    query.bindValue(":file", s);

    if (!query.exec()) {
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): Query Error:  %1").arg(query.lastError().text());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): DB Error:     %1").arg(query.lastError().databaseText());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): Driver Error: %1").arg(query.lastError().driverText());
        dumpError("getReferenceStructure - exec ");
        return QVector<Hkl>();
    }

    if (!query.first()) {
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): Query Error:  %1").arg(query.lastError().text());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): DB Error:     %1").arg(query.lastError().databaseText());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): Driver Error: %1").arg(query.lastError().driverText());
        dumpError("getReferenceStructure - valid ");
        return QVector<Hkl>();
    }

    HklPhaseData hklData;

    int idxPhase = query.record().indexOf("phase");
    int idxColor = query.record().indexOf("color");
    int idxData  = query.record().indexOf("data");

    hklData.setPhase(query.value(idxPhase).toString());
    hklData.setColor(query.value(idxColor).toString());
    hklData.setHklData(query.value(idxData).toByteArray());

    return hklData.hklData();
}

QVector<Hkl> BgmnRefStructureManager::getReferenceLines(const QFileInfo &fi)
{
    return getReferenceLines(fi.absoluteFilePath());
}

QVector<QVector<double> > BgmnRefStructureManager::getReferencePattern(const QString &s)
{
    QVector<QVector<double> > vec(2, QVector<double>());
    if (!openDb("getReferencePattern - open ")) return vec;

    QSqlQuery query(hklBufferDb);
    query.prepare("SELECT xyData FROM referenceStructures WHERE file = :file");
    query.bindValue(":file", s);

    if (!query.exec()) {
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): Query Error:  %1").arg(query.lastError().text());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): DB Error:     %1").arg(query.lastError().databaseText());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): Driver Error: %1").arg(query.lastError().driverText());
        dumpError("getReferencePattern - exec ");
        return vec;
    }

    if (!query.first()) {
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): Query Error:  %1").arg(query.lastError().text());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): DB Error:     %1").arg(query.lastError().databaseText());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::getReferencePattern(): Driver Error: %1").arg(query.lastError().driverText());
        dumpError("getReferencePattern - valid ");
        return vec;
    }

    HklPhaseData hklData;

    int idxXyData  = query.record().indexOf("xyData");
    hklData.setXyData(query.value(idxXyData).toByteArray());

    return hklData.xyData();
}

QVector<QVector<double> > BgmnRefStructureManager::getReferencePattern(const QFileInfo &fi)
{
    return getReferencePattern(fi.absoluteFilePath());
}

Scan BgmnRefStructureManager::getReferenceScan(const QFileInfo &fi)
{
    return getReferenceScan(fi.absoluteFilePath());
}

Scan BgmnRefStructureManager::getReferenceScan(const QString &s)
{
    Scan scan;
    QVector<QVector<double> > xyData = getReferencePattern(s);

    scan.pDataHkl() = getReferenceLines(s);
    scan.pDataAngle() = xyData.at(0);
    scan.pDataIntensity() = xyData.at(1);

    return scan;
}

void BgmnRefStructureManager::referencePosClicked(double d, QMultiMap<double, PeakFile> &nearestMatches, const QString &repository, bool fav, int near)
{
    nearestMatches = nearestPeakFiles(d, repository, fav, near);
}

void BgmnRefStructureManager::cancel()
{
    cancelled = true;
    strIndexer->cancel();
}

void BgmnRefStructureManager::skipCurrent()
{
    strIndexer->cancel();
}

QStringList BgmnRefStructureManager::getAllSourceDirs(bool favs)
{
    QStringList dbSourceDirs;
    if (!openDb("getAllSourceDirs")) return dbSourceDirs;

    QString favstr;
    if (favs) favstr = " WHERE favorite = 1";

    QSqlQuery query(QString("SELECT sourcedir FROM referenceStructures%1").arg(favstr), hklBufferDb);
    int sd = query.record().indexOf("sourcedir");

    while (query.next()) {
        if (!dbSourceDirs.contains(query.value(sd).toString())) {
            dbSourceDirs.append(query.value(sd).toString());
        }
    }

    return dbSourceDirs;
}

QStringList BgmnRefStructureManager::getIndexedFileNames(const QString &src)
{
    if (bufferedFileNames.isEmpty()) {
        if (!openDb("getIndexedFileNames")) return QStringList();

        QSqlQuery query(hklBufferDb);
        query.prepare("SELECT file, sourcedir FROM referenceStructures WHERE LENGTH(data) != 0");

        if (!query.exec()) {
            if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::setXyData(): Query Error:  %1").arg(query.lastError().text());
            if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::setXyData(): DB Error:     %1").arg(query.lastError().databaseText());
            if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::setXyData(): Driver Error: %1").arg(query.lastError().driverText());
            dumpError("getReferencePattern - exec ");
            return QStringList();
        }

        int fidx = query.record().indexOf("file");
        int sidx = query.record().indexOf("sourcedir");

        while (query.next()) {
            bufferedFileNames[query.value(sidx).toString()].append(query.value(fidx).toString());
        }
    }

    if (src.isEmpty()) {
        QStringList allFiles;
        QMapIterator<QString, QStringList> it(bufferedFileNames);

        while (it.hasNext()) {
            it.next();
            allFiles.append(it.value());
        }

        return allFiles;
    }

    return bufferedFileNames.value(src, QStringList());
}

QStringList BgmnRefStructureManager::getIndexedFavoriteFileNames()
{
    if (bufferedFavFileNames.isEmpty()) {
        if (!openDb("getIndexedFavoriteFileNames")) return QStringList();

        QSqlQuery query("SELECT file FROM referenceStructures WHERE favorite = 1", hklBufferDb);
        int fidx = query.record().indexOf("file");

        while (query.next()) {
            bufferedFavFileNames.append(query.value(fidx).toString());
        }
    }

    return bufferedFavFileNames;
}

void BgmnRefStructureManager::setFavorites(const QStringList &favs)
{
    if (!openDb("setFavorites")) return;

    bufferedFavFileNames.clear();
    bufferedFavFileCountAll = -1;
    bufferedFavFileCountIndexed = -1;

    QStringList uFavs = favoritesToUnset(favs);
    if (uFavs.size()) updateFavorites(uFavs, 0);
    if (favs.size())  updateFavorites(favs, 1);
}

void BgmnRefStructureManager::setXyData(const QString &file, const Scan &scan)
{
    if (settings->verboseLevel() > 2) qDebug() << QString("BgmnRefStructureManager::setXyData(): Updating xy data for file %1").arg(file);
    if (!openDb("setXyData - open ")) return;

    QSqlQuery query(hklBufferDb);
    query.prepare("SELECT * FROM referenceStructures WHERE file = :file");
    query.bindValue(":file", file);

    if (!query.exec()) {
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::setXyData(): Query Error:  %1").arg(query.lastError().text());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::setXyData(): DB Error:     %1").arg(query.lastError().databaseText());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::setXyData(): Driver Error: %1").arg(query.lastError().driverText());
        dumpError("getReferencePattern - exec ");
        return;
    }

    if (!query.first()) {
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::setXyData(): Could not find file in DB: %1").arg(file);
        return;
    }

    HklPhaseData phaseData = queryToPhase(query);
    if (settings->verboseLevel() > 2) qDebug() << QString("BgmnRefStructureManager::setXyData(): Retrieved DB query for file %1").arg(phaseData.file());
    QVector<QVector<double> > vec(2, QVector<double>());
    vec[0] = scan.pDataAngle();
    vec[1] = scan.pDataIntensity();
    phaseData.setXyData(vec);
    insertReferenceStructure(phaseData);
}

/*************************************************
 *           private functions                   *
 *************************************************/

/*
 * remove DB entries that:
 * - exist in the DB but not on HD
 * - exist in DB and on HD, but outside of current repo directories
 * - have non-matching MD5 sums
 * - have hkl data but should not be indexed
 * - do not have hkl data but should be indexed
 */
void BgmnRefStructureManager::diffFilesWithDb(QMap<QString, HklPhaseData> &toRemove, QMap<QString, HklPhaseData> &toIndex, QMap<QString, HklPhaseData> &toInsert)
{
    QMap<QString, HklPhaseData> filesDb(filesInDatabase());
    QMap<QString, HklPhaseData> filesHdIdx;
    QMap<QString, HklPhaseData> filesHdNonIdx;
    filesInDirectories(filesHdIdx, filesHdNonIdx);

    QMapIterator<QString, HklPhaseData> itHdIdx(filesHdIdx);
    QMapIterator<QString, HklPhaseData> itHdNonIdx(filesHdNonIdx);
    QMapIterator<QString, HklPhaseData> itDb(filesDb);

    bool dbg = settings->verboseLevel() > 1;
    if (dbg) qDebug() << QString("BgmnRefStructureManager::diffFilesWithDb(): Modifications to hkl database:");

    emit signalTotalFiles(filesHdIdx.size() + filesHdNonIdx.size() + filesDb.size());
    emit signalCurrentFile(0);
    emit signalCurrentText(QString("Checking database entries..."));
    int n = 0;

    while (itHdIdx.hasNext()) {
        itHdIdx.next();

        if (filesDb.contains(itHdIdx.key())) {
            if (filesDb.value(itHdIdx.key()).hklBinary().size() == 0) {
                toIndex.insert(itHdIdx.key(), itHdIdx.value());
                if (dbg) qDebug() << QString("    Re-indexing because hkl data is empty: %1").arg(itHdIdx.value().file());
            } else {
                QString md5hashHd = itHdIdx.value().md5hash();
                QString md5hashDb = filesDb.value(itHdIdx.key()).md5hash();

                if (md5hashDb != md5hashHd) {
                    toIndex.insert(itHdIdx.key(), itHdIdx.value());
                    if (dbg) qDebug() << QString("    Re-indexing because of MD5 checksum mismatch: %1").arg(itHdIdx.value().file());
                }
            }
        } else {
            toIndex.insert(itHdIdx.key(), itHdIdx.value());
            if (dbg) qDebug() << QString("    Indexing new file: %1").arg(itHdIdx.value().file());
        }

        ++n;
        emit signalCurrentFile(n);
        qApp->processEvents();
    }

    while (itHdNonIdx.hasNext()) {
        itHdNonIdx.next();

        if (filesDb.contains(itHdNonIdx.key())) {
            if (filesDb.value(itHdNonIdx.key()).hklBinary().size() > 0) {
                toInsert.insert(itHdNonIdx.key(), itHdNonIdx.value());
                if (dbg) qDebug() << QString("    Re-inserting as unindexed file: %1").arg(itHdNonIdx.value().file());
            } else {
                QString md5hashHd = itHdNonIdx.value().md5hash();
                QString md5hashDb = filesDb.value(itHdNonIdx.key()).md5hash();

                if (md5hashDb != md5hashHd) {
                    toInsert.insert(itHdNonIdx.key(), itHdNonIdx.value());
                    if (dbg) qDebug() << QString("    Re-inserting because of MD5 checksum mismatch: %1").arg(itHdNonIdx.value().file());
                }
            }
        } else {
            toInsert.insert(itHdNonIdx.key(), itHdNonIdx.value());
            if (dbg) qDebug() << QString("    Inserting new file: %1").arg(itHdNonIdx.value().file());
        }

        ++n;
        emit signalCurrentFile(n);
        qApp->processEvents();
    }

    while (itDb.hasNext()) {
        itDb.next();

        if (!QFile::exists(itDb.value().file())) {
            toRemove.insert(itDb.key(), itDb.value());
            if (dbg) qDebug() << QString("    Removing non-existing file from DB: %1").arg(itDb.value().file());
        } else if (!filesHdIdx.contains(itDb.value().file()) && !filesHdNonIdx.contains(itDb.value().file())) {
            toRemove.insert(itDb.key(), itDb.value());
            if (dbg) qDebug() << QString("    Removing file outside of repositories from DB: %1").arg(itDb.value().file());
        }

        ++n;
        emit signalCurrentFile(n);
        qApp->processEvents();
    }
}

void BgmnRefStructureManager::dumpError(const QString &function)
{
    qDebug() << QString("BgmnRefStructureManager::%1(): Error message = %2").arg(function).arg(hklBufferDb.lastError().text());
}

bool BgmnRefStructureManager::openDb(const QString &caller)
{
    if (hklBufferDb.isOpen()) return true;

    if (!hklBufferDb.open()) {
        dumpError(caller);
        return false;
    }

    return true;
}

void BgmnRefStructureManager::filesInDirectories(QMap<QString, HklPhaseData> &idx, QMap<QString, HklPhaseData> &nonIdx)
{
    BgmnBackendConfig bkgCfg;
    QStringList fileNamesIndexed(bkgCfg.getAllStrFiles(1).values());
    QStringList fileNamesNonIndexed(bkgCfg.getAllStrFiles(2).values());

    for (int i = 0; i < fileNamesIndexed.size(); ++i) {
        QFileInfo fi(fileNamesIndexed.at(i));

        QByteArray baMd5Hash(BgmnFileIO::checksum(fi.absoluteFilePath(), QCryptographicHash::Md5));

        HklPhaseData data;
        data.setFile(fi.absoluteFilePath());
        data.setMd5Hash(QString::fromLocal8Bit(baMd5Hash));
        data.setSourceDir(fi.absolutePath());
        data.setComment(QString());
        data.setPhase(QString());
        data.setColor(QString());
        data.setFavorite(0);
        data.setStrongest(0.0);
        data.setStrongest2(0.0);
        data.setStrongest3(0.0);
        data.setLongest(0.0);
        data.setLongest2(0.0);
        data.setLongest3(0.0);
        data.setHklData(QByteArray());
        data.setXyData(QByteArray());
        data.setDoIndex(true);

        idx.insert(fi.absoluteFilePath(), data);
    }

    for (int i = 0; i < fileNamesNonIndexed.size(); ++i) {
        QFileInfo fi(fileNamesNonIndexed.at(i));

        QByteArray baMd5Hash(BgmnFileIO::checksum(fi.absoluteFilePath(), QCryptographicHash::Md5));

        HklPhaseData data;
        data.setFile(fi.absoluteFilePath());
        data.setMd5Hash(QString::fromLocal8Bit(baMd5Hash));
        data.setSourceDir(fi.absolutePath());
        data.setComment(QString());
        data.setPhase(QString());
        data.setColor(QString());
        data.setFavorite(0);
        data.setStrongest(0.0);
        data.setStrongest2(0.0);
        data.setStrongest3(0.0);
        data.setLongest(0.0);
        data.setLongest2(0.0);
        data.setLongest3(0.0);
        data.setHklData(QByteArray());
        data.setXyData(QByteArray());
        data.setDoIndex(false);

        nonIdx.insert(fi.absoluteFilePath(), data);
    }
}

QMap<QString, HklPhaseData> BgmnRefStructureManager::filesInDatabase()
{
    QMap<QString, HklPhaseData> files;
    if (!openDb("filesInDatabase")) return files;

    QSqlQuery query("SELECT * FROM referenceStructures", hklBufferDb);

    while (query.next()) {
        HklPhaseData data = queryToPhase(query);
        files.insert(data.file(), data);
    }

    return files;
}

void BgmnRefStructureManager::removeDatabaseEntry(const QString &f)
{
    if (openDb("removeDatabaseEntry")) {
        hklBufferDb.transaction();
        QSqlQuery query(hklBufferDb);
        query.prepare("DELETE FROM referenceStructures WHERE file = :file");
        query.bindValue(":file", f);
        query.exec();
        hklBufferDb.commit();
    }
}

void BgmnRefStructureManager::removeObsoleteEntries()
{
    if (!openDb("removeObsoleteEntries")) return;

    emit signalTotalFiles(filesToRemove.size());
    emit signalCurrentFile(0);

    for (int i = 0; i < filesToRemove.size(); ++i) {
        HklPhaseData hklRemove = filesToRemove.at(i);
        removeDatabaseEntry(hklRemove.file());

        emit signalCurrentText(QString("Removing %1").arg(hklRemove.file()));
        emit signalCurrentFile(i);
        qApp->processEvents();
    }

    clearDbBuffers();
}

/*
 * Insert files in database without indexing
 */
void BgmnRefStructureManager::insertFiles()
{
    if (cancelled) return;

    emit signalTotalFiles(filesToInsert.size());

    for (int i = 0; i < filesToInsert.size(); ++i) {
        HklPhaseData hklNext = filesToInsert.at(i);

        emit signalCurrentFile(i);
        emit signalCurrentText(QString("Inserting %1").arg(hklNext.file()));
        qApp->processEvents();

        BgmnStrParser strParser(hklNext.file());
        hklNext.setPhase(strParser.getPhaseName());

        insertReferenceStructure(hklNext);

        if (cancelled) return;
    }
}

/*
 * index and insert files in database
 */
void BgmnRefStructureManager::indexFiles()
{
    if (!filesToIndex.size() || cancelled) {
        updateComplete();
        return;
    }

    nCurrentFile = 0;
    emit signalTotalFiles(filesToIndex.size());

    HklPhaseData hklNext = filesToIndex.takeFirst();

    emit signalCurrentFile(nCurrentFile);
    emit signalCurrentText(QString("Indexing %1").arg(hklNext.file()));

    strIndexer->setHklPhase(hklNext);
    strIndexer->runStrIndexing();
}

void BgmnRefStructureManager::indexProcessComplete()
{ 
    HklPhaseData hklComplete = strIndexer->getLastIndexedHkl();

    if (hklComplete.hklData().isEmpty() || hklComplete.phase().isEmpty()) {
        QString msg = strIndexer->getLastIndexMessage();
        if (!msg.isEmpty()) errorFiles << indexedStructure.strFile().fileName() << msg;
    } else {
        insertReferenceStructure(hklComplete);
    }

    nCurrentFile++;

    if (filesToIndex.size() && !cancelled) {
        HklPhaseData hklNext = filesToIndex.takeFirst();

        emit signalCurrentFile(nCurrentFile);
        emit signalCurrentText(QString("Indexing %1").arg(hklNext.file()));

        strIndexer->setHklPhase(hklNext);
        strIndexer->runStrIndexing();
    } else {
        updateComplete();
    }
}

QVector<Hkl> BgmnRefStructureManager::parseParFile(const QString &parFile)
{
    bool b;
    BgmnParParser pparser(parFile, b);

    if (!b) {
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::parseParFile(): BgmnParParser reported an error parsing file %1").arg(parFile);
        return QVector<Hkl>();
    }

    QVector<Hkl> vec = pparser.getReflections(); // only of first phase?
    if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::parseParFile(): *.par file was successfully parsed. Read %1 hkl indices").arg(vec.size());

    return vec;
}

QMap<QString, QVariant> BgmnRefStructureManager::parseLstFile(const QString &lstFile)
{
    QMap<QString, QVariant> data;

    QString lstFileData(BgmnFileIO::readTextFile(lstFile));

    if (!lstFileData.isEmpty()) {
        QRegularExpression rxDensity("XrayDensity=(\\d+\\.?\\d*)");
        QRegularExpressionMatch rmDensity = rxDensity.match(lstFileData);
        if (rmDensity.hasMatch()) {
            data["XrayDensity"] = rmDensity.captured(1).toDouble();
        } else {
            data["XrayDensity"] = -1.0;
        }
    }

    return data;
}

QVector<QVector<double> > BgmnRefStructureManager::parseDiaFile(const QString &diaFile)
{
    QVector<QVector<double> > vec(2, QVector<double>());
    QStringList f = BgmnFileIO::readTextFileLines(diaFile);

    if (f.size() < 2) return vec;

    double x, y;

    for (int i = 1; i < f.size(); ++i) {
        QStringList l = f.at(i).trimmed().split(QRegularExpression("\\s+"));
        x = l.first().toDouble();
        y = l.last().toDouble();
        vec[0].append(x);
        vec[1].append(y);
    }

    return vec;
}

void BgmnRefStructureManager::insertReferenceStructure(const HklPhaseData &data)
{
    if (!openDb("insertReferenceStructure - open")) return;

    // insert or update data in sqlite database
    const QString createSql = "CREATE TABLE IF NOT EXISTS referenceStructures ("
                              "file TEXT PRIMARY KEY NOT NULL, "
                              "comment TEXT, "
                              "sourcedir TEXT NOT NULL, "
                              "md5hash TEXT NOT NULL, "
                              "phase TEXT NOT NULL, "
                              "color TEXT, "
                              "favorite INTEGER NOT NULL, "
                              "strongest REAL, "
                              "strongest2 REAL, "
                              "strongest3 REAL, "
                              "longest REAL, "
                              "longest2 REAL, "
                              "longest3 REAL, "
                              "data BLOB, "
                              "xyData BLOB, "
                              "elementCount INTEGER, "
                              "elemH  INTEGER, elemHE INTEGER, elemLI INTEGER, "
                              "elemBE INTEGER, elemB  INTEGER, elemC  INTEGER, "
                              "elemN  INTEGER, elemO  INTEGER, elemF  INTEGER, "
                              "elemNE INTEGER, elemNA INTEGER, elemMG INTEGER, "
                              "elemAL INTEGER, elemSI INTEGER, elemP  INTEGER, "
                              "elemS  INTEGER, elemCL INTEGER, elemAR INTEGER, "
                              "elemK  INTEGER, elemCA INTEGER, elemSC INTEGER, "
                              "elemTI INTEGER, elemV  INTEGER, elemCR INTEGER, "
                              "elemMN INTEGER, elemFE INTEGER, elemCO INTEGER, "
                              "elemNI INTEGER, elemCU INTEGER, elemZN INTEGER, "
                              "elemGA INTEGER, elemGE INTEGER, elemAS INTEGER, "
                              "elemSE INTEGER, elemBR INTEGER, elemKR INTEGER, "
                              "elemRB INTEGER, elemSR INTEGER, elemY  INTEGER, "
                              "elemZR INTEGER, elemNB INTEGER, elemMO INTEGER, "
                              "elemTC INTEGER, elemRU INTEGER, elemRH INTEGER, "
                              "elemPD INTEGER, elemAG INTEGER, elemCD INTEGER, "
                              "elemIN INTEGER, elemSN INTEGER, elemSB INTEGER, "
                              "elemTE INTEGER, elemI  INTEGER, elemXE INTEGER, "
                              "elemCS INTEGER, elemBA INTEGER, elemLA INTEGER, "
                              "elemCE INTEGER, elemPR INTEGER, elemND INTEGER, "
                              "elemSM INTEGER, elemPM INTEGER, elemEU INTEGER, "
                              "elemGD INTEGER, elemTB INTEGER, elemDY INTEGER, "
                              "elemHO INTEGER, elemER INTEGER, elemTM INTEGER, "
                              "elemYB INTEGER, elemLU INTEGER, elemHF INTEGER, "
                              "elemTA INTEGER, elemW  INTEGER, elemRE INTEGER, "
                              "elemOS INTEGER, elemIR INTEGER, elemPT INTEGER, "
                              "elemAU INTEGER, elemHG INTEGER, elemTL INTEGER, "
                              "elemPB INTEGER, elemBI INTEGER, elemPO INTEGER, "
                              "elemAT INTEGER, elemRN INTEGER, elemFR INTEGER, "
                              "elemRA INTEGER, elemAC INTEGER, elemTH INTEGER, "
                              "elemPA INTEGER, elemU  INTEGER, elemNP INTEGER, "
                              "elemPU INTEGER, elemAM INTEGER, elemCM INTEGER, "
                              "elemBK INTEGER, elemCF INTEGER)";

    hklBufferDb.transaction();
    QSqlQuery queryCreate(hklBufferDb);

    if (!queryCreate.exec(createSql)) {
        dumpError("insertReferenceStructure - create");
        hklBufferDb.commit();
        return;
    }

    QSqlQuery queryInsert(hklBufferDb);
    queryInsert.prepare("INSERT OR REPLACE INTO referenceStructures ("
                            "file, "
                            "comment, "
                            "sourcedir, "
                            "md5hash, "
                            "phase, "
                            "color, "
                            "favorite, "
                            "strongest, "
                            "strongest2, "
                            "strongest3, "
                            "longest, "
                            "longest2, "
                            "longest3, "
                            "data, "
                            "xyData, "
                            "elementCount, "
                            "elemH,  elemHE, elemLI, elemBE, "
                            "elemB,  elemC,  elemN,  elemO, "
                            "elemF,  elemNE, elemNA, elemMG, "
                            "elemAL, elemSI, elemP,  elemS, "
                            "elemCL, elemAR, elemK,  elemCA, "
                            "elemSC, elemTI, elemV,  elemCR, "
                            "elemMN, elemFE, elemCO, elemNI, "
                            "elemCU, elemZN, elemGA, elemGE, "
                            "elemAS, elemSE, elemBR, elemKR, "
                            "elemRB, elemSR, elemY,  elemZR, "
                            "elemNB, elemMO, elemTC, elemRU, "
                            "elemRH, elemPD, elemAG, elemCD, "
                            "elemIN, elemSN, elemSB, elemTE, "
                            "elemI,  elemXE, elemCS, elemBA, "
                            "elemLA, elemCE, elemPR, elemND, "
                            "elemPM, elemSM, elemEU, elemGD, "
                            "elemTB, elemDY, elemHO, elemER, "
                            "elemTM, elemYB, elemLU, elemHF, "
                            "elemTA, elemW,  elemRE, elemOS, "
                            "elemIR, elemPT, elemAU, elemHG, "
                            "elemTL, elemPB, elemBI, elemPO, "
                            "elemAT, elemRN, elemFR, elemRA, "
                            "elemAC, elemTH, elemPA, elemU, "
                            "elemNP, elemPU, elemAM, elemCM, "
                            "elemBK, elemCF"
                        ") VALUES("
                            ":file, "
                            ":comment, "
                            ":sourcedir, "
                            ":md5hash, "
                            ":phase, "
                            ":color, "
                            ":favorite, "
                            ":strongest, "
                            ":strongest2, "
                            ":strongest3, "
                            ":longest, "
                            ":longest2, "
                            ":longest3, "
                            ":data, "
                            ":xyData, "
                            ":elementCount, "
                            ":elemH,  :elemHE, :elemLI, :elemBE, "
                            ":elemB,  :elemC,  :elemN,  :elemO, "
                            ":elemF,  :elemNE, :elemNA, :elemMG, "
                            ":elemAL, :elemSI, :elemP,  :elemS, "
                            ":elemCL, :elemAR, :elemK,  :elemCA, "
                            ":elemSC, :elemTI, :elemV,  :elemCR, "
                            ":elemMN, :elemFE, :elemCO, :elemNI, "
                            ":elemCU, :elemZN, :elemGA, :elemGE, "
                            ":elemAS, :elemSE, :elemBR, :elemKR, "
                            ":elemRB, :elemSR, :elemY,  :elemZR, "
                            ":elemNB, :elemMO, :elemTC, :elemRU, "
                            ":elemRH, :elemPD, :elemAG, :elemCD, "
                            ":elemIN, :elemSN, :elemSB, :elemTE, "
                            ":elemI,  :elemXE, :elemCS, :elemBA, "
                            ":elemLA, :elemCE, :elemPR, :elemND, "
                            ":elemPM, :elemSM, :elemEU, :elemGD, "
                            ":elemTB, :elemDY, :elemHO, :elemER, "
                            ":elemTM, :elemYB, :elemLU, :elemHF, "
                            ":elemTA, :elemW,  :elemRE, :elemOS, "
                            ":elemIR, :elemPT, :elemAU, :elemHG, "
                            ":elemTL, :elemPB, :elemBI, :elemPO, "
                            ":elemAT, :elemRN, :elemFR, :elemRA, "
                            ":elemAC, :elemTH, :elemPA, :elemU, "
                            ":elemNP, :elemPU, :elemAM, :elemCM, "
                            ":elemBK, :elemCF)");

    queryInsert.bindValue(":file",      data.file());
    queryInsert.bindValue(":comment",   data.comment());
    queryInsert.bindValue(":sourcedir", data.sourceDir());
    queryInsert.bindValue(":md5hash",   data.md5hash());
    queryInsert.bindValue(":phase",     data.phase());
    queryInsert.bindValue(":color",     data.color());
    queryInsert.bindValue(":favorite",  data.favorite());
    queryInsert.bindValue(":strongest", data.strongest());
    queryInsert.bindValue(":strongest2",data.strongest2());
    queryInsert.bindValue(":strongest3",data.strongest3());
    queryInsert.bindValue(":longest",   data.longest());
    queryInsert.bindValue(":longest2",  data.longest2());
    queryInsert.bindValue(":longest3",  data.longest3());
    queryInsert.bindValue(":data",      data.hklBinary());
    queryInsert.bindValue(":xyData",    data.xyBinary());
    queryInsert.bindValue(":elementCount", data.elements().size());
    queryInsert.bindValue(":elemH",  data.elements().contains("H")  ? 1 : 0);
    queryInsert.bindValue(":elemHE", data.elements().contains("HE") ? 1 : 0);
    queryInsert.bindValue(":elemLI", data.elements().contains("LI") ? 1 : 0);
    queryInsert.bindValue(":elemBE", data.elements().contains("BE") ? 1 : 0);
    queryInsert.bindValue(":elemB",  data.elements().contains("B")  ? 1 : 0);
    queryInsert.bindValue(":elemC",  data.elements().contains("C")  ? 1 : 0);
    queryInsert.bindValue(":elemN",  data.elements().contains("N")  ? 1 : 0);
    queryInsert.bindValue(":elemO",  data.elements().contains("O")  ? 1 : 0);
    queryInsert.bindValue(":elemF",  data.elements().contains("F")  ? 1 : 0);
    queryInsert.bindValue(":elemNE", data.elements().contains("NE") ? 1 : 0);
    queryInsert.bindValue(":elemNA", data.elements().contains("NA") ? 1 : 0);
    queryInsert.bindValue(":elemMG", data.elements().contains("MG") ? 1 : 0);
    queryInsert.bindValue(":elemAL", data.elements().contains("AL") ? 1 : 0);
    queryInsert.bindValue(":elemSI", data.elements().contains("SI") ? 1 : 0);
    queryInsert.bindValue(":elemP",  data.elements().contains("P")  ? 1 : 0);
    queryInsert.bindValue(":elemS",  data.elements().contains("S")  ? 1 : 0);
    queryInsert.bindValue(":elemCL", data.elements().contains("CL") ? 1 : 0);
    queryInsert.bindValue(":elemAR", data.elements().contains("AR") ? 1 : 0);
    queryInsert.bindValue(":elemK",  data.elements().contains("K")  ? 1 : 0);
    queryInsert.bindValue(":elemCA", data.elements().contains("CA") ? 1 : 0);
    queryInsert.bindValue(":elemSC", data.elements().contains("SC") ? 1 : 0);
    queryInsert.bindValue(":elemTI", data.elements().contains("TI") ? 1 : 0);
    queryInsert.bindValue(":elemV",  data.elements().contains("V")  ? 1 : 0);
    queryInsert.bindValue(":elemCR", data.elements().contains("CR") ? 1 : 0);
    queryInsert.bindValue(":elemMN", data.elements().contains("MN") ? 1 : 0);
    queryInsert.bindValue(":elemFE", data.elements().contains("FE") ? 1 : 0);
    queryInsert.bindValue(":elemCO", data.elements().contains("CO") ? 1 : 0);
    queryInsert.bindValue(":elemNI", data.elements().contains("NI") ? 1 : 0);
    queryInsert.bindValue(":elemCU", data.elements().contains("CU") ? 1 : 0);
    queryInsert.bindValue(":elemZN", data.elements().contains("ZN") ? 1 : 0);
    queryInsert.bindValue(":elemGA", data.elements().contains("GA") ? 1 : 0);
    queryInsert.bindValue(":elemGE", data.elements().contains("GE") ? 1 : 0);
    queryInsert.bindValue(":elemAS", data.elements().contains("AS") ? 1 : 0);
    queryInsert.bindValue(":elemSE", data.elements().contains("SE") ? 1 : 0);
    queryInsert.bindValue(":elemBR", data.elements().contains("BR") ? 1 : 0);
    queryInsert.bindValue(":elemKR", data.elements().contains("KR") ? 1 : 0);
    queryInsert.bindValue(":elemRB", data.elements().contains("RB") ? 1 : 0);
    queryInsert.bindValue(":elemSR", data.elements().contains("SR") ? 1 : 0);
    queryInsert.bindValue(":elemY",  data.elements().contains("Y")  ? 1 : 0);
    queryInsert.bindValue(":elemZR", data.elements().contains("ZR") ? 1 : 0);
    queryInsert.bindValue(":elemNB", data.elements().contains("NB") ? 1 : 0);
    queryInsert.bindValue(":elemMO", data.elements().contains("MO") ? 1 : 0);
    queryInsert.bindValue(":elemTC", data.elements().contains("TC") ? 1 : 0);
    queryInsert.bindValue(":elemRU", data.elements().contains("RU") ? 1 : 0);
    queryInsert.bindValue(":elemRH", data.elements().contains("RH") ? 1 : 0);
    queryInsert.bindValue(":elemPD", data.elements().contains("PD") ? 1 : 0);
    queryInsert.bindValue(":elemAG", data.elements().contains("AG") ? 1 : 0);
    queryInsert.bindValue(":elemCD", data.elements().contains("CD") ? 1 : 0);
    queryInsert.bindValue(":elemIN", data.elements().contains("IN") ? 1 : 0);
    queryInsert.bindValue(":elemSN", data.elements().contains("SN") ? 1 : 0);
    queryInsert.bindValue(":elemSB", data.elements().contains("SB") ? 1 : 0);
    queryInsert.bindValue(":elemTE", data.elements().contains("TE") ? 1 : 0);
    queryInsert.bindValue(":elemI",  data.elements().contains("I")  ? 1 : 0);
    queryInsert.bindValue(":elemXE", data.elements().contains("XE") ? 1 : 0);
    queryInsert.bindValue(":elemCS", data.elements().contains("CS") ? 1 : 0);
    queryInsert.bindValue(":elemBA", data.elements().contains("BA") ? 1 : 0);
    queryInsert.bindValue(":elemLA", data.elements().contains("LA") ? 1 : 0);
    queryInsert.bindValue(":elemCE", data.elements().contains("CE") ? 1 : 0);
    queryInsert.bindValue(":elemPR", data.elements().contains("PR") ? 1 : 0);
    queryInsert.bindValue(":elemND", data.elements().contains("ND") ? 1 : 0);
    queryInsert.bindValue(":elemPM", data.elements().contains("PM") ? 1 : 0);
    queryInsert.bindValue(":elemSM", data.elements().contains("SM") ? 1 : 0);
    queryInsert.bindValue(":elemEU", data.elements().contains("EU") ? 1 : 0);
    queryInsert.bindValue(":elemGD", data.elements().contains("GD") ? 1 : 0);
    queryInsert.bindValue(":elemTB", data.elements().contains("TB") ? 1 : 0);
    queryInsert.bindValue(":elemDY", data.elements().contains("DY") ? 1 : 0);
    queryInsert.bindValue(":elemHO", data.elements().contains("HO") ? 1 : 0);
    queryInsert.bindValue(":elemER", data.elements().contains("ER") ? 1 : 0);
    queryInsert.bindValue(":elemTM", data.elements().contains("TM") ? 1 : 0);
    queryInsert.bindValue(":elemYB", data.elements().contains("YB") ? 1 : 0);
    queryInsert.bindValue(":elemLU", data.elements().contains("LU") ? 1 : 0);
    queryInsert.bindValue(":elemHF", data.elements().contains("HF") ? 1 : 0);
    queryInsert.bindValue(":elemTA", data.elements().contains("TA") ? 1 : 0);
    queryInsert.bindValue(":elemW",  data.elements().contains("W")  ? 1 : 0);
    queryInsert.bindValue(":elemRE", data.elements().contains("RE") ? 1 : 0);
    queryInsert.bindValue(":elemOS", data.elements().contains("OS") ? 1 : 0);
    queryInsert.bindValue(":elemIR", data.elements().contains("IR") ? 1 : 0);
    queryInsert.bindValue(":elemPT", data.elements().contains("PT") ? 1 : 0);
    queryInsert.bindValue(":elemAU", data.elements().contains("AU") ? 1 : 0);
    queryInsert.bindValue(":elemHG", data.elements().contains("HG") ? 1 : 0);
    queryInsert.bindValue(":elemTL", data.elements().contains("TL") ? 1 : 0);
    queryInsert.bindValue(":elemPB", data.elements().contains("PB") ? 1 : 0);
    queryInsert.bindValue(":elemBI", data.elements().contains("BI") ? 1 : 0);
    queryInsert.bindValue(":elemPO", data.elements().contains("PO") ? 1 : 0);
    queryInsert.bindValue(":elemAT", data.elements().contains("AT") ? 1 : 0);
    queryInsert.bindValue(":elemRN", data.elements().contains("RN") ? 1 : 0);
    queryInsert.bindValue(":elemFR", data.elements().contains("FR") ? 1 : 0);
    queryInsert.bindValue(":elemRA", data.elements().contains("RA") ? 1 : 0);
    queryInsert.bindValue(":elemAC", data.elements().contains("AC") ? 1 : 0);
    queryInsert.bindValue(":elemTH", data.elements().contains("TH") ? 1 : 0);
    queryInsert.bindValue(":elemPA", data.elements().contains("PA") ? 1 : 0);
    queryInsert.bindValue(":elemU",  data.elements().contains("U")  ? 1 : 0);
    queryInsert.bindValue(":elemNP", data.elements().contains("NP") ? 1 : 0);
    queryInsert.bindValue(":elemPU", data.elements().contains("PU") ? 1 : 0);
    queryInsert.bindValue(":elemAM", data.elements().contains("AM") ? 1 : 0);
    queryInsert.bindValue(":elemCM", data.elements().contains("CM") ? 1 : 0);
    queryInsert.bindValue(":elemBK", data.elements().contains("BK") ? 1 : 0);
    queryInsert.bindValue(":elemCF", data.elements().contains("CF") ? 1 : 0);

    if (!queryInsert.exec()) {
        hklBufferDb.commit();
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::insertReferenceStructure(): Query Error:  %1").arg(queryInsert.lastError().text());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::insertReferenceStructure(): DB Error:     %1").arg(queryInsert.lastError().databaseText());
        if (settings->verboseLevel() > 1) qDebug() << QString("BgmnRefStructureManager::insertReferenceStructure(): Driver Error: %1").arg(queryInsert.lastError().driverText());
        dumpError("insertReferenceStructure - insert");
        return;
    }

    hklBufferDb.commit();
}

QString BgmnRefStructureManager::getComment(const QString &file)
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

QMultiMap<double, PeakFile> BgmnRefStructureManager::nearestPeakFiles(double pos, const QString &repo, bool fav, int near)
{
    QMultiMap<double, PeakFile> fileList;
    if (!openDb("nearestPeakFiles")) return fileList;

    double dif = 0.0;
    QString fileName;
    int favorite;

    QSqlQuery query(hklBufferDb);
    query.setForwardOnly(true);

    if (repo.isEmpty()) {
        query.prepare("SELECT file, favorite, phase, strongest, strongest2, strongest3 FROM referenceStructures");
    } else {
        query.prepare("SELECT file, favorite, phase, strongest, strongest2, strongest3 FROM referenceStructures WHERE sourcedir = :sdir");
        query.bindValue(":sdir", repo);
    }

    query.exec();

    int idxFile     = query.record().indexOf("file");
    int idxFavorite = query.record().indexOf("favorite");
    int idxPhase    = query.record().indexOf("phase");
    int idxStrong   = query.record().indexOf("strongest");
    int idxStrong2  = query.record().indexOf("strongest2");
    int idxStrong3  = query.record().indexOf("strongest3");

    while (query.next()) {
        favorite = query.value(idxFavorite).toInt();

        if ((!fav) || (favorite > 0)) {
            fileName = query.value(idxFile).toString();

            double dif1 = qAbs(query.value(idxStrong).toDouble()  - pos/10.0);
            double dif2 = qAbs(query.value(idxStrong2).toDouble() - pos/10.0);
            double dif3 = qAbs(query.value(idxStrong3).toDouble() - pos/10.0);

            if      (near == 2) dif = qMin(dif1, dif2);
            else if (near == 3) dif = qMin(dif1, qMin(dif2, dif3));
            else                dif = dif1;

            PeakFile pfile;
            pfile.fileInfo = QFileInfo(fileName);
            pfile.phase = query.value(idxPhase).toString();
            pfile.offset = dif;
            fileList.insert(dif, pfile);
        }
    }

    return fileList;
}

QStringList BgmnRefStructureManager::favoritesToUnset(const QStringList &favs)
{
    QStringList unsetFavs;
    if (!openDb("favoritesToUnset")) return unsetFavs;

    QSqlQuery query(hklBufferDb);
    hklBufferDb.transaction();

    query.prepare("SELECT file, favorite FROM referenceStructures");
    query.exec();
    int idxFile = query.record().indexOf("file");
    int idxFav  = query.record().indexOf("favorite");

    QString ifile;
    int ifav;

    while (query.next()) {
        ifile = query.value(idxFile).toString();
        ifav  = query.value(idxFav).toInt();

        if ((!favs.contains(ifile)) && (ifav > 0)) {
            unsetFavs.append(ifile);
        }
    }

    hklBufferDb.commit();
    return unsetFavs;
}

void BgmnRefStructureManager::updateFavorites(const QStringList &files, int nstatus)
{
    int ostatus = nstatus == 0 ? 1 : 0;
    if (!openDb("updateFavorites")) return;

    QSqlQuery query(hklBufferDb);
    hklBufferDb.transaction();

    for (int i = 0; i < files.size(); ++i) {
        query.prepare("UPDATE referenceStructures SET favorite = :nfav WHERE file = :file AND favorite = :ofav");
        query.bindValue(":file", files.at(i));
        query.bindValue(":nfav", nstatus);
        query.bindValue(":ofav", ostatus);
        query.exec();
    }

    hklBufferDb.commit();
}

QMap<QString, QTreeWidgetItem*> BgmnRefStructureManager::createDirTree(QTreeWidget *treeWidget, const QStringList &roots, bool favorites, const Qt::ItemFlags &dirFlags)
{
    bool extraDebugOutput = false;
    treeWidget->clear();
    QMap<QString, QTreeWidgetItem*> mappedItems;
    QMultiMap<int, QStringList> sortedSourceDirs;
    QSet<QString> parsedPaths;
    QString sep("/"); // do not use QDir::separator(), which returns the native separator
    QStringList strDirs = roots;
    strDirs.append(getAllSourceDirs(favorites));

    // sort the sourceDirs by their depth
    for (int i = 0; i < strDirs.size(); ++i) {
        if (parsedPaths.contains(strDirs.at(i))) continue;

        QStringList path = QDir::fromNativeSeparators(strDirs.at(i)).split(sep);

        sortedSourceDirs.insert(path.size(), path);
        parsedPaths.insert(strDirs.at(i));
    }

    QList<int> k = sortedSourceDirs.uniqueKeys();

    // create items for each depth, starting with topmost items
    foreach (int d, k) {
        // all sourceDirs of depth d
        QList<QStringList> pathsAtDepthD = sortedSourceDirs.values(d);

        // the paths are stringlists split at dir separator
        foreach (QStringList path, pathsAtDepthD) {
            QTreeWidgetItem *par = nullptr;
            if (extraDebugOutput)  qDebug() << QString("*** processing: %1").arg(path.join(sep));

            int parDepth = 0;
            // looping up the dir tree and check if one of the mapped items is a parent
            for (int i = path.size()-1; i > 0; --i) {
                QString testParDir(path.mid(0, i).join(sep));
                if (extraDebugOutput)  qDebug() << QString("  - testing if parent is found: %1").arg(testParDir);

                if (mappedItems.contains(testParDir)) {
                    par = mappedItems.value(testParDir);
                    parDepth = i;
                    if (extraDebugOutput)  qDebug() << QString("    - Yes, parent depth is %1").arg(i);
                    break;
                } else {
                    if (extraDebugOutput) qDebug()  << QString("    - No");
                }
            }

            if (par) {
                if (extraDebugOutput) qDebug() << QString("  + building tree from %2 to %1").arg(path.join(sep)).arg(par->data(0, Qt::UserRole).toString());

                // create all items from parent+1 to path
                for (int i = parDepth; i < path.size(); ++i) {
                    if (extraDebugOutput) qDebug() << QString("    - new child item: %1").arg(path.at(i));
                    QTreeWidgetItem *cit = new QTreeWidgetItem(QStringList() << path.at(i), TYPE_DIRECTORY);
                    cit->setData(0, Qt::UserRole, path.mid(0, i+1).join(sep));
                    cit->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
                    cit->setFlags(dirFlags);
                    cit->setFirstColumnSpanned(true);
                    cit->setToolTip(0, QDir::toNativeSeparators(path.mid(0, i+1).join(sep)));

                    mappedItems.insert(path.mid(0, i+1).join(sep), cit);
                    par->addChild(cit);
                    par->setExpanded(true);
                    par = cit;
                    if (extraDebugOutput) qDebug() << QString("    - new parent is: %1").arg(par->data(0, Qt::UserRole).toString());
                }
            } else {
                if (extraDebugOutput) qDebug() << QString("  + adding toplevelitem %1").arg(path.join(sep));
                QTreeWidgetItem *tit = new QTreeWidgetItem(QStringList() << QDir::toNativeSeparators(path.join(sep)), TYPE_DIRECTORY);
                tit->setData(0, Qt::UserRole, path.join(sep));
                tit->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
                tit->setFlags(dirFlags);
                tit->setFirstColumnSpanned(true);
                tit->setToolTip(0, QDir::toNativeSeparators(path.join(sep)));

                mappedItems.insert(path.join(sep), tit);
                treeWidget->addTopLevelItem(tit);
            }
        }
    }

    return mappedItems;
}

void BgmnRefStructureManager::createFileTree(const QMap<QString, QTreeWidgetItem *> &dirItems, bool favorites, const Qt::ItemFlags &fileFlags)
{
    QList<HklPhaseData> strFiles = favorites ? getFavPhases() : getAllPhases();

    for (int i = 0; i < strFiles.size(); ++i) {
        const HklPhaseData *data = &strFiles.at(i);

        if (!dirItems.contains(data->sourceDir())) {
            continue;
        }

        QFileInfo fi(data->file());
        QTreeWidgetItem *it = new QTreeWidgetItem(TYPE_FILE);

        it->setFlags(fileFlags);
        if (it->flags().testFlag(Qt::ItemIsUserCheckable)) it->setCheckState(0, Qt::Unchecked);
        it->setData(0, Qt::UserRole, data->file());
        it->setText(0, fi.fileName());
        it->setText(1, data->phase());
        it->setText(2, data->comment());

        it->setToolTip(0, data->file());
        it->setToolTip(1, data->phase());
        it->setToolTip(2, data->comment());

        dirItems.value(data->sourceDir())->addChild(it);
        dirItems.value(data->sourceDir())->setExpanded(true);
    }

}

void BgmnRefStructureManager::createTree(QTreeWidget *treeWidget, const QStringList &roots, bool favorites, const Qt::ItemFlags &dirFlags, const Qt::ItemFlags &fileFlags)
{
    treeWidget->clear();
    QMap<QString, QTreeWidgetItem*> dirItems = createDirTree(treeWidget, roots, favorites, dirFlags);
    createFileTree(dirItems, favorites, fileFlags);
}

QString BgmnRefStructureManager::getConditionsString(const QStringList &all, const QStringList &one, const QStringList &none)
{
    QStringList out;

    if (all.size()) {
        QStringList l;

        for (int i = 0; i < all.size(); ++i) {
            l.append(QString("elem%1 = 1").arg(all.at(i)));
        }

        out.append(QString("(%1)").arg(l.join(" AND ")));
    }

    if (one.size()) {
        QStringList l;

        for (int i = 0; i < one.size(); ++i) {
            l.append(QString("elem%1 = 1").arg(one.at(i)));
        }

        out.append(QString("(%1)").arg(l.join(" OR ")));
    }

    if (none.size()) {
        QStringList l;

        for (int i = 0; i < none.size(); ++i) {
            l.append(QString("elem%1 = 0").arg(none.at(i)));
        }

        out.append(QString("(%1)").arg(l.join(" AND ")));
    }

    return out.join(" AND ").simplified();
}

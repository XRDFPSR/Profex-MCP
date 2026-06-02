/***************************************************************************
                          bgmnrefstructuremanager.h  -  description
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

#ifndef BGMNREFSTRUCTUREMANAGER_H
#define BGMNREFSTRUCTUREMANAGER_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <QMap>
#include <QFileInfo>
#include <QtSql/QtSql>
#include <QMap>
#include <QTreeWidget>
#include <QTextEdit>
#include "../libXrdIO/hkl.h"
#include "../libXrdIO/hklphasedata.h"
#include "../libXrdIO/scan.h"
#include "../libXrdIO/settingsmanager.h"
#include "indexedhklstructure.h"
#include "bgmnstrindexer.h"

enum itemType {
    TYPE_DIRECTORY = 1001,
    TYPE_FILE = 1002
};

struct PeakFile {
    QFileInfo fileInfo;
    QString phase;
    double offset;
};

class BgmnRefStructureManager : public QObject
{
    Q_OBJECT
public:
    static BgmnRefStructureManager *getInstance();
    void destroy();

    void init();
    void update();
    void sync();
    void closeDb();
    QString getHklBufferFileName();
    QList<HklPhaseData> getAllPhases(const QStringList &elAll = QStringList(),
                                     const QStringList &elOne = QStringList(),
                                     const QStringList &elNone = QStringList());
    QList<HklPhaseData> getSubDirPhases(const QString &,
                                        const QStringList &elAll = QStringList(),
                                        const QStringList &elOne = QStringList(),
                                        const QStringList &elNone = QStringList());
    QList<HklPhaseData> getFavPhases(const QStringList &elAll = QStringList(),
                                     const QStringList &elOne = QStringList(),
                                     const QStringList &elNone = QStringList());

    QSet<QString> getAllFileNames(const QStringList &elAll = QStringList(),
                                  const QStringList &elOne = QStringList(),
                                  const QStringList &elNone = QStringList());
    QSet<QString> getFavFileNames(const QStringList &elAll = QStringList(),
                                  const QStringList &elOne = QStringList(),
                                  const QStringList &elNone = QStringList());

    QVector<Hkl> getReferenceLines(const QFileInfo &);
    QVector<Hkl> getReferenceLines(const QString &);

    QVector<QVector<double> > getReferencePattern(const QFileInfo &);
    QVector<QVector<double> > getReferencePattern(const QString &);

    Scan getReferenceScan(const QFileInfo &);
    Scan getReferenceScan(const QString &);

    void referencePosClicked(double, QMultiMap<double, PeakFile> &, const QString &, bool, int near = 1);
    QStringList getAllSourceDirs(bool favs = false);
    QStringList getIndexedFileNames(const QString &src = QString());
    QStringList getIndexedFavoriteFileNames();
    void setFavorites(const QStringList &);
    int countAllPhases();
    int countAllPhasesIndexed();
    int countFavPhases();
    int countFavPhasesIndexed();
    int countPhasesInDirectory(const QString &dir);
    int countPhasesInDirectoryIndexed(const QString &dir);

    void setXyData(const QString &file, const Scan &scan);

    QMap<QString, QTreeWidgetItem *> createDirTree(QTreeWidget *treeWidget, const QStringList &roots, bool favorites, const Qt::ItemFlags &dirCheckMode);
    void createFileTree(const QMap<QString, QTreeWidgetItem *> &, bool favorites, const Qt::ItemFlags &fileFlags);
    void createTree(QTreeWidget *, const QStringList &roots, bool favorites,
                    const Qt::ItemFlags &dirCheckMode = Qt::ItemFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled),
                    const Qt::ItemFlags &fileFlags = Qt::ItemFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled));

    inline HklPhaseData getLastIndexedHkl() const {return indexedStructure.hklPhase();}
    inline QString getLastIndexMessage() const    {return indexedStructure.lastMessage();}

    void clearDbBuffers();

private:
    static bool instanceFlag;
    static BgmnRefStructureManager *instance;
    explicit BgmnRefStructureManager(QObject *parent);
    ~BgmnRefStructureManager();

    bool cancelled;
    SettingsManager *settings;
    QString projectID;
    QSqlDatabase hklBufferDb;
    QString lastLogMessage;
    unsigned short runningProcesses;
    QList<HklPhaseData> filesToRemove;
    QList<HklPhaseData> filesToInsert;
    QList<HklPhaseData> filesToIndex;
    QStringList errorFiles;
    BgmnStrIndexer *strIndexer;
    int nCurrentFile;
    IndexedHklStructure indexedStructure;

    QMap<QString, QStringList> bufferedFileNames;
    QStringList bufferedFavFileNames;
    QMap<QString, int> bufferedDirFileCount;
    QMap<QString, int> bufferedDirFileCountIndexed;
    int bufferedFileCountAll;
    int bufferedFileCountIndexed;
    int bufferedFavFileCountAll;
    int bufferedFavFileCountIndexed;

    bool openDb(const QString &caller);
    void diffFilesWithDb(QMap<QString, HklPhaseData> &toRemove, QMap<QString, HklPhaseData> &toIndex, QMap<QString, HklPhaseData> &toInsert);
    void dumpError(const QString &);
    void filesInDirectories(QMap<QString, HklPhaseData> &, QMap<QString, HklPhaseData> &);
    QMap<QString, HklPhaseData> filesInDatabase();
    void removeObsoleteEntries();
    void indexFiles();
    void insertFiles();

    QVector<Hkl> parseParFile(const QString &parFile);
    QMap<QString, QVariant> parseLstFile(const QString &lstFile);
    QVector<QVector<double> > parseDiaFile(const QString &diaFile);

    void runIndexing();
    void insertReferenceStructure(const HklPhaseData &);
    QString getComment(const QString &);
    QMultiMap<double, PeakFile> nearestPeakFiles(double pos, const QString &, bool fav, int near = 1);
    QStringList favoritesToUnset(const QStringList &);
    void updateFavorites(const QStringList &, int);
    HklPhaseData queryToPhase(const QSqlQuery &);
    void removeDatabaseEntry(const QString &);

    QString getConditionsString(const QStringList &, const QStringList &, const QStringList &);

public slots:
    void cancel();
    void skipCurrent();

private slots:
    void updateComplete();
    void indexProcessComplete();

signals:
    void signalTotalFiles(int);
    void signalCurrentFile(int);
    void signalCurrentText(const QString &);
    void signalProcessOutput(const QString &);
    void signalUpdateComplete(const QStringList &);
};

#endif // BGMNREFSTRUCTUREMANAGER_H

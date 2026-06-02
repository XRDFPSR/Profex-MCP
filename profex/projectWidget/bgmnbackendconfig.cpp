/***************************************************************************
                          bgmnbackendconfig.h  -  description
                             -------------------
    begin                : Mon Jan 25 16:00:00 CEST 2016
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

#include "bgmnbackendconfig.h"

#include <QDebug>
#include <QFileInfo>
#include <QString>
#include <QFile>
#include <QDirIterator>
#include <QApplication>

BgmnBackendConfig::BgmnBackendConfig()
{
    settings = SettingsManager::getInstance();
    initSettings();
}

void BgmnBackendConfig::initSettings()
{
    execFiles.append(ExecFile("bgmnProject/bgmnExec",                                        // Settings tag
                              settings->value("bgmnProject/bgmnExec", QString()).toString(), // Settings value
                              "bgmn"));                                                      // Exec file name (lower case, no extension)

    execFiles.append(ExecFile("bgmnProject/makegeqExec",
                              settings->value("bgmnProject/makegeqExec", QString()).toString(),
                              "makegeq"));

    execFiles.append(ExecFile("bgmnProject/geometExec",
                              settings->value("bgmnProject/geometExec", QString()).toString(),
                              "geomet"));

    execFiles.append(ExecFile("bgmnProject/teilExec",
                              settings->value("bgmnProject/teilExec", QString()).toString(),
                              "teil"));

    execFiles.append(ExecFile("bgmnProject/eflechExec",
                              settings->value("bgmnProject/eflechExec", QString()).toString(),
                              "eflech"));

    deviceDir = DbDir("bgmnProject/deviceDatabase",                                                // Settings tag
                      settings->value("bgmnProject/deviceDatabase", QStringList()).toStringList(), // Default value
                      "Devices");                                                                  // Dir name

    strucDirs = DbDir("bgmnProject/structureDatabaseIndexed", QStringList(), "Structures");
    strucDirs.dirpath.append(settings->value("bgmnProject/structureDatabaseIndexed", QStringList()).toStringList());
    strucDirs.dirpath.append(settings->value("bgmnProject/structureDatabaseNonIndexed", QStringList()).toStringList());

    presetDir = DbDir("bgmnProject/presetDirectory",
                      settings->value("bgmnProject/presetDirectory", QStringList()).toStringList(),
                      "Presets");
}

void BgmnBackendConfig::createDefaultExecSettings()
{
    for (int i = 0; i < execFiles.size(); ++i) {
        execFiles[i].filepath = bundledBgmnLocation(execFiles.at(i).exename);
        settings->setValue(execFiles.at(i).settingsTag, execFiles.at(i).filepath);
    }
}

QString BgmnBackendConfig::bundledBgmnLocation(const QString &s)
{
#ifdef Q_OS_WIN
    QFileInfo pe(qApp->applicationFilePath());
    QFileInfo exe(pe.absolutePath() + QDir::separator() + "BGMNwin" + QDir::separator() + s.toUpper() + ".EXE");
#else
    #ifdef Q_OS_MACOS
        QFileInfo pe(qApp->applicationFilePath());
        QFileInfo exe(pe.absolutePath() + "/../Resources/BGMNwin/" + s);
    #else
        QFileInfo exe;
    #endif
#endif

    if (exe.exists()) {
        qDebug() << QString("BgmnBackendConfig::bundledBgmnLocation(): Setting default location: %1").arg(exe.canonicalFilePath());
        return exe.canonicalFilePath();
    }

    qDebug() << QString("BgmnBackendConfig::bundledBgmnLocation(): Default location not found: %1").arg(exe.canonicalFilePath());
    return QString();
}

bool BgmnBackendConfig::checkExecutables()
{
#ifdef Q_OS_WIN
    int defaultBackendMode = 0;
#else
    #ifdef Q_OS_MACOS
        int defaultBackendMode = 0;
    #else
        int defaultBackendMode = 1;
    #endif
#endif

    if (settings->value("bgmnProject/bgmnInstallation", defaultBackendMode).toInt() == 0) {
        createDefaultExecSettings();
    }

    bool allFound = true;

    for (int i = 0; i < execFiles.size(); ++i) {
        if (QFile::exists(execFiles.at(i).filepath)) {
            if (defaultBackendMode == 0) qDebug() << QString("BgmnBackendConfig::checkExecutables(): %1 found in bundle: %2").arg(execFiles.at(i).settingsTag).arg(execFiles.at(i).filepath);
            if (defaultBackendMode == 1) qDebug() << QString("BgmnBackendConfig::checkExecutables(): %1 read from settings: %2").arg(execFiles.at(i).settingsTag).arg(execFiles.at(i).filepath);
        } else {
            qDebug() << QString("BgmnBackendConfig::checkExecutables(): %1 doesn't exist").arg(execFiles.at(i).exename);
            if ((execFiles.at(i).exename != "teil") && (execFiles.at(i).exename != "eflech")) {
                // do not cause error message if only teil and eflech are not found.
                // these two are not essential for running bgmn
                allFound = false;
            }
        }
    }

    return allFound;
}

bool BgmnBackendConfig::checkRepositories()
{
    if (!deviceDir.dirpath.size()) return false;
    if (!strucDirs.dirpath.size()) return false;
    if (!presetDir.dirpath.size()) return false;

    int reposDevErrors   = 0;
    int reposStrucErrors = 0;
    int reposPresErrors  = 0;

    for (int i = 0; i < deviceDir.dirpath.size(); ++i) {
        QDir repo(deviceDir.dirpath.at(i));
        if (!repo.exists()) ++reposDevErrors;
    }

    for (int i = 0; i < strucDirs.dirpath.size(); ++i) {
        QDir repo(strucDirs.dirpath.at(i));
        if (!repo.exists()) ++reposStrucErrors;
    }

    for (int i = 0; i < presetDir.dirpath.size(); ++i) {
        QDir repo(presetDir.dirpath.at(i));
        if (!repo.exists()) ++reposPresErrors;
    }

    if (reposDevErrors + reposStrucErrors + reposPresErrors > 0) {
        qDebug() << QString("BgmnBackendConfig::checkRepositories(): %1 missing device repositories").arg(reposDevErrors);
        qDebug() << QString("BgmnBackendConfig::checkRepositories(): %1 missing structure repositories").arg(reposStrucErrors );
        qDebug() << QString("BgmnBackendConfig::checkRepositories(): %1 missing preset repositories").arg(reposPresErrors);
        return false;
    }

    return true;
}

/*
 * performs automatic detection of the BGMN backend.
 * returns true if successful, else returns false
 */
bool BgmnBackendConfig::guessExecutables()
{
    #ifdef Q_OS_WIN
        int errors = guessExecutablesWindows();
    #else
        #ifdef Q_OS_MACOS
            int errors = guessExecutablesMac();
        #else
            int errors = guessExecutablesUnix();
        #endif
    #endif

    if (errors == 0) {
        return true;
    }

    return false;
}

bool BgmnBackendConfig::guessRepositories()
{
#ifdef Q_OS_WIN
    int errors = guessRepositoriesWindows();
#else
    #ifdef Q_OS_MACOS
        int errors = guessRepositoriesMac();
    #else
        int errors = guessRepositoriesUnix();
    #endif
#endif

    if (errors == 0) {
        return true;
    }

    return false;
}

/*
 * try to guess the path of executables and databases on Linux systems.
 * do not call this function from the constructor, because it needs to know the executable
 * path of profex (profexExec), which is set from main after construction of MainWindow.
 *
 * returns the number of errors (0 = all files and directories were found)
 */
int BgmnBackendConfig::guessExecutablesUnix()
{
    QFileInfo pe(qApp->applicationFilePath());
    QDir ped(pe.absoluteDir());
    ped.cdUp();
    ped.cd("BGMNwin");

    int errors = 0;

    // scan these directories, use the first one that was found
    // note: the order of scanning determines the priority of the version that will be used
    // if several installations are found
    QVector<QDir> defaultExeLocations;
    defaultExeLocations << ped.absolutePath()
            << QDir("/opt/Profex-BGMN/BGMNwin")
            << QDir(QString("%1/BGMN").arg(QDir::homePath()))
            << QDir(QString("%1/BGMNwin").arg(QDir::homePath()))
            << QDir("/opt/bgmnwin")
            << QDir("/opt/bgmn")
            << QDir("/opt/bgmn-4.2.23")
            << QDir("/opt/bgmn-4.2.22")
            << QDir("/opt/BGMNwin")
            << QDir("/usr/bin")
            << QDir("/usr/local/bin");

    QString lastSuccessfulDir;

    // check if all executable files exist
    for (int i = 0; i < execFiles.size(); ++i) {
        if (QFile::exists(execFiles.at(i).filepath)) {
            // the file path read from settings is correct, nothing to do
            QFileInfo fi(execFiles.at(i).filepath);
            lastSuccessfulDir = fi.absolutePath();
            qDebug() << QString("BgmnBackendConfig::guessExecutablesUnix(): %1 read from settings: %2").arg(execFiles.at(i).settingsTag).arg(execFiles.at(i).filepath);
        } else {
            // the file path read from settings doesn't exist, so scan all directories in dirList for the exe file
            QString ename = execFiles.at(i).exename;

            QVector<QDir> tDirList = defaultExeLocations;
            if (!lastSuccessfulDir.isEmpty()) tDirList.prepend(QDir(lastSuccessfulDir));

            if (findFileInDirs(ename, tDirList, execFiles[i].filepath)) {
                // file found, store it in settings
                QFileInfo fi(execFiles.at(i).filepath);
                lastSuccessfulDir = fi.absolutePath();
                settings->setValue(execFiles.at(i).settingsTag, execFiles.at(i).filepath);
                qDebug() << QString("BgmnBackendConfig::guessExecutablesUnix(): %1 autodetected: %2").arg(execFiles.at(i).settingsTag).arg(execFiles.at(i).filepath);
            } else {
                // file not found, report error
                qDebug() << QString("BgmnBackendConfig::guessExecutablesUnix(): %1 could not be found").arg(execFiles.at(i).exename);
                if ((execFiles.at(i).exename != "teil") && (execFiles.at(i).exename != "eflech")) {
                    ++errors;
                }
            }
        }
    }

    return errors;
}

/*
 * try to guess the path of executables and databases on Windows systems.
 * do not call this function from the constructor, because it needs to know the executable
 * path of profex (profexExec), which is set from main after construction of MainWindow.
 *
 * returns the number of errors (0 = all files and directories were found)
 */
int BgmnBackendConfig::guessExecutablesWindows()
{
    QFileInfo pe(qApp->applicationFilePath());
    int errors = 0;

    // on windows, assume that the BGMNwin directory and the profex directory are both at
    // the same place, e.g. Program Files
    QDir bdir(pe.absolutePath());

    qDebug() << QString("BgmnBackendConfig::guessExecutablesWindows(): Profex binary is located in: %1").arg(bdir.absolutePath());

    QVector<QDir> dirList;
    dirList << QDir(bdir.absolutePath() + "/BGMNwin");

    QString lastSuccessfulDir;

    // check if all executable files exist
    for (int i = 0; i < execFiles.size(); ++i) {
        if (QFile::exists(execFiles.at(i).filepath)) {
            // the file path read from settings is correct, nothing to do
            QFileInfo fi(execFiles.at(i).filepath);
            lastSuccessfulDir = fi.absolutePath();
            qDebug() << QString("BgmnBackendConfig::guessExecutablesWindows(): %1 read from settings: %2").arg(execFiles.at(i).settingsTag).arg(execFiles.at(i).filepath);
        } else {
            // the file path read from settings doesn't exist, so scan all directories in dirList for the exe file
            QString ename = execFiles.at(i).exename.toUpper() + ".EXE";

            QVector<QDir> tDirList = dirList;
            if (!lastSuccessfulDir.isEmpty()) tDirList.prepend(QDir(lastSuccessfulDir));

            if (findFileInDirs(ename, tDirList, execFiles[i].filepath)) {
                // file found, store it in settings
                QFileInfo fi(execFiles.at(i).filepath);
                lastSuccessfulDir = fi.absolutePath();
                settings->setValue(execFiles.at(i).settingsTag, execFiles.at(i).filepath);
                qDebug() << QString("BgmnBackendConfig::guessExecutablesWindows(): %1 autodetected: %2").arg(execFiles.at(i).settingsTag).arg(execFiles.at(i).filepath);
            } else {
                // file not found, report error
                qDebug() << QString("BgmnBackendConfig::guessExecutablesWindows(): %1 could not be found").arg(execFiles.at(i).exename);
                if ((execFiles.at(i).exename != "teil") && (execFiles.at(i).exename != "eflech")) {
                    ++errors;
                }
            }
        }
    }

    return errors;
}

/*
 * try to guess the path of executables and databases on OS X systems.
 * do not call this function from the constructor, because it needs to know the executable
 * path of profex (profexExec), which is set from main after construction of MainWindow.
 *
 * returns the number of errors (0 = all files and directories were found)
 */
int BgmnBackendConfig::guessExecutablesMac()
{
    QFileInfo pe(qApp->applicationFilePath());
    int errors = 0;

    // on mac, assume that the BGMNwin directory and the profex bundle are both at
    // the same place
    QDir edir(pe.absolutePath());
    qDebug() << QString("BgmnBackendConfig::guessExecutablesMac(): Profex binary is located in: %1").arg(pe.absolutePath());

    edir.cdUp();
    edir.cd("Resources");

    if (edir.exists("BGMNwin")) {
        qDebug() << QString("BgmnBackendConfig::guessExecutablesMac(): Confirmed - BGMNwin directory is located in %1").arg(edir.absolutePath());
    } else {
        qDebug() << QString("BgmnBackendConfig::guessExecutablesMac(): Warning - BGMNwin directory not found in %1").arg(edir.absolutePath());
    }

    QVector<QDir> dirList;
    dirList << QDir(edir.absolutePath() + "/BGMNwin");

    QString lastSuccessfulDir;

    // check if all executable files exist
    for (int i = 0; i < execFiles.size(); ++i) {
        if (QFile::exists(execFiles.at(i).filepath)) {
            // the file path read from settings is correct, nothing to do
            QFileInfo fi(execFiles.at(i).filepath);
            lastSuccessfulDir = fi.absolutePath();
            qDebug() << QString("BgmnBackendConfig::guessExecutablesMac(): %1 read from settings: %2").arg(execFiles.at(i).settingsTag).arg(execFiles.at(i).filepath);
        } else {
            // the file path read from settings doesn't exist, so scan all directories in dirList for the exe file
            QString ename = execFiles.at(i).exename;

            QVector<QDir> tDirList = dirList;
            if (!lastSuccessfulDir.isEmpty()) tDirList.prepend(QDir(lastSuccessfulDir));

            if (findFileInDirs(ename, tDirList, execFiles[i].filepath)) {
                // file found, store it in settings
                QFileInfo fi(execFiles.at(i).filepath);
                lastSuccessfulDir = fi.absolutePath();
                settings->setValue(execFiles.at(i).settingsTag, execFiles.at(i).filepath);
                qDebug() << QString("BgmnBackendConfig::guessExecutablesMac(): %1 autodetected: %2").arg(execFiles.at(i).settingsTag).arg(execFiles.at(i).filepath);
            } else {
                // file not found, report error
                qDebug() << QString("BgmnBackendConfig::guessExecutablesMac(): %1 could not be found").arg(execFiles.at(i).exename);
                if ((execFiles.at(i).exename != "teil") && (execFiles.at(i).exename != "eflech")) {
                    ++errors;
                }
            }
        }
    }

    return errors;
}

int BgmnBackendConfig::guessRepositoriesUnix()
{
    QFileInfo pe(qApp->applicationFilePath());
    QDir ped(pe.absoluteDir());
    ped.cdUp();

    int errors = 0;

    QVector<QDir> defaultRepoLocations;
    defaultRepoLocations << ped
                         << QDir("/opt/Profex-BGMN/BGMN-Templates")
                         << QDir("/opt/BGMN-Templates")
                         << QDir(QString("%1/Documents/BGMN-Templates").arg(QDir::homePath()))
                         << createUserRepos();

    errors += setDbDirs(deviceDir, defaultRepoLocations);
    errors += setDbDirs(strucDirs, defaultRepoLocations);
    errors += setDbDirs(presetDir, defaultRepoLocations);

    return errors;
}

int BgmnBackendConfig::guessRepositoriesWindows()
{
    QFileInfo pe(qApp->applicationFilePath());
    int errors = 0;

    // database directories are expected to be stored in the profex directory
    QVector<QDir> defaultRepoLocations;
    defaultRepoLocations << pe.absolutePath()
                         << createUserRepos();

    errors += setDbDirs(deviceDir, defaultRepoLocations);
    errors += setDbDirs(strucDirs, defaultRepoLocations);
    errors += setDbDirs(presetDir, defaultRepoLocations);

    return errors;
}

int BgmnBackendConfig::guessRepositoriesMac()
{
    QFileInfo pe(qApp->applicationFilePath());
    int errors = 0;

    QDir bdir(pe.absolutePath()); // this returns the path of the executable inside the bundle (<installdir>/profex.app/Contents/MacOS/)

    bdir.cdUp(); // <installdir>/profex.app/Contents/
    bdir.cdUp(); // <installdir>/profex.app/
    bdir.cdUp(); // <installdir>/

    if (bdir.exists("Profex.app")) {
        qDebug() << QString("BgmnBackendConfig::guessRepositoriesMac(): Confirmed - Profex app bundle is located in %1").arg(bdir.absolutePath());
    } else {
        qDebug() << QString("BgmnBackendConfig::guessRepositoriesMac(): Warning - Profex app bundle not found in %1").arg(bdir.absolutePath());
    }

    if (bdir.exists("BGMN-Templates")) {
        qDebug() << QString("BgmnBackendConfig::guessRepositoriesMac(): Confirmed - BGMN-Templates directory is located in %1").arg(bdir.absolutePath());
    } else {
        qDebug() << QString("BgmnBackendConfig::guessRepositoriesMac(): Warning - BGMN-Templates directory not found in %1").arg(bdir.absolutePath());
    }

    // database directories are expected to be stored in the profex directory
    QVector<QDir> defaultRepoLocations;
    defaultRepoLocations << QDir(bdir.absolutePath() + "/BGMN-Templates")
                         << createUserRepos();

    errors += setDbDirs(deviceDir, defaultRepoLocations);
    errors += setDbDirs(strucDirs, defaultRepoLocations);
    errors += setDbDirs(presetDir, defaultRepoLocations);

    return errors;
}

/*
 * Checks if valid directories were read from the settings. If not, runs auto-detection
 *
 * returns 1 if errors occurred, or 0 if all went well
 */
int BgmnBackendConfig::setDbDirs(const DbDir &d, const QVector<QDir> &v)
{
    // keep the repos read from the settings that still exist
    QStringList repos = checkIfDirsExist(d);

    // check which of the autoconfigured repos exist
    QStringList autoconf = checkIfDirsExist(v, d.dirname);

    // merge the two lists
    for (int i = 0; i < autoconf.size(); ++i) {
        if (!repos.contains(autoconf.at(i)))
            repos.append(autoconf.at(i));
    }

    if (repos.size()) {
        // auto-config successful
        settings->setValue(d.settingsTag, repos);
        qDebug() << QString("BgmnBackendConfig::setDbDirs(): %1 autodetected: %2").arg(d.settingsTag).arg(repos.join("; "));
        return 0;
    }

    // auto-config failed
    qDebug() << QString("BgmnBackendConfig::setDbDirs(): %1 could not be found").arg(d.settingsTag);
    return 1;
}

/*
 * eliminates all directories that don't exist from d
 */
QStringList BgmnBackendConfig::checkIfDirsExist(const DbDir &d)
{
    QStringList existing;

    for (int i = 0; i < d.dirpath.size(); ++i) {
        if (QFile::exists(d.dirpath.at(i))) {
            qDebug() << QString("BgmnBackendConfig::checkIfDirsExist(): Checking %1 ... found.").arg(d.dirpath.at(i));
            existing.append(d.dirpath.at(i));
        } else {
            qDebug() << QString("BgmnBackendConfig::checkIfDirsExist(): Checking %1 ... not found.").arg(d.dirpath.at(i));
        }
    }

    return existing;
}

/*
 * eliminates all directories that don't exist from d
 */
QStringList BgmnBackendConfig::checkIfDirsExist(const QVector<QDir> &d, const QString &n)
{
    QStringList existing;

    for (int i = 0; i < d.size(); ++i) {
        QDir dir(d.at(i).absolutePath() + QDir::separator() + n);
        if (dir.exists()) {
            qDebug() << QString("BgmnBackendConfig::checkIfDirsExist(): Checking %1 ... found.").arg(dir.absolutePath());
            existing.append(dir.absolutePath());
        } else {
            qDebug() << QString("BgmnBackendConfig::checkIfDirsExist(): Checking %1 ... not found.").arg(dir.absolutePath());
        }
    }

    return existing;
}

/*
 * finds file "fn" in a list of directories "dirs", and writes the absolute file path to "ret"
 * returns true if successful, false if not. If not successful, "ret" is erased
 */
bool BgmnBackendConfig::findFileInDirs(const QString &fn, const QVector<QDir> &dirs, QString &ret)
{
    for (int i = 0; i < dirs.size(); ++i) {
        if (dirs.at(i).exists(fn)) {
            ret = dirs.at(i).absoluteFilePath(fn);
            return true;
        }
    }

    ret = QString();
    return false;
}

/*
 * returns a list of all device files (*.geq) in the current BGMN installation. The map contains:
 * <QFileInfo::completeBaseName();QFileInfo::absoluteFilePath()>
 */
QMultiMap<QString, QString> BgmnBackendConfig::getAllDevFiles()
{
    QStringList dirs = settings->value("bgmnProject/deviceDatabase", QStringList()).toStringList();
    QStringList filters(QStringList() << "*.geq" << "*.GEQ");

    return getAllFilesOfType(dirs, filters, false);
}

/*
 * returns a list of all str files (*.str) in the current BGMN installation. The map contains:
 * <QFileInfo::completeBaseName();QFileInfo::absoluteFilePath()>
 * n = 0: search all repos
 * n = 1: search indexed repos only
 * n = 2: search unindexed repos only
 */
QMultiMap<QString, QString> BgmnBackendConfig::getAllStrFiles(int n)
{
    QStringList dirsIdx(settings->value("bgmnProject/structureDatabaseIndexed", QStringList()).toStringList());
    QStringList dirsNonIdx(settings->value("bgmnProject/structureDatabaseNonIndexed", QStringList()).toStringList());
    QStringList filters(QStringList() << "*.str" << "*.STR");

    if (n == 0)      return getAllFilesOfType(QStringList() << dirsIdx << dirsNonIdx, filters, true);
    else if (n == 1) return getAllFilesOfType(dirsIdx, filters, true);
    else             return getAllFilesOfType(dirsNonIdx, filters, true);
}

/*
 * returns a list of all wavelength distribution files (*.lam) in the current BGMN installation. The map contains:
 * <QFileInfo::completeBaseName();QFileInfo::absoluteFilePath()>
 */
QMultiMap<QString, QString> BgmnBackendConfig::getAllLamFiles()
{
    QFileInfo fiExec(settings->value("bgmnProject/bgmnExec", QString()).toString());
    QStringList filters(QStringList() << "*.lam" << "*.LAM");

    return getAllFilesOfType(fiExec.absolutePath(), filters, false);
}

/*
 * returns a list of files in "root"
 */
QMultiMap<QString, QString> BgmnBackendConfig::getAllFilesOfType(const QString &root, const QStringList &filter, bool recoursive)
{
    QDir dir(root);
    QMultiMap<QString, QString> files;

    if (!dir.exists()) return files;

    QDirIterator it(root,
                    filter,
                    QDir::NoFilter | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable,
                    recoursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
        it.next();
        QFileInfo fi(it.fileInfo());
        files.insert(fi.completeBaseName(), fi.absoluteFilePath());
    }

    return files;
}

/*
 * returns a list of files in all directories given in "root"
 */
QMultiMap<QString, QString> BgmnBackendConfig::getAllFilesOfType(const QStringList &root, const QStringList &filter, bool recoursive)
{
    QMultiMap<QString, QString> files;

    for (int i = 0; i < root.size(); ++i) {
        files.unite(getAllFilesOfType(root.at(i), filter, recoursive));
    }

    return files;
}

QDir BgmnBackendConfig::createUserRepos()
{
    QString tplDir = settings->getAppDataLocation();
    QDir strDir(tplDir + QDir::separator() + "Structures");
    QDir devDir(tplDir + QDir::separator() + "Devices");
    QDir prsDir(tplDir + QDir::separator() + "Presets");

    if (!strDir.exists()) strDir.mkpath(".");
    if (!devDir.exists()) devDir.mkpath(".");
    if (!prsDir.exists()) prsDir.mkpath(".");

    return QDir(tplDir);
}

/*
 * returns the structure repo located in AppDataLocation
 * falls back to the first repo if AppDataLocation wasn't found
 */
QString BgmnBackendConfig::getUserStuctureRepo()
{
    QDir searchDir(settings->getAppDataLocation());
    QStringList repos = settings->value("bgmnProject/structureDatabaseIndexed", QStringList()).toStringList();
    repos.append(settings->value("bgmnProject/structureDatabaseNonIndexed", QStringList()).toStringList());

    for (int i = 0; i < repos.size(); ++i) {
        QDir repoDir(repos.at(i));
        repoDir.cdUp();

        if (repoDir.absolutePath() == searchDir.absolutePath()) {
            return repos.at(i);
        }
    }

    return repos.size() ? repos.first() : QString();
}

QString BgmnBackendConfig::getUserDeviceRepo()
{
    QDir searchDir(settings->getAppDataLocation());
    QStringList repos = settings->value("bgmnProject/deviceDatabase", QStringList()).toStringList();

    for (int i = 0; i < repos.size(); ++i) {
        QDir repoDir(repos.at(i));
        repoDir.cdUp();

        if (repoDir.absolutePath() == searchDir.absolutePath()) {
            return repos.at(i);
        }
    }

    return repos.size() ? repos.first() : QString();
}

QString BgmnBackendConfig::getUserPresetRepo()
{
    QDir searchDir(settings->getAppDataLocation());
    QStringList repos = settings->value("bgmnProject/presetDirectory", QStringList()).toStringList();

    for (int i = 0; i < repos.size(); ++i) {
        QDir repoDir(repos.at(i));
        repoDir.cdUp();

        if (repoDir.absolutePath() == searchDir.absolutePath()) {
            return repos.at(i);
        }
    }

    return repos.size() ? repos.first() : QString();
}

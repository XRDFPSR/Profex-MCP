/***************************************************************************
                          bgmnbackendconfig.cpp  -  description
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

#ifndef BGMNBACKENDCONFIG_H
#define BGMNBACKENDCONFIG_H

#include <QVector>
#include <QDir>
#include "../libXrdIO/settingsmanager.h"

struct ExecFile {
    QString settingsTag;
    QString filepath;
    QString exename;

    ExecFile() : settingsTag(), filepath(), exename() {}
    ExecFile(QString s, QString p, QString f) : settingsTag(s), filepath(p), exename(f) {}
};

struct DbDir {
    QString settingsTag;
    QStringList dirpath;
    QString dirname;

    DbDir() : settingsTag(), dirpath(), dirname() {}
    DbDir(QString s, QStringList p, QString f) : settingsTag(s), dirpath(p), dirname(f) {}
};

class BgmnBackendConfig
{
public:
    BgmnBackendConfig();

    bool checkExecutables();
    bool checkRepositories();
    bool guessExecutables();
    bool guessRepositories();

    QMultiMap<QString, QString> getAllDevFiles();
    QMultiMap<QString, QString> getAllStrFiles(int n = 0);
    QMultiMap<QString, QString> getAllLamFiles();

    QString getUserStuctureRepo();
    QString getUserDeviceRepo();
    QString getUserPresetRepo();

private:
    SettingsManager *settings;
    QVector<ExecFile> execFiles;
    DbDir deviceDir;
    DbDir strucDirs;
    DbDir presetDir;

    void initSettings();
    void createDefaultExecSettings();
    QString bundledBgmnLocation(const QString &);
    QStringList checkIfDirsExist(const DbDir &);
    QStringList checkIfDirsExist(const QVector<QDir> &, const QString &);
    int setDbDirs(const DbDir &, const QVector<QDir> &);

    QMultiMap<QString, QString> getAllFilesOfType(const QString &root, const QStringList &filter, bool recoursive = false);
    QMultiMap<QString, QString> getAllFilesOfType(const QStringList &root, const QStringList &filter, bool recoursive = false);

    int guessExecutablesUnix();
    int guessExecutablesWindows();
    int guessExecutablesMac();

    int guessRepositoriesUnix();
    int guessRepositoriesWindows();
    int guessRepositoriesMac();

    bool findFileInDirs(const QString &fn, const QVector<QDir> &dirs, QString &ret);
    QDir createUserRepos();

};

#endif // BGMNBACKENDCONFIG_H

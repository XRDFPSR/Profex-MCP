/***************************************************************************
                          main.h  -  description
                             -------------------
    begin                : Mon Sep 22 09:00:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#include <QApplication>
#include <QStringList>
#include <QString>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QMutex>

#include "mainwindow.h"

static QFile outFile; // Static global file
static QTextStream *str = nullptr;
#ifndef Q_OS_LINUX
static QMutex logMutex; // For thread safety

QString logFileName()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/../Profex5/";
    QFileInfo logDir(appDataPath);

    if (!logDir.isWritable()) {
        QDir dir(logDir.absoluteFilePath());
        if (!dir.mkpath(".")) {
            logDir = QFileInfo(QDir::homePath() + "/");
        }
    }

    return logDir.canonicalFilePath() + "/profexed.log";
}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);

    QMutexLocker locker(&logMutex); // Ensure thread safety

    if (str && outFile.isOpen()) {
        (*str) << QString("[%1] %2\n").arg(QDateTime::currentDateTime().toString()).arg(msg);
        str->flush(); // Ensure all data is written
    }
}
#endif

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("doebelin.org");
    QCoreApplication::setApplicationName("ProfexED");

#ifdef Q_OS_WIN
    QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif

#ifdef Q_OS_LINUX
    QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif

#ifdef Q_OS_MACOS
    QApplication::setStyle(QStyleFactory::create("macOS"));
#endif

    QApplication a(argc, argv);

    QString logDest("(not set)");

    // we only write the log file on Windows and OS X. On Linux we can easily get the debug output
    // in the console
    #ifdef QT_NO_DEBUG_OUTPUT
        logDest = "(debug output disabled)";
    #else
        #ifdef Q_OS_LINUX
            logDest = "Console";
        #else
    outFile.setFileName(logFileName());
    if (outFile.exists()) {
        outFile.remove();
    }

    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        logDest = outFile.fileName();
        str = new QTextStream(&outFile);
        qInstallMessageHandler(myMessageOutput);

        qDebug() << "Log initialized at:" << logDest;
    } else {
        logDest = QString("Disabled (could not open file %1").arg(outFile.fileName());
    }
        #endif
    #endif

    MainWindow w;
    QStringList l;

    for (int i = 1; i < argc; ++i) {
        l.append(QString(argv[i]));
    }

    if (l.size()) {
        QFileInfo fi(l.first());
        if (fi.isFile()) w.setWorkingDir(fi.absolutePath());
        if (fi.isDir())  w.setWorkingDir(fi.absoluteFilePath());
        w.loadFileList(l);
    }

    w.setLogDest(logDest);

    if (w.doShowMaximized()) {
        w.showMaximized();
    } else {
        w.show();
    }

    int result = a.exec();

    // Ensure clean shutdown
    if (str) {
        str->flush();
        delete str;
        str = nullptr;
    }
    if (outFile.isOpen()) {
        outFile.close();
    }

    return result;
}

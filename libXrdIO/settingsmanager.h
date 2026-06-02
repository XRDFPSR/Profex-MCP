/***************************************************************************
                          settingsmanager.h  -  description
                             -------------------
    begin                : Tue Mar 15 13:10:07 CET 2016
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

#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QMap>
#include <QString>
#include <QVariant>
#include <QSettings>
#include <QStringList>
#include <QColor>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

/* singleton pattern */

/*
 * Used as an interface to QSettings. We could access a QSettings object
 * directly. But the program makes heavy use of the SettingsManager singleton
 * object to read and write data to during painting operations. Thus, in order
 * to guarantee good performance, SettingsManager buffers these read and
 * write accesses in memory, and only writes them to disk when ::sync() is
 * called.
 */

class XRDIO_EXPORT SettingsManager
{
public:
    static SettingsManager* getInstance();

    void destroy();
    void initialize();
    void sync();

    void setValue(const QString &key, const QVariant &value);

    QVariant value(const QString &key, const QVariant &value = QVariant()) const;
    QSettings::Format format() const;
    QString fileName() const;
    QStringList allKeys() const;
    QColor getRandomColor(int) const;
    QColor toFillColor(const QColor &, const QColor &bg = QColor(255, 255, 255, 0)) const;

    QList<QColor> getColorList(int);
    void setColorList(const QList<QColor> &);

    QList<int> getScanStyleList(int);
    void setScanStyleList(const QList<int> &);
    QStringList getScanStyleNames();

    QString getTempLocation() const;
    QString getAppDataLocation() const;
    int getSyntaxHighlightingMode() const;
    bool isDarkMode() const;
    double defaultWavelength() const;
    int verboseLevel() const;
    bool probeDirectoryWriteAccess(const QString &dir, QStringList &errors);

private:
    static bool instanceFlag;
    static bool needsSync;
    static SettingsManager *instance;

    SettingsManager();
    ~SettingsManager();

    QSettings *settings;
    QMap<QString, QVariant> settingsData;
};

#endif // SETTINGSMANAGER_H

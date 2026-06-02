/***************************************************************************
                          settingsmanager.cpp  -  description
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

#include "settingsmanager.h"
#include "structs.h"
#include <QtMath>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QPalette>
#include <QDir>
#include <QUuid>

bool SettingsManager::instanceFlag = false;
bool SettingsManager::needsSync = false;
SettingsManager* SettingsManager::instance = nullptr;

SettingsManager::SettingsManager()
{
    settings = new QSettings("doebelin.org", "Profex5");
    settings->setDefaultFormat(QSettings::NativeFormat);
}

SettingsManager::~SettingsManager()
{
    instanceFlag = false;
    if (needsSync) sync();
    if (settings) delete settings;
}

SettingsManager* SettingsManager::getInstance()
{
    if (!instanceFlag) {
        instance = new SettingsManager();
        instanceFlag = true;
        return instance;
    } else {
        return instance;
    }
}

void SettingsManager::destroy()
{
    if (instanceFlag) delete instance;
}

void SettingsManager::initialize()
{
    settingsData.clear();
    if (!instanceFlag) return;

    QStringList keys = settings->allKeys();

    for (int i = 0; i < keys.size(); ++i) {
        settingsData.insert(keys.at(i), settings->value(keys.at(i), QString()));
    }

    needsSync = false;
}

void SettingsManager::sync()
{
    if (!instanceFlag) return;
    QMap<QString, QVariant>::ConstIterator it = settingsData.constBegin();

    while (it != settingsData.constEnd()) {
        settings->setValue(it.key(), it.value());
        ++it;
    }

    settings->sync();
    needsSync = false;
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    settingsData.insert(key, value);
    needsSync = true;
}

QVariant SettingsManager::value(const QString &key, const QVariant &value) const
{
    return settingsData.value(key, value);
}

QSettings::Format SettingsManager::format() const
{
    if (!instanceFlag) return QSettings::InvalidFormat;
    return settings->format();
}

QString SettingsManager::fileName() const
{
    if (!instanceFlag) return QString();
    return settings->fileName();
}

QStringList SettingsManager::allKeys() const
{
    if (!instanceFlag) return QStringList();
    return settings->allKeys();
}

QColor SettingsManager::getRandomColor(int n) const
{
    double steps = M_E + 3.0; // steps between colors. use a non-periodic value
                               // so the colors don't repeat after one revolution
                               // in h space
    double yellowDamp = 0.5;  // dampens v in the yellow range to avoid very bright yellow hues
                         // value = 0: no dampening, yellow is bright
                         // value = 1: maximum dampening, yellow turns black
    int offset = 4;      // change this to adjust the first color in h space. Avoid color number 4 to be
                         // dark blue, as it would be very similar to the hard-coded blue for background
                         // curves

    // h goes in circles through hue space in steps of n/steps.
    //   steps is a non-int value so h is shifted a little bit with each revolution
    // v is a sine curve that is darkest ("value") in the yellow and green area,
    //   and brightest (1.0) in blue and violet area
    double h = double(n + offset) / steps - qFloor(double(n + offset) / steps);
    double s = 1.0;
    double v = 1.0 - yellowDamp * 0.5 * (1.0 + qSin(M_PI * (2.0*h - 1.0/6.0)));
    double a = 1.0;

    QColor col;
    col.setHsvF(h, s, v, a);

    return col;
}

QColor SettingsManager::toFillColor(const QColor &c, const QColor &bg) const
{
    int bcol = bg.value();
    QColor fcol = c;

    if (bcol < 150) {
        fcol.setHsl(c.hslHue(), c.hslSaturation() / 3, c.lightness() / 3);
    } else {
        fcol.setHsl(c.hslHue(), c.hslSaturation() / 3, (c.lightness() + 768) / 4);
    }

    return fcol;
}

/*
 * n is the number of requested colors (i.e. the size of the scan heap).
 * the returned list can be longer than n, if more colors are found in the
 * settings, but will never be shorter than n. If necessary, new colors
 * will be generated to return n colors.
 */
QList<QColor> SettingsManager::getColorList(int n)
{
    QList<QVariant> v = settingsData.value("graph/colorTable", QList<QVariant>()).toList();
    QList<QColor> c;
    bool newCol = false;

    for (int i = 0; i < qMax(n, v.size()); ++i) {
        if (i < v.size()) {
            c.append(QColor(v.at(i).toString()));
        } else {
            c.append(getRandomColor(i));
            newCol = true;
        }
    }

    if (newCol) {
        QList<QVariant> vnew;

        for (int i = 0; i < c.size(); ++i) {
            vnew.append(QVariant(c.at(i).name()));
        }

        setValue("graph/colorTable", vnew);
    }

    return c;
}

void SettingsManager::setColorList(const QList<QColor> &lcol)
{
    QList<QVariant> lvar;

    for (int i = 0; i < lcol.size(); ++i) {
        lvar.append(lcol.at(i).name());
    }

    settingsData["graph/colorTable"] = lvar;
}

QList<int> SettingsManager::getScanStyleList(int n)
{
    QList<QVariant> lv = settingsData.value("graph/styleTable", QList<QVariant>()).toList();
    QList<int> li;

    for (int i = 0; i < qMax(n, lv.size()); ++i) {
        if (i < lv.size()) li.append(lv.at(i).toInt());
        else li.append(0);
    }

    return li;
}

void SettingsManager::setScanStyleList(const QList<int> &l)
{
    QList<QVariant> lv;

    for (int i = 0; i < l.size(); ++i) {
        lv.append(QVariant(l.at(i)));
    }

    settingsData["graph/styleTable"] = lv;
}

QStringList SettingsManager::getScanStyleNames()
{
    QStringList pstyles;

    pstyles << "Solid"
            << "Points"
            << "Crosses"
            << "Dash Line"
            << "Solid + Points"
            << "Solid + Crosses"
            << "Dash Line + Points"
            << "Dash Line + Crosses";

    return pstyles;
}

QString SettingsManager::getAppDataLocation() const
{
    QString s = QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!instanceFlag) return s;
    return settings->value("config/hklBufferFile", s).toString();
}

QString SettingsManager::getTempLocation() const
{
    QString s =  QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    if (!instanceFlag) return s;
    return settings->value("config/tempFileLocation", s).toString();
}

int SettingsManager::getSyntaxHighlightingMode() const
{
    if (!instanceFlag) return -1;
    int hlmode = settings->value("config/editorSyntaxHighlighting", -1).toInt();

    if (hlmode < 0) {
        if (isDarkMode()) {
            // assuming dark mode
            return 1;
        } else {
            // assuming light mode
            return 0;
        }
    }

    return hlmode;
}

bool SettingsManager::isDarkMode() const
{
    return (QGuiApplication::palette().color(QPalette::Base).value() < 150);
}

double SettingsManager::defaultWavelength() const
{
    if (!instanceFlag) return 1.54056;
    return settings->value("config/defaultWl", 1.54056).toDouble();
}

int SettingsManager::verboseLevel() const
{
    if (!instanceFlag) return 3;
    return settings->value("config/debugVerboseLevel", 3).toInt();
}

bool SettingsManager::probeDirectoryWriteAccess(const QString &dir, QStringList &errors)
{
    QString fname(dir + QDir::separator() + "Profex-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".tmp");
    QFile file(fname);
    errors.clear();

    qDebug() << QString("SettingsManager::probeDirectoryWriteAccess(): Probing write access to directory:");
    qDebug() << QString("    %1").arg(dir);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << QString("    Error opening file for writing: %1").arg(fname);
        qDebug() << QString("    Error string: %1").arg(file.errorString());
        errors.append(file.errorString());
        return false;
    } else {
        qDebug() << QString("    Opening file for writing was successful: %1").arg(fname);

        int i = file.write(QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss.zzz").toLocal8Bit());

        if (i <= 0) {
            qDebug() << QString("    Error writing to file %1").arg(fname);
            errors.append(file.errorString());
            return false;
        } else {
            qDebug() << QString("    %1 bytes written to file").arg(i);
        }

        file.close();

        if (!file.remove()) {
            qDebug() << QString("    Error deleting test file %1").arg(fname);
            errors.append(file.errorString());
            return false;
        } else {
            qDebug() << QString("    File deleted successfully.");
            qDebug() << QString("    Access to directory is working.");
        }
    }

    return true;
}

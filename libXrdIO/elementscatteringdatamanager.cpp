/***************************************************************************
                          elementscatteringdatamanager.cpp  -  description
                             -------------------
    begin                : Wed Jul 14 18:25:07 CET 2021
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

#include "elementscatteringdatamanager.h"
#include "elementscatteringdatadownloader.h"
#include "functions.h"

ElementScatteringDataManager::ElementScatteringDataManager()
{
    initData();
}

void ElementScatteringDataManager::initData()
{
    // data source:
    // https://physics.nist.gov/PhysRefData/FFast/html/form.html

    // warning: Before changing this bool to false to read the scattering data from
    // html files, read the comments in "elementscatteringdatadownloader.cpp",
    // as it needs some manual intervention.
    // normally leave this at true to read the data from the binary resources.
    bool useBinaryResource = true;

    if (useBinaryResource) {
        QFile f(":/resources/anoScatData.bin");
        f.open(QIODevice::ReadOnly);
        QDataStream in(&f);

        while (!in.atEnd()) {
            qint32 l;
            QByteArray b;
            in >> l;
            b.resize(l);
            in >> b;
            ElementScatteringData el(QString(), 0);
            el.deserialize(b);
            _data.insert(el.z(), el);
        }
    } else {
        ElementScatteringDataDownloader esDl;
        _data = esDl.getDataMap();
    }
}

QList<ElementScatteringData> ElementScatteringDataManager::getAllScatteringData() const
{
    return _data.values();
}

void ElementScatteringDataManager::getElementValuesAtEnergy(double v, QStringList &el, QList<int> &z, QList<double> &f0, QList<double> &f1, QList<double> &f2, QList<double> &mac, QList<double> &lac)
{
    QMapIterator<int, ElementScatteringData> it(_data);

    while (it.hasNext()) {
        it.next();
        el.append(it.value().element());
        z.append(it.value().z());
        f0.append(interpolate(v, it.value().dataEnergy(), it.value().dataF0()));
        f1.append(interpolate(v, it.value().dataEnergy(), it.value().dataF1()));
        f2.append(interpolate(v, it.value().dataEnergy(), it.value().dataF2()));
        mac.append(interpolate(v, it.value().dataEnergy(), it.value().dataMac()));
        lac.append(interpolate(v, it.value().dataEnergy(), it.value().dataLac()));
    }
}

double ElementScatteringDataManager::interpolate(double v, const QVector<double> &x, const QVector<double> &y)
{
    double val = 0.0;
    double xlo = 0.0;
    double xhi = 0.0;
    double ylo = 0.0;
    double yhi = 0.0;
    bool ok = false;

    if (x.size() != y.size()) return val;
    if (v < x.first())        return val;
    if (v > x.last())         return val;

    for (int i = 1; i < x.size(); ++i) {
        if (x.at(i) > v) {
            xlo = x.at(i-1);
            xhi = x.at(i);
            ylo = y.at(i-1);
            yhi = y.at(i);
            ok = true;
            break;
        }
    }

    if (!ok) return val;

    val = ylo + (v-xlo)/(xhi-xlo) * (yhi-ylo);
    return val;
}

QString ElementScatteringDataManager::getElementSymbol(int z)
{
    return _data.value(z, ElementScatteringData(QString(), -1)).element();
}

double ElementScatteringDataManager::getF0ForKeV(const QString &el, double v)
{
    if (el.isEmpty()) return -1.0;

    QMapIterator<int, ElementScatteringData> it(_data);
    while (it.hasNext()) {
        it.next();
        if (el.toUpper() == it.value().element().toUpper()) {
            return interpolate(v, it.value().dataEnergy(), it.value().dataF0());
        }
    }

    return -1.0;
}

double ElementScatteringDataManager::getF0ForKeV(int z, double v)
{
    ElementScatteringData d = _data.value(z, ElementScatteringData(QString(), -1));
    if (d.z() > 0) return interpolate(v, d.dataEnergy(), d.dataF0());
    return -1.0;
}

double ElementScatteringDataManager::getMacForKeV(const QString &el, double v)
{
    if (el.isEmpty()) return -1.0;

    QMapIterator<int, ElementScatteringData> it(_data);
    while (it.hasNext()) {
        it.next();
        if (el.toUpper() == it.value().element().toUpper()) {
            return interpolate(v, it.value().dataEnergy(), it.value().dataMac());
        }
    }

    return -1.0;
}

double ElementScatteringDataManager::getMacForKeV(int z, double v)
{
    ElementScatteringData d = _data.value(z, ElementScatteringData(QString(), -1));
    if (d.z() > 0) return interpolate(v, d.dataEnergy(), d.dataMac());
    return -1.0;
}

double ElementScatteringDataManager::getF0ForWlNm(const QString &el, double v)
{
    return getF0ForKeV(el, global::Functions::wavelengthToEnergy(v));
}

double ElementScatteringDataManager::getF0ForWlNm(int z, double v)
{
    return getF0ForKeV(z, global::Functions::wavelengthToEnergy(v));
}

double ElementScatteringDataManager::getMacForWlNm(const QString &el, double v)
{
    return getMacForKeV(el, global::Functions::wavelengthToEnergy(v));
}

double ElementScatteringDataManager::getMacForWlNm(int z, double v)
{
    return getMacForKeV(z, global::Functions::wavelengthToEnergy(v));
}


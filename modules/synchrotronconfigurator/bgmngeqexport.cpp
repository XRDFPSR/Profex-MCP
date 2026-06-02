/***************************************************************************
                          synchrotronl2interpolator.h  -  description
                             -------------------
    begin                : Sat Dec 28 09:16:00 CEST 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#include "bgmngeqexport.h"
#include <QDataStream>
#include <QtConcurrent>
#include <QMutexLocker>

BgmnGeqExport::BgmnGeqExport(QObject *parent)
    : QObject(parent), wmin_(5.0), wmax_(140.0), wstep_(0.01)
{
    geometry = GEOMETRY::REFLEXION;
    d_ = 0.0;
    r_ = 0.0;
    t_ = 0.0;
}

void BgmnGeqExport::setCurveData(const QList<LorentzParams> &inputData)
{
    QMutexLocker locker(&mutex_);
    QMap<double, LorentzParams> map;

    // use a map to sort the support points by theta
    for (int i = 0; i < inputData.size(); ++i) {
        map[inputData.at(i).t()] = inputData.at(i);
    }

    lorentzParams_ = map.values();
    wmin_ = lorentzParams_.constFirst().t() * 2.0;
    wmax_ = lorentzParams_.constLast().t() * 2.0;
}

void BgmnGeqExport::setInstrumentParameters(double d, double r, double t)
{
    d_ = d;
    r_ = r;
    t_ = t;
}

bool BgmnGeqExport::checkInputData() const
{
    if (t_ < -1e-6) { // >= 0.0
        qWarning() << QString("BgmnGeqExport::checkInputData(): t_ is invalid (%1)").arg(t_);
        return false;
    }

    if (r_ < 1e-6) { // > 0.0
        qWarning() << QString("BgmnGeqExport::checkInputData(): r_ is invalid (%1)").arg(r_);
        return false;
    }

    if (wstep_ < 1e-6) { // > 0.0
        qWarning() << QString("BgmnGeqExport::checkInputData(): wstep_ is invalid (%1)").arg(wstep_);
        return false;
    }

    if (wmin_ < -1e-6) { // >= 0.0
        qWarning() << QString("BgmnGeqExport::checkInputData(): wmin_ is invalid (%1)").arg(wmin_);
        return false;
    }

    if (wmax_ < 1e-6) { // > 0.0
        qWarning() << QString("BgmnGeqExport::checkInputData(): wmax_ is invalid (%1)").arg(wmax_);
        return false;
    }

    return true;
}

QByteArray BgmnGeqExport::getBinaryFileContent()
{
    if (!checkInputData()) return QByteArray();

    QByteArray binaryData;
    QDataStream out(&binaryData, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);

    // Write header
    struct Header {
        char kenn[8] = "GEQ.C1";
        float tdivr;
        float tdivd;
        float sinxmin;
        float sinxmax;
        char ger[100];
    } header;

    double tdivd_ = t_ / d_;
    double tdivr_ = t_ / r_;

    header.sinxmin = static_cast<float>(std::sin(wmin_ * M_PI / 180.0));
    header.sinxmax = static_cast<float>(std::sin(wmax_ * M_PI / 180.0));
    header.tdivr = static_cast<float>(tdivr_);
    header.tdivd = static_cast<float>(tdivd_);

    // Convert QString to QByteArray, then to char*
    QByteArray gerBytes = ger_.toUtf8(); // Converts QString to UTF-8 encoded QByteArray
    const char *gerFileName = gerBytes.constData(); // Get C-style string from QByteArray

    // Clear header.ger and copy gerFileName into it, ensuring null-termination
    std::memset(header.ger, 0, sizeof(header.ger));
    std::strncpy(header.ger, gerFileName, sizeof(header.ger) - 1);

    out.writeRawData(reinterpret_cast<char *>(&header), sizeof(header));

    for (int i = 0; i < lorentzParams_.size(); ++i) {
        float sinx = static_cast<float>(std::sin(lorentzParams_.at(i).t() * M_PI / 180.0));
        int nCurves = lorentzParams_.at(i).size();

        if (nCurves > 0) {
            out.writeRawData(reinterpret_cast<char *>(&sinx), sizeof(sinx));
            out.writeRawData(reinterpret_cast<char *>(&nCurves), sizeof(nCurves));

            QList<float> g;
            QList<float> e;
            QList<float> q;

            for (int j = 0; j < nCurves; ++j) {
                g.append(static_cast<float>(lorentzParams_.at(i).at(j).g()));
                e.append(static_cast<float>(lorentzParams_.at(i).at(j).e()));
                q.append(static_cast<float>(lorentzParams_.at(i).at(j).q()));
            }

            out.writeRawData(reinterpret_cast<const char *>(g.data()), g.size() * sizeof(float));
            out.writeRawData(reinterpret_cast<const char *>(e.data()), e.size() * sizeof(float));
            out.writeRawData(reinterpret_cast<const char *>(q.data()), q.size() * sizeof(float));
        }
    }

    return binaryData;
}

/***************************************************************************
                          profexcsvexport.cpp  -  description
                             -------------------
    begin                : Wed Apr 17 22:16:07 CEST 2024
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

#include "profexpgxexport.h"
#include "functions.h"

ProfexPgxExport::ProfexPgxExport(QObject *parent)
    : GenericExport{parent}
{
    uId = "PROFEX_PGX";
}

int ProfexPgxExport::save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &)
{
    QString out;
    out += QString("#HEADER lambda=%1\n").arg(scan.waveLength(), 0, 'f', 8);
    out += getData(QVector<Scan>() << scan);
    out += getHkl(QVector<Scan>() << scan);
    out += QString("#END\n");

    return writeFile(file, out);
}

int ProfexPgxExport::save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &)
{
    if (!scanHeap.size()) return 0;

    QString out;
    out += QString("#HEADER lambda=%1\n").arg(scanHeap.first().waveLength(), 0, 'f', 8);
    out += getDataSep(scanHeap);
    out += getHklSep(scanHeap);
    out += QString("#END\n");

    return writeFile(file, out);
}

QString ProfexPgxExport::getData(const QVector<Scan> &scanHeap)
{
    QString out("#SCANS");

    int rMax = 0;

    for (int i = 0; i < scanHeap.size(); ++i) {
        rMax = qMax(rMax, scanHeap.at(i).size());
    }

    for (int c = 0; c < scanHeap.size(); ++c) {
        out += QString(" \"Diffraction Angle [°2theta]\" \"%1\"").arg(scanHeap.at(c).name());
    }

    out += "\n";

    for (int r = 0; r < rMax; ++r) {
        QStringList line;

        for (int c = 0; c < scanHeap.size(); ++c) {
            const Scan scan = scanHeap.at(c);

            if (r < scan.size()) {
                line << QString("%1").arg(scan.pDataAngle().at(r), 8, 'f', 4)
                     << QString("%1").arg(scan.pDataIntensity().at(r), precision + 6, 'f', precision);
            } else {
                line << QString(8, ' ') << QString(precision + 6, ' ');
            }
        }

        out += line.join(" ") + QString("\n");
    }

    return out;
}

QString ProfexPgxExport::getHkl(const QVector<Scan> &scanHeap)
{
    QString out("#HKL");

    int rMax = 0;
    QList<int> scanIdx;

    for (int i = 0; i < scanHeap.size(); ++i) {
        if (scanHeap.at(i).hasHklData()) {
            scanIdx.append(i);
            rMax = qMax(rMax, scanHeap.at(i).pDataHkl().size());
        }
    }

    for (int c = 0; c < scanIdx.size(); ++c) {
        out += QString(" \"Phase\" \"h k l\" \"Diffraction Angle [°2theta]\" \"Intensity\"");
    }

    out += "\n";

    for (int r = 0; r < rMax; ++r) {
        QStringList line;

        for (int c = 0; c < scanIdx.size(); ++c) {
            const Scan scan = scanHeap.at(scanIdx.at(c));
            double wl = scan.waveLength();
            bool cvD = scan.getHklXunit() == "dnm";

            if (r < scan.pDataHkl().size()) {
                const Hkl hkl = scan.pDataHkl().at(r);
                double x = hkl.position();
                double y = hkl.intensity();

                line << QString("\"%1\"").arg(scan.name())
                     << QString("%1").arg("\"" + hkl.hkl() + "\"", 11)
                     << QString("%1").arg(cvD ? global::Functions::dToTwoTheta(10.0 * x, wl) : x, 8, 'f', 4)
                     << QString("%1").arg(y, precision + 6, 'f', precision);
            } else {
                line << QString(scan.name().length() + 2, ' ') << QString(11, ' ') << QString(8, ' ') << QString(precision + 6, ' ');
            }
        }

        out += line.join(" ") + QString("\n");
    }

    return out;
}


QString ProfexPgxExport::getDataSep(const QVector<Scan> &scanHeap)
{
    QString out("#SCANS ");

    int rMax = 0;

    for (int i = 0; i < scanHeap.size(); ++i) {
        rMax = qMax(rMax, scanHeap.at(i).size());
    }

    QStringList header;
    for (int c = 0; c < scanHeap.size(); ++c) {
        header << "Diffraction Angle [°2theta]" << scanHeap.at(c).name();
    }

    out += header.join(";") + "\n";

    for (int r = 0; r < rMax; ++r) {
        QStringList line;

        for (int c = 0; c < scanHeap.size(); ++c) {
            const Scan scan = scanHeap.at(c);

            if (r < scan.size()) {
                line << QString("%1").arg(scan.pDataAngle().at(r), 0, 'f', 4)
                     << QString("%1").arg(scan.pDataIntensity().at(r), 0, 'f', precision);
            } else {
                line << QString() << QString();
            }
        }

        out += line.join(";") + QString("\n");
    }

    return out;
}

QString ProfexPgxExport::getHklSep(const QVector<Scan> &scanHeap)
{
    QString out("#HKL ");

    int rMax = 0;
    QList<int> scanIdx;

    for (int i = 0; i < scanHeap.size(); ++i) {
        if (scanHeap.at(i).hasHklData()) {
            scanIdx.append(i);
            rMax = qMax(rMax, scanHeap.at(i).pDataHkl().size());
        }
    }

    QStringList header;
    for (int c = 0; c < scanIdx.size(); ++c) {
        header << "Phase" << "h k l" << "Diffraction Angle [°2theta]" << "Intensity";
    }

    out += header.join(";") + "\n";

    for (int r = 0; r < rMax; ++r) {
        QStringList line;

        for (int c = 0; c < scanIdx.size(); ++c) {
            const Scan scan = scanHeap.at(scanIdx.at(c));
            double wl = scan.waveLength();
            bool cvD = scan.getHklXunit() == "dnm";

            if (r < scan.pDataHkl().size()) {
                const Hkl hkl = scan.pDataHkl().at(r);
                double x = hkl.position();
                double y = hkl.intensity();

                line << scan.name()
                     << hkl.hkl()
                     << QString("%1").arg(cvD ? global::Functions::dToTwoTheta(10.0 * x, wl) : x, 0, 'f', 4)
                     << QString("%1").arg(y, 0, 'f', precision);
            } else {
                line << QString() << QString() << QString() << QString();
            }
        }

        out += line.join(";") + QString("\n");
    }

    return out;
}

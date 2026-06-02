/***************************************************************************
                          asciixyexport.cpp  -  description
                             -------------------
    begin                : Sun Oct 04 14:16:07 CEST 2009
    copyright            : (C) 2009 by Nicola Doebelin
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

#include "gsasstdexport.h"
#include "bgmnfileio.h"

GsasStdExport::GsasStdExport(QObject *parent) :
    GenericExport(parent)
{
    uId = "GSAS_STD";
}

int GsasStdExport::save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &)
{
    bool b = writeToFile(file, getData(scan));
    if (b) return 1;
    return 0;
}

int GsasStdExport::save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &)
{
    bool b = writeToFile(file, getData(scanHeap));
    if (b) return scanHeap.size();
    return 0;
}

/*
 * we need to use a separate writetofile function which forces \r\n line endings
 * on all platforms
 */
bool GsasStdExport::writeToFile(const QString &fname, const QString &content)
{
    QFile f(fname);

    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QTextStream out(&f);
    out << content;
    f.close();

    return true;
}

QString GsasStdExport::getData(const Scan &scan)
{
    return getHeader(scan) + getGlobalComment() + getBankComment(scan) + getBank(scan, 0) + getValues(scan);
}

QString GsasStdExport::getData(const QVector<Scan> &scanHeap)
{
    QString data(getHeader(scanHeap.first()));
    data += getGlobalComment();

    for (int i = 0; i < scanHeap.size(); ++i) {
        data += getBank(scanHeap.at(i), i);
        data += getValues(scanHeap.at(i));
    }

    return data;
}

QString GsasStdExport::getGlobalComment()
{
    QString date(QLocale::system().toString(QDateTime::currentDateTime(), "dd/MM/yyyy"));
    return QString("%1").arg(QString("# exported from Profex on ") + date, -80);
}

QString GsasStdExport::getBankComment(const Scan &scan)
{
    bool ok;
    QString sId = scan.auxInfo("sampleId", ok).toString();
    return QString("%1").arg("# Bank ID: " + sId, -80) + "\r\n";
}

QString GsasStdExport::getHeader(const Scan &scan)
{
    return QString("%1").arg(scan.sourceFileName(), -80) + "\r\n";
}

QString GsasStdExport::getBank(const Scan &scan, int i)
{
    int nchan = qMin(scan.pDataAngle().size(), scan.pDataIntensity().size());
    int nrec = nchan;

    QString line = QString("\r\nBANK %1 %2 %3 CONS %4 %5 0 0 FXY")
            .arg(i + 1)
            .arg(nchan)
            .arg(nrec)
            .arg(100.0 * scan.minAngle(), 0, 'f', 12)
            .arg(100.0 * scan.stepSize(), 0, 'f', 12);

    return QString("%1").arg(line, -80) + "\r\n";
}

QString GsasStdExport::getValues(const Scan &scan)
{
    QStringList values;

    int n = qMin(scan.pDataAngle().size(), scan.pDataIntensity().size());

    for (int i = 0; i < n; ++i) {
        QString line = QString("%1 %2")
                .arg(100.0 * scan.pDataAngle().at(i), 20, 'f', 10)
                .arg(scan.pDataIntensity().at(i), 20, 'f', 8);

        values.append(QString("%1").arg(line, -80));
    }

    return values.join("\r\n");
}

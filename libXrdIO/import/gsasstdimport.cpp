/***************************************************************************
                          asciixyimport.cpp  -  description
                             -------------------
    begin                : Sat Oct 03, 2009
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


#include "gsasstdimport.h"

GsasStdImport::GsasStdImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "fxy" << "fxye";
    descr = QLatin1String("GSAS Standard Powder File");
}

bool GsasStdImport::isSupported(const QByteArray &ba)
{
    static QRegularExpression rx("BANK\\s+\\d+\\s+\\d+\\s+\\d+\\s+CONS\\s+(?:\\d+\\.?\\d*\\s+){4}FXYE?");
    QRegularExpressionMatch rm = rx.match(QString(ba).simplified());

    return rm.hasMatch();
}

int GsasStdImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.isEmpty()) {
        qDebug() << QString("GsasStdImport::load(): No data read from file %1").arg(file);
        return -1;
    }

    QFileInfo fi(file);
    QString sampleId = content.first().simplified();

    static QRegularExpression rx("BANK\\s+\\d+\\s+(\\d+)\\s+.*");

    int idx = content.indexOf(rx, 1);
    int n = 0;

    while (idx >= 0) {
        QRegularExpressionMatch rm = rx.match(content.at(idx));
        int len = rm.captured(1).toInt();
        qDebug() << QString("GsasStdImport::load(): Found bank at line %1. Parsing %2 records").arg(idx).arg(len);

        QVector<double> vec_a;
        QVector<double> vec_i;

        getBankData(content.mid(idx, len), vec_a, vec_i);

        QString label = QString("%1 Data %2").arg(fi.fileName()).arg(n+1);

        Scan scan(label, QColor(), 1);
        scan.setSourceFileName(file);
        scan.setDataAng(vec_a);
        scan.setDataInt(vec_i);
        scan.setWaveLength(0.0);
        scan.setAuxInfo("sampleId", QVariant(sampleId));
        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);
        ++n;

        idx = content.indexOf(rx, idx + len);
    }

    qDebug() << QString("GsasStdImport::load(): No more banks found. Read %1 banks in total").arg(n);

    return n;
}

void GsasStdImport::getBankData(const QStringList &bank, QVector<double> &vec_a, QVector<double> &vec_i)
{
    static QRegularExpression rxSplit("\\s+");

    for (int i = 0; i < bank.size(); ++i) {
        QStringList l = bank.at(i).trimmed().split(rxSplit);

        if (l.size() > 1) {
            bool ok_a, ok_b;
            double ang = l.at(0).toDouble(&ok_a) / 100.0;
            double cts = l.at(1).toDouble(&ok_b);

            if (ok_a && ok_b) {
                vec_a.push_back(ang);
                vec_i.push_back(cts);
            }
        }
    }
}

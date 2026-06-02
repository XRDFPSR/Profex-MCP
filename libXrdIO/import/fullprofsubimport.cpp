/***************************************************************************
                          FullprofSubImport.h  -  description
                             -------------------
    begin                : Wed Sept 25, 2013
    copyright            : (C) 2013 by Nicola Doebelin
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


#include "fullprofsubimport.h"

FullprofSubImport::FullprofSubImport(QObject *parent)
    : GenericImport(parent)
{
    extens << "sub";
    descr = QLatin1String("Fullprof Sub Phase pattern");
}

bool FullprofSubImport::isSupported(const QByteArray &ba)
{
    QRegularExpression rx("^!\\s+Phase\\s+No:\\s+\\d+");

    QRegularExpressionMatch rm = rx.match(ba.left(25));
    return rm.hasMatch();
}

int FullprofSubImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.size() < 3) {
        qDebug() << QString("FullprofSubImport::load(): No data found in file %1").arg(file);
        return -1;
    }

    QFileInfo fi(file);
    Scan scan(fi.completeBaseName(), QColor(), 1);
    scan.setSourceFileName(file);

    QVector<double> vec_a;
    QVector<double> vec_i;

    QRegularExpression rx("^!\\s+Phase\\s+No:\\s+\\d+\\s+(.*)$");
    QRegularExpressionMatch rm = rx.match(content.at(0));

    if (rm.hasMatch()) {
        scan.setName(rm.captured(1).simplified());
    }

    // loop over all lines and read the data
    for (int i = 1; i < content.size(); ++i) {

        QStringList line = content.at(i).split(" ", Qt::SkipEmptyParts);

        if (line.size() < 2) {
            continue;
        }

        vec_a.push_back(line.at(0).toDouble());
        vec_i.push_back(line.at(1).toDouble());
    }

    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);
    scan.setTypes(Scan::XY | Scan::REFINED | Scan::PHASE | Scan::ISABOVEBACKGROUND);
    scanHeap.push_back(scan);

    return 1;
}

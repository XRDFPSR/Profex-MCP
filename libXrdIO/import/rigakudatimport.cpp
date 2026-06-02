/***************************************************************************
                          rigakudatimport.cpp  -  description
                             -------------------
    begin                : Thu July 21, 2013
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

#include "rigakudatimport.h"

RigakuDatImport::RigakuDatImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "dat" << "rig";
    descr = QLatin1String("Rigaku DAT/RIG scan");
}

bool RigakuDatImport::isSupported(const QByteArray &ba)
{
    return ba.left(5) == "*TYPE";
}

int RigakuDatImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    qDebug() << QString("RigakuDatImport::load(): Loading file %1").arg(file);
    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.size() < 4) {
        qDebug() << QString("RigakuDatImport::load(): Insufficient lines read from file %1").arg(file);
        return -1;
    }

    QRegularExpression rxParam("(^\\*\\S+)(\\s+=\\s+)(.+)");
    QRegularExpression rxValue("(\\d+[\\s,]+)+");
    QRegularExpression rxSplitValue("[\\s,]+");
    QRegularExpressionMatch rm;

    QString name;
    QString comment;
    int groupCount = 0;
    double lambda1 = 1.540598;
    double lambda2 = 1.544426;

    // First we search some parameters in the file header
    for (int i = 0; i < content.size(); ++i) {
        // here the header ends and the first data block begins
        if (content.at(i).trimmed() == "*BEGIN") {
            break;
        }

        rm = rxParam.match(content.at(i).trimmed());

        // check if a line of type "*PARAM = xxxx" was read
        if (!rm.hasMatch()) {
            continue;
        }

        if (rm.captured(1) == "*SAMPLE") {
            name = rm.captured(3);
        }

        if (rm.captured(1) == "*COMMENT") {
            comment = rm.captured(3);
        }

        if (rm.captured(1) == "*GROUP_COUNT") {
            groupCount = rm.captured(3).toInt();
        }

        if (rm.captured(1) == "*WAVE_LENGTH1") {
            lambda1 = rm.captured(3).toDouble();
        }

        if (rm.captured(1) == "*WAVE_LENGTH2") {
            lambda2 = rm.captured(3).toDouble();
        }
    }

    // now lets find the data blocks
    QList<int> blocks;
    int i = content.indexOf(QRegularExpression("^\\*BEGIN\\s*$"), 0);

    while (i > -1) {
        blocks.append(i);
        i = content.indexOf(QRegularExpression("^\\*BEGIN\\s*$"), i+1);
    }

    // just a cross check: the GROUP_COUNT parameter and the number of blocks should be the same
    // but to avoid overrunning the stringlist, check again
    groupCount = qMin(groupCount, blocks.size());

    // now we loop over the group block
    for (int n = 0; n < blocks.size(); ++n) {
        int b = blocks.at(n);

        if (b > content.size()) {
            qDebug() << QString("RigakuDatImport::load(): trying to read beyond the end of file $1").arg(file);
            // we might have read some scans already, so let's return them
            return scanHeap.size();
        }

        Scan scan(name, QColor(), 1);
        scan.setSourceFileName(file);

        if (groupCount > 1) {
            scan.setName(QString("%1-%2").arg(name).arg(n));
        }

        scan.setWaveLength(lambda1);
        scan.setWaveLength2(lambda2);

        QVector<double> vec_a;
        QVector<double> vec_i;
        double startang = 0.0;
        double stepsize = 0.0;
        int count = 0;

        QString line = QString();

        // loop over the group block
        while ((line != "*END") && (b < content.size())) {
            line = content.at(b).trimmed();

            rm = rxParam.match(line);

            // if yes, extract parameters
            if (rm.hasMatch()) {
                if (rm.captured(1) == "*START") {
                    startang = rm.captured(3).toDouble();
                }

                if (rm.captured(1) == "*STEP") {
                    stepsize = rm.captured(3).toDouble();
                }

                if (rm.captured(1) == "*COUNT") {
                    count = rm.captured(3).toInt();
                    Q_UNUSED(count);
                }

                // no need to parse the rest of the block if we already know that it is a parameter line
                b++;
                continue;
            }

            rm = rxValue.match(line);

            // if yes, read the values
            if (rm.hasMatch()) {
                QStringList vals = line.split(rxSplitValue);
                double endang = startang + vals.size() * stepsize;
                int p = vals.size();

                for (int j = 0; j < p; ++j) {
                    double ang = startang + (double(j) / double(p - 1)) * (endang - startang);
                    double its = vals.at(j).toDouble();

                    vec_a.push_back(ang);
                    vec_i.push_back(its);
                }
            }

            b++;
        }

        scan.setDataAng(vec_a);
        scan.setDataInt(vec_i);
        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);
    }

    return scanHeap.size();
}

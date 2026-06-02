/***************************************************************************
                          eflechparparser.cpp  -  description
                             -------------------
    begin                : Jan 19 12:15:00 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#include "eflechparparser.h"
#include "bgmnfileio.h"

#include <QRegularExpression>

EflechParParser::EflechParParser()
{

}

EflechParParser::EflechParParser(const QString &s)
{
    load(s);
}

bool EflechParParser::load(const QString &s)
{
    content.clear();
    content = BgmnFileIO::readTextFileLines(s);
    return content.size() ? true : false;
}

QVector<QVector<double> > EflechParParser::getPeakData()
{
    QVector<QVector<double> > vec = QVector<QVector<double> >(4, QVector<double>());

    if (content.size() < 2) {
        qDebug() << QString("EflechParParser::getPeakData(): No data found");
        return vec;
    }

    QString rxExp("-?\\d+\\.\\d+E[\\+-]\\d+");
    QString rxFlt("-?\\d+\\.\\d+");

    QRegularExpression rxPz("PEAKZAHL=(\\d+)");
    QRegularExpression rxLine(QString("^(\\d)\\s+([\\s\\d\\.E\\+-]+)\\s+GSUM=%1$").arg(rxFlt));
    QRegularExpression rxRp2(QString("(%1)\\s+(%2)").arg(rxExp).arg(rxFlt));
    QRegularExpression rxRp3(QString("(%1)\\s+(%2)\\s+(%2)").arg(rxExp).arg(rxFlt));
    QRegularExpression rxRp4(QString("(%1)\\s+(%2)\\s+(%2)\\s+(%2)").arg(rxExp).arg(rxFlt));

    QRegularExpressionMatch rmPz = rxPz.match(content.at(0));

    if (!rmPz.hasMatch()) {
        qDebug() << QString("EflechParParser::getPeakData(): Peakzahl not found");
        return vec;
    }

    int n = rmPz.captured(1).toInt();

    for (int i = 1; i < n + 1; ++i) {
        if (i >= content.size()) break;

        QRegularExpressionMatch rmLine = rxLine.match(content.at(i));

        if (rmLine.hasMatch()) {
            int rp = rmLine.captured(1).toInt();
            double intens = 0.0;
            double dinv = 0.0;
            double d = 0.0;
            double b1 = 0.0;
            double b2 = 0.0;

            if (rp == 2) {
                QRegularExpressionMatch rmValues = rxRp2.match(rmLine.captured(2));
                intens = rmValues.captured(1).toDouble();
                dinv = rmValues.captured(2).toDouble();
                d = qFuzzyIsNull(dinv) ? 0.0 : 1.0 / dinv;
            }

            if (rp == 3) {
                QRegularExpressionMatch rmValues = rxRp3.match(rmLine.captured(2));
                intens = rmValues.captured(1).toDouble();
                dinv = rmValues.captured(2).toDouble();
                d = qFuzzyIsNull(dinv) ? 0.0 : 1.0 / dinv;
                b1 = rmValues.captured(3).toDouble();
            }

            if (rp == 4) {
                QRegularExpressionMatch rmValues = rxRp4.match(rmLine.captured(2));
                intens = rmValues.captured(1).toDouble();
                dinv = rmValues.captured(2).toDouble();
                d = qFuzzyIsNull(dinv) ? 0.0 : 1.0 / dinv;
                b1 = rmValues.captured(3).toDouble();
                b2 = std::sqrt(rmValues.captured(4).toDouble());
            }

            vec[0].append(d);
            vec[1].append(intens);
            vec[2].append(b1);
            vec[3].append(b2);
        }
    }

    return vec;
}

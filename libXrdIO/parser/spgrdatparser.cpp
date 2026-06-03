/***************************************************************************
                          spgrdatparser.cpp  -  description
                             -------------------
    begin                : Tue Jul 19 10:54:00 CEST 2016
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

#include "spgrdatparser.h"
#include <QFile>
#include <QStringList>
#include <QDebug>
#include <QRegularExpression>

SpgrDatParser::SpgrDatParser()
{
    QFile f(":/resources/spacegrp.xml");
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    spaceGroupDat.setContent(f.readAll());
    f.close();
}

QList<QStringList> SpgrDatParser::getSymOps(const QString &hm, int s)
{
    QDomNode setting(settingNodes(hm, s));

    if (setting.isNull()) {
        qDebug() << QString("SpgrDatParser::getSymOps(): Could not find setting node for HermannMauguin=%1 Setting=%2").arg(hm).arg(s);
        QList<QStringList>();
    }

    QStringList sops(symOps(setting));
    return splitSymOps(sops);
}

QDomNode SpgrDatParser::settingNodes(const QString &hm, int s)
{
    QDomNodeList allSettings = spaceGroupDat.elementsByTagName("setting");

    for (int i = 0; i < allSettings.size(); ++i) {
        QString chm = allSettings.at(i).toElement().attribute("HermannMauguin");
        int cs = allSettings.at(i).toElement().attribute("Number").toInt();

        if (s < 1) cs = s; // if setting 0 or <0 is requested, we ignore the setting and only compare HM symbols

        if ((chm.simplified() == hm.simplified()) && (cs == s)) {
            return allSettings.at(i);
        }
    }

    return QDomNode();
}

QStringList SpgrDatParser::symOps(const QDomNode &setting)
{
    QDomNodeList wyckoffs = setting.toElement().elementsByTagName("wyckoff");

    for (int i = 0; i < wyckoffs.size(); ++i) {
        QStringList posOps = positions(wyckoffs.at(i));

        if (posOps.contains("x y z")) {
            return posOps;
        }
    }

    return QStringList();
}

QStringList SpgrDatParser::positions(const QDomNode &wyck)
{
    QStringList pos;
    QDomNodeList posNodes = wyck.toElement().elementsByTagName("position");

    for (int i = 0; i < posNodes.size(); ++i) {
        pos.append(posNodes.at(i).toElement().text());
    }

    return pos;
}

QList<QStringList> SpgrDatParser::splitSymOps(const QStringList &sops)
{
    QList<QStringList> lops;

    for (int i = 0; i < sops.size(); ++i) {
        QStringList splitList(sops.at(i).toUpper().split(QRegularExpression("[\\s,]+"), Qt::KeepEmptyParts));

        // guarantee that splitList always contains 3 elements
        while (splitList.size() < 3) {
            splitList << QString();
        }

        lops.append(splitList);
    }

    return lops;
}

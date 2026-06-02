/***************************************************************************
                          genericimport.cpp  -  description
                             -------------------
    begin                : Thu June 01, 2013
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

#include "genericimport.h"

GenericImport::GenericImport(QObject *parent) :
    QObject(parent)
{
    defaultWl = 1.54056;
}

QString GenericImport::filter()
{
    QStringList allExt;

    if (!extens.size()) return QString();

    for (int i = 0; i < extens.size(); ++i) {
        allExt << extens.at(i).toLower();
        allExt << extens.at(i).toUpper();
    }

    return QString("%1 (*.%2)").arg(descr, allExt.join(" *."));
}

QStringList GenericImport::extensions()
{
    return extens;
}

QString GenericImport::description()
{
    return descr;
}

QString GenericImport::baToString(const QByteArray &ba, int start, int len)
{
    QString s = ba.mid(start, len);
    return s.remove('\0').trimmed();
}

/***************************************************************************
                          optionsconfigpageblank.cpp  -  description
                             -------------------
    begin                : Tue Jul 16 18:00:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "opticsconfigpageblank.h"

OpticsConfigPageBlank::OpticsConfigPageBlank(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent)
{
}

QMap<QString, QString> OpticsConfigPageBlank::setParameters(const QMap<QString, QString> &)
{
    return QMap<QString, QString>();
}

QMap<QString, QString> OpticsConfigPageBlank::getParameters()
{
    return QMap<QString, QString>();
}

QString OpticsConfigPageBlank::helpText()
{
    return QString();
}

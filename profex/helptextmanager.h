/***************************************************************************
                          helptextmanager.h  -  description
                             -------------------
    begin                : Mon Jul 31 18:00:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#ifndef HELPTEXTMANAGER_H
#define HELPTEXTMANAGER_H

#include <QMap>
#include <QString>

class HelpTextManager
{
public:
    HelpTextManager();

    QString getHelpText(const QString &module);

private:
    QMap<QString, QString> _helpTextBuffer;

    void parseHelpFile (const QString &module);
    QString translateToMac(const QString &);
};

#endif // HELPTEXTMANAGER_H

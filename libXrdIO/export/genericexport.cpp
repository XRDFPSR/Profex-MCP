/***************************************************************************
                          genericexport.cpp  -  description
                             -------------------
    begin                : Thu June 17, 2013
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

#include "genericexport.h"

GenericExport::GenericExport(QObject *parent) :
    QObject(parent)
{
}

int GenericExport::writeFile(const QString &file, const QString &str)
{
    QFile outfile(file);

    if (!outfile.open(QIODevice::WriteOnly))
    {
        qDebug() << QString("GenericExport::writeFile: Could not open file for writing: %1").arg(file);
        return 0;
    }

    QTextStream stream(&outfile);
    stream << str;
    outfile.close();

    return 1;
}

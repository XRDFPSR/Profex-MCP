/***************************************************************************
                          oqprojecthandler.h  -  description
                             -------------------
    begin                : Thu Mar 02 18:00:00 CEST 2023
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

#ifndef OQPROJECTHANDLER_H
#define OQPROJECTHANDLER_H

#include <QString>

class OqProjectHandler
{
public:
    OqProjectHandler();

    bool createProject(const QString &name, const QString &targetDir, QString &scanFile, QString &filter, QString &errors);
    bool createEmptyProject(const QString &name, const QString &targetDir, QString &scanFile, QString &filter, QString &errors);

private:
};

#endif // OQPROJECTHANDLER_H

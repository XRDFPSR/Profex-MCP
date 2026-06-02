/***************************************************************************
                          codlocalcifmanager.h  -  description
                             -------------------
    begin                : Thu Jul 20 18:30:00 CEST 2023
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

#ifndef CODLOCALCIFMANAGER_H
#define CODLOCALCIFMANAGER_H

#include "codciffilemanager.h"

class CodLocalCifManager : public CodCifFileManager
{
public:
    explicit CodLocalCifManager(QObject *parent = nullptr);

    void startCifDownload(const QStringList &, const QString &);

private:
    QString findFile(const QString &, const QString &);

public slots:
    void cancel();
};

#endif // CODLOCALCIFMANAGER_H

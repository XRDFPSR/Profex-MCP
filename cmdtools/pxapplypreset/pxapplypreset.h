/***************************************************************************
                          pxapplypreset.h  -  description
                             -------------------
    begin                : Tue Nov 24 19:42:15 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#include <QString>

class PxApplyPreset {

public:
    PxApplyPreset(const QString &scan, const QString &preset, const QString &sampleId, bool force = false);

    static void listPresets(const QString &d);

private:
    QString adjustOutputFiles(const QString &controlFile, const QString &content, const QString &scanFile, const QString &sampleId);
};

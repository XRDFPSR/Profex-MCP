/***************************************************************************
                          indexedhklstructure.cpp  -  description
                             -------------------
    begin                : Sun Oct 31 13:14:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#include "indexedhklstructure.h"

IndexedHklStructure::IndexedHklStructure()
{
    reset();
}

void IndexedHklStructure::reset()
{
    _hklData = HklPhaseData();
    _strFile = QFileInfo();
    _parFile = QFileInfo();
    _lstFile = QFileInfo();
    _doRemove = false;
    _message = QString();
}

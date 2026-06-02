/***************************************************************************
                          crystalstructurefactor.h  -  description
                             -------------------
    begin                : Sat Aug 26 14:28:00 CEST 2019
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

#include "crystalstructurefactor.h"

CrystalStructureFactor::CrystalStructureFactor()
{
}

CrystalStructureFactor::CrystalStructureFactor(int h, int k, int l, double i, double d)
    : _h(h), _k(k), _l(l), _intensity(i), _dnm(d)
{
}

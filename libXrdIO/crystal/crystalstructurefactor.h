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

#ifndef CRYSTALSTRUCTUREFACTOR_H
#define CRYSTALSTRUCTUREFACTOR_H

#include <QObject>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT CrystalStructureFactor
{
public:
    CrystalStructureFactor();
    CrystalStructureFactor(int h, int k, int l, double i, double d);

    inline int h()            const {return _h;}
    inline int k()            const {return _k;}
    inline int l()            const {return _l;}
    inline double intensity() const {return _intensity;}
    inline double d_nm()      const {return _dnm;}

private:
    int _h;
    int _k;
    int _l;
    double _intensity;
    double _dnm;
};

#endif // CRYSTALSTRUCTUREFACTOR_H

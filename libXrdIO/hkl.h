/***************************************************************************
                          hkl.h  -  description
                             -------------------
    begin                : Wed Mar 04, 2009
    copyright            : (C) 2009 by Nicola Doebelin
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

#ifndef HKL_H
#define HKL_H

#include <QString>
#include <QColor>
#include <QByteArray>
#include <QJsonObject>
#include <QUuid>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

/*
 * The position can either be given in 2theta [degrees], or in d [nm].
 * Set the property Scan::setHklXunit() accordingly:
 *
 * Scan::setHklXunit("dnm") for d [nm]
 * Scan::setHklXunit("tt")  for 2theta [degrees]
 *
 * default in Scan is 2theta [degrees].
 */

/*
 * Status:
 * 0: invisible
 * 1: normal
 * 2: highlighted
 * >2: to be defined
 */

class XRDIO_EXPORT Hkl
{
	public:
        Hkl();
        Hkl(const Hkl &);
        Hkl(double pos, const QString &hkl, int vpos, const QString &ph, const QColor &c, double intens = 1.0, double tex = 1.0, int stat = 1, double b1 = 0.0, double b2 = 0.0, double gs = 1.0);
        Hkl(const QString &, const QString &, const QJsonObject &);
        Hkl operator=(const Hkl &);

        inline QUuid uid()        const {return _uid;}
        inline double position()  const {return _pos;}
        inline QString hkl()      const {return _hkl;}
        inline int vPosition()    const {return _vpos;}
        inline QString phase()    const {return _phase;}
        inline QColor color()     const {return _col;}
        inline double intensity() const {return _intensity;}
        inline double texture()   const {return _texture;}
        inline int status()       const {return _status;}
        inline double B1()        const {return _b1;}
        inline double B2()        const {return _b2;}
        inline double gsum()      const {return _gsum;}
        QJsonObject getJsonData() const;

        inline void setIntensity(double d) {_intensity = d;}
        inline void setStatus(int i) {_status = i;}
        inline void setB1(double d)  {_b1 = d;}
        inline void setB2(double d)  {_b2 = d;}

        inline bool isEmpty() const {return qFuzzyIsNull(_pos) ? true : false;}

	private:
		double _pos;
		int _vpos;
        QString _hkl;
        QString _phase;
        QColor _col;
        double _intensity;
        double _texture;
        QString _splitter;
        int _status;
        double _b1;
        double _b2;
        QUuid _uid;
        double _gsum;
};

#endif

/***************************************************************************
                          elementscatteringdata.h  -  description
                             -------------------
    begin                : Wed Feb 10 19:16:07 CET 2021
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

#ifndef ELEMENTSCATTERINGDATA_H
#define ELEMENTSCATTERINGDATA_H

#include <QVector>
#include <QString>
#include <QMap>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT ElementScatteringData
{
public:
    ElementScatteringData();
    ElementScatteringData(const QString &, int);
    ElementScatteringData(const ElementScatteringData &);
    ElementScatteringData operator=(const ElementScatteringData &);

    inline QString element() const {return m_element;}
    inline int     z()       const {return m_z;}
    inline const QVector<double> & dataEnergy()  const {return m_energy;}
    inline const QVector<double> & dataF0()      const {return m_f0;}
    inline const QVector<double> & dataF1()      const {return m_f1;}
    inline const QVector<double> & dataF2()      const {return m_f2;}
    inline const QVector<double> & dataMac()     const {return m_mac;}
    inline const QVector<double> & dataLac()     const {return m_lac;}

    inline QVector<double> & dataEnergy()        {return m_energy;}
    inline QVector<double> & dataF0()            {return m_f0;}
    inline QVector<double> & dataF1()            {return m_f1;}
    inline QVector<double> & dataF2()            {return m_f2;}
    inline QVector<double> & dataMac()           {return m_mac;}
    inline QVector<double> & dataLac()           {return m_lac;}

    QByteArray serialize() const;
    void deserialize(const QByteArray &);

private:
    QString m_element;
    int m_z;
    QVector<double> m_energy;
    QVector<double> m_f0;
    QVector<double> m_f1;
    QVector<double> m_f2;
    QVector<double> m_mac;
    QVector<double> m_lac;
};

#endif // ELEMENTSCATTERINGDATA_H

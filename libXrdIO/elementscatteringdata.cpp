/***************************************************************************
                          elementscatteringdata.cpp  -  description
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

#include "elementscatteringdata.h"
#include <QDataStream>
#include <QIODevice>

ElementScatteringData::ElementScatteringData()
{
    m_element = QString();
    m_z = 0;
}

ElementScatteringData::ElementScatteringData(const QString &_e, int _z)
    : m_element(_e), m_z(_z)
{
}

ElementScatteringData::ElementScatteringData(const ElementScatteringData &e)
    : m_element(e.m_element),
      m_z(e.m_z),
      m_energy(e.m_energy),
      m_f0(e.m_f0),
      m_f1(e.m_f1),
      m_f2(e.m_f2),
      m_mac(e.m_mac),
      m_lac(e.m_lac)
{
}

ElementScatteringData ElementScatteringData::operator =(const ElementScatteringData &e)
{
    m_element = e.m_element;
    m_z = e.m_z;
    m_energy = e.m_energy;
    m_f0 = e.m_f0;
    m_f1 = e.m_f1;
    m_f2 = e.m_f2;
    m_mac = e.m_mac;
    m_lac = e.m_lac;

    return *this;
}

QByteArray ElementScatteringData::serialize() const
{
    QByteArray ba;

    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_9);

    stream << m_element
           << m_z
           << m_energy
           << m_f0
           << m_f1
           << m_f2
           << m_mac
           << m_lac;

    return ba;
}

void ElementScatteringData::deserialize(const QByteArray &ba)
{
    QDataStream stream(ba);
    stream.setVersion(QDataStream::Qt_5_9);

    stream >> m_element
           >> m_z
           >> m_energy
           >> m_f0
           >> m_f1
           >> m_f2
           >> m_mac
           >> m_lac;
}

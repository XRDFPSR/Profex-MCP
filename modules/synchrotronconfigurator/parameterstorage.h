/***************************************************************************
                          parameterstorage.h  -  description
                             -------------------
    begin                : Thu Feb 20 18:00:00 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#ifndef PARAMETERSTORAGE_H
#define PARAMETERSTORAGE_H

#include <QHash>
#include <QString>
#include <QVariant>
#include "structssc.h"

class ParameterStorage
{
public:
    ParameterStorage();

    inline void setValue(synchro::ParameterKey k, const QString & v) {_data[k] = v;}
    inline void setValue(synchro::ParameterKey k, double v)          {_data[k] = v;}
    inline void setValue(synchro::ParameterKey k, int v)             {_data[k] = v;}
    inline void setValue(synchro::ParameterKey k, bool v)            {_data[k] = v;}

    template<typename T>
    T getParameter(synchro::ParameterKey key, const T &defaultValue = T()) const;

    const QHash<synchro::ParameterKey, QVariant> & getData() const {return _data;}
    void update(const ParameterStorage *);
    inline bool isEmpty() const {return _data.isEmpty();}
    inline bool contains(synchro::ParameterKey k) const {return _data.contains(k);}

private:
    QHash<synchro::ParameterKey, QVariant> _data;
};

#endif // PARAMETERSTORAGE_H

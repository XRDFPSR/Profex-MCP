/***************************************************************************
                          parameterstorage.cpp  -  description
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

#include "parameterstorage.h"

ParameterStorage::ParameterStorage() {}

template<typename T>
T ParameterStorage::getParameter(synchro::ParameterKey key, const T &defaultValue) const {
    QVariant var = _data.value(key);
    return var.canConvert<T>() ? var.value<T>() : defaultValue;
}

// Explicit instantiations for the types you need:
template QString ParameterStorage::getParameter<QString>(synchro::ParameterKey key, const QString &defaultValue) const;
template double ParameterStorage::getParameter<double>(synchro::ParameterKey key, const double &defaultValue) const;
template int ParameterStorage::getParameter<int>(synchro::ParameterKey key, const int &defaultValue) const;
template bool ParameterStorage::getParameter<bool>(synchro::ParameterKey key, const bool &defaultValue) const;

void ParameterStorage::update(const ParameterStorage *h)
{
    if (!h) return;
    _data.insert(h->getData());
}

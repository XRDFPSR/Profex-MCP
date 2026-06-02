/***************************************************************************
                          chemtablestruct.h  -  description
                             -------------------
    begin                : Wed Oct 08 11:30:00 CEST 2025
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

#ifndef CHEMTABLESTRUCT_H
#define CHEMTABLESTRUCT_H

#include <QStringList>
#include <QHash>
#include <QList>
#include <limits>

#if defined XRDIO
#define XRDIO_EXPORT Q_DECL_EXPORT
#else
#define XRDIO_EXPORT Q_DECL_IMPORT
#endif

enum ChemistryMode {UNKNOWN = -1, // set new enum to -1 to maintain backwards compatibility
                    ELEMENT = 0,
                    ATOMIC  = 1,
                    OXIDE   = 2};

struct XRDIO_EXPORT ChemCell {
    double mean = std::numeric_limits<double>::quiet_NaN();
    double esd  = std::numeric_limits<double>::quiet_NaN();
};

class XRDIO_EXPORT ChemTableStruct {
public:
    ChemTableStruct(const ChemistryMode &m = UNKNOWN);

    void clear();
    inline void setPhaseLabels(const QStringList &l)                {_phaseLabels = l;}
    inline void setElementLabels(const QStringList &l)              {_elementLabels = l;}
    inline void setPhaseQuantities(const QHash<QString, double> &h) {_phaseQuantities = h;}

    inline QStringList phaseLabels()   const {return _phaseLabels;}
    inline QStringList elementLabels() const {return _elementLabels;}

    void addValue(const QString &ph, const QString &el, double v);
    void addEsd(const QString &ph, const QString &el, double v);

    inline double getPhaseQuantity(const QString &p) const {return _phaseQuantities.value(p, -1.0);}
    double getValue(int r, int c) const;
    double getValue(const QString &p, const QString &e) const;
    double getEsd(int r, int c) const;
    double getEsd(const QString &p, const QString &e) const;
    double getTotal(int c) const;
    double getTotal(const QString &e) const;
    double getSumQuantities() const;

    bool hasData() const;

private:
    ChemistryMode _mode;
    QStringList _phaseLabels;
    QStringList _elementLabels;
    QHash<QString, QHash<QString, double> > _values; // QHash<Phase, QHash<element, value> >
    QHash<QString, QHash<QString, double> > _esds;   // QHash<Phase, QHash<element, esd> >
    QHash<QString, double> _phaseQuantities;
};

#endif // CHEMTABLESTRUCT_H

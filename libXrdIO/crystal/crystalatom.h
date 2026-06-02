/***************************************************************************
                          crystalatom.h  -  description
                             -------------------
    begin                : Sat Aug 22 08:53:00 CEST 2015
    copyright            : (C) 2015 by Nicola Doebelin
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

#ifndef CRYSTALATOM_H
#define CRYSTALATOM_H

#include <QObject>
#include <QString>
#include <QVariant>
#include "crystalunitcell.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT CrystalAtom
{
public:
    CrystalAtom();
    CrystalAtom(const CrystalAtom &a);
    CrystalAtom(const QString &e, double x, double y, double z,
                double o = 1.0,
                double t = 0.0);

    void operator=(const CrystalAtom &);
    bool operator==(const CrystalAtom &) const;

    inline QString name() const          {return _name;}
    inline QString element() const       {return _element;}
    inline QString wyckoff() const       {return _wyckoff;}
    inline double biso() const           {return _tds;}
    inline double biso_esd() const       {return _tds_esd;}
    inline double occupancy() const      {return _occ;}
    inline double occupancy_esd() const  {return _occ_esd;}
    inline int multiplicity() const      {return _multiplicity;}
    void setAuxInfo(const QString &, const QVariant &);
    QVariant auxInfo(const QString &, const QVariant &d = QVariant()) const;

    void fcoordinates(double &, double &, double &) const;
    void fcoords_esd(double &, double &, double &) const;

    void baniso(double &, double &, double &, double &, double &, double &) const;
    void baniso_esd(double &, double &, double &, double &, double &, double &) const;
    void uaniso(double &, double &, double &, double &, double &, double &) const;
    void uaniso_esd(double &, double &, double &, double &, double &, double &) const;

    bool has_fcoords_esd();
    bool has_baniso();
    bool has_baniso_esd();
    bool has_uaniso();
    bool has_uaniso_esd();

    inline double & x()                         {return _fcoords[0];}
    inline double x() const                     {return _fcoords[0];}
    inline double & y()                         {return _fcoords[1];}
    inline double y() const                     {return _fcoords[1];}
    inline double & z()                         {return _fcoords[2];}
    inline double z() const                     {return _fcoords[2];}

    inline double x_esd() const                 {return _fcoords_esd[0];}
    inline double y_esd() const                 {return _fcoords_esd[1];}
    inline double z_esd() const                 {return _fcoords_esd[2];}

    inline void setName(const QString &s)       {_name = s;}
    inline void setElement(const QString &s)    {_element = s;}
    inline void setWyckoff(const QString &s)    {_wyckoff = s;}
    inline void setBiso(double d)               {_tds = d;}
    inline void setBiso_esd(double d)           {_tds_esd = d;}
    inline void setOccupancy(double d)          {_occ = d;}
    inline void setOccupancy_esd(double d)      {_occ_esd = d;}
    inline void setMultiplicity(int i)          {_multiplicity = i;}

    inline void setFcoord_x(double d)           {_fcoords[0] = d;}
    inline void setFcoord_y(double d)           {_fcoords[1] = d;}
    inline void setFcoord_z(double d)           {_fcoords[2] = d;}
    inline void setFcoord_x_esd(double d)       {_fcoords_esd[0] = d;}
    inline void setFcoord_y_esd(double d)       {_fcoords_esd[1] = d;}
    inline void setFcoord_z_esd(double d)       {_fcoords_esd[2] = d;}

    void setFcoordinates(double, double, double);
    void setFcoords_esd(double, double, double);

    void setBaniso(double, double, double, double, double, double);
    void setBaniso_esd(double, double, double, double, double, double);
    void setUaniso(double, double, double, double, double, double);
    void setUaniso_esd(double, double, double, double, double, double);

private:
    QString _name;
    QString _element;

    double _occ;
    double _occ_esd;
    double _tds;
    double _tds_esd;

    QString _wyckoff;
    int _multiplicity;

    double _fcoords[3];
    double _fcoords_esd[3];
    double _baniso[6];
    double _baniso_esd[6];
    double _uaniso[6];
    double _uaniso_esd[6];

    QMap<QString, QVariant> _auxInfo;
};

#endif // CRYSTALATOM_H

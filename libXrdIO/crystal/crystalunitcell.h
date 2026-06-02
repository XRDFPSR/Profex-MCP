/***************************************************************************
                          crystalunitcell.h  -  description
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

#ifndef CRYSTALUNITCEL_H
#define CRYSTALUNITCEL_H

#include <QObject>
#include <QString>
#include <QMatrix4x4>
#include <QVector3D>
#include "crystalsymop.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

enum unitLength {AA, NM};

class XRDIO_EXPORT CrystalUnitCell
{
public:
    CrystalUnitCell();

    void setAxes(double a,         double b = -1.0,  double c = -1.0,
                 double ea = -1.0, double eb = -1.0, double ec = -1.0);

    void setAngles(double al = -1.0,  double be = -1.0,  double ga = -1.0,
                   double eal = -1.0, double ebe = -1.0, double ega = -1.0);

    void setCell(double a,          double b = -1.0,   double c = -1.0,
                 double al = -1.0,  double be = -1.0,  double ga = -1.0,
                 double ea = -1.0,  double eb = -1.0,  double ec = -1.0,
                 double eal = -1.0, double ebe = -1.0, double ega = -1.0);

    inline void setA(double d)                      {_axes[0] = d;}
    inline void setB(double d)                      {_axes[1] = d;}
    inline void setC(double d)                      {_axes[2] = d;}

    inline void setA_esd(double d)                  {_axes_esd[0] = d;}
    inline void setB_esd(double d)                  {_axes_esd[1] = d;}
    inline void setC_esd(double d)                  {_axes_esd[2] = d;}

    inline void setAlpha(double d)                  {_angles[0] = d;}
    inline void setBeta(double d)                   {_angles[1] = d;}
    inline void setGamma(double d)                  {_angles[2] = d;}

    inline void setAlpha_esd(double d)              {_angles_esd[0] = d;}
    inline void setBeta_esd(double d)               {_angles_esd[1] = d;}
    inline void setGamma_esd(double d)              {_angles_esd[2] = d;}

    inline void setAxisUnit(unitLength u)           {_axis_unit = u;}

    void setSpaceGroupHMBgmn(const QString &);
    void setSpaceGroupHMCif(const QString &);
    void setItNumber(int);
    void setSettingNumber(int);

    int setSymmetryOperations(const QList<QStringList> &);

    bool axes(double &, double &, double &);
    bool angles(double &, double &, double &);
    bool axes_esd(double &, double &, double &);
    bool angles_esd(double &, double &, double &);

    bool axes_cartesian(double &, double &, double &,
                        double &, double &, double &,
                        double &, double &, double &);

    inline double a() const                         {return _axes[0];}
    inline double b() const                         {return _axes[1];}
    inline double c() const                         {return _axes[2];}
    inline double a_esd() const                     {return _axes_esd[0];}
    inline double b_esd() const                     {return _axes_esd[1];}
    inline double c_esd() const                     {return _axes_esd[2];}

    inline double alpha() const                     {return _angles[0];}
    inline double beta()  const                     {return _angles[1];}
    inline double gamma() const                     {return _angles[2];}
    inline double alpha_esd() const                 {return _angles_esd[0];}
    inline double beta_esd()  const                 {return _angles_esd[1];}
    inline double gamma_esd() const                 {return _angles_esd[2];}
    inline double volume() const                    {return _vol;}
    inline unitLength axisUnit() const              {return _axis_unit;}

    inline int itNumber() const                     {return _it_num;}
    inline QString spaceGroupHMBgmn() const         {return _spgr_name_bgmn;}
    inline QString spaceGroupHMCif()  const         {return _spgr_name_cif;}
    inline QString crystalSystemCif() const         {return _spgr_cryst_system_cif;}

    QList<QStringList> symmetryOperations();
    QList<CrystalSymOp> symmetryOperationMatrices();

    double dSpacing(int, int, int);

    void reset();
    bool fixCell();
    bool isValid() const;

private:
    double _axes[3];
    double _angles[3];
    double _axes_esd[3];
    double _angles_esd[3];
    double _vol;
    QString _spgr_name_bgmn;
    QString _spgr_name_cif;
    QString _spgr_cryst_system_cif;
    int _it_num;
    int _setting_num;
    QList<CrystalSymOp> _symOpMatrices;
    unitLength _axis_unit;

    QString spgrBgmnToCif(const QString &);
    QString spgrCrystalSystemCif(int);
    QMatrix4x4 f2cMatrix();
    double calcVolume();

    bool fixTriclinicCell();
    bool fixMonoclinicCell();
    bool fixOrthorhombicCell();
    bool fixTetragonalCell();
    bool fixTrigonalCell();
    bool fixHexagonalCell();
    bool fixCubicCell();
    void dumpFixError(const QString &);
    bool parseSymmetryOperations();
    QList<CrystalSymOp> checkSymmetryOperationTranslations(const QList<CrystalSymOp> &, const QString &);
};

#endif // CRYSTALUNITCEL_H

/***************************************************************************
                          hklphasedata.h  -  description
                             -------------------
    begin                : Mon Feb 04, 2019
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

#ifndef HKLPHASEDATA_H
#define HKLPHASEDATA_H

#include "hkl.h"

#include <QString>
#include <QByteArray>
#include <QMap>
#include <QVariant>
#include <QVector>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif


class XRDIO_EXPORT HklPhaseData
{
public:
    HklPhaseData();
    HklPhaseData(const QString &f, const QString &md5, const QString &sdir, const QString &com,
                 const QString &ph, const QString &col, const QString &form, int fav,
                 double strA, double strB, double strC, double lngA, double lngB, double lngC, double dns,
                 const QByteArray &hkl, const QMap<QString, QVariant> &aux, const QByteArray &xy,
                 const QStringList &el, bool di);

    HklPhaseData(const HklPhaseData &);
    HklPhaseData operator=(const HklPhaseData &);

    inline QString file() const                    {return _file;}
    inline QString md5hash() const                 {return _md5hash;}
    inline QString sourceDir() const               {return _sourceDir;}
    inline QString comment() const                 {return _comment;}
    inline QString phase() const                   {return _phase;}
    inline QString color() const                   {return _color;}
    inline QString formula() const                 {return _formula;}
    inline bool doIndex() const                    {return _doIndex;}
    inline int favorite() const                    {return _favorite;}
    inline double strongest() const                {return _strongest;}
    inline double strongest2() const               {return _strongest2;}
    inline double strongest3() const               {return _strongest3;}
    inline double longest() const                  {return _longest;}
    inline double longest2() const                 {return _longest2;}
    inline double longest3() const                 {return _longest3;}
    inline double density() const                  {return _density;}

    inline QByteArray hklBinary() const            {return _hklData;}
    QVector<Hkl> hklData() const;

    inline QByteArray xyBinary() const             {return _xyData;}
    QVector<QVector<double> > xyData() const;

    inline QStringList elements() const            {return _elements;}
    inline QStringList & elements()                {return _elements;}

    inline QMap<QString, QVariant> auxData() const {return _auxData;}
    inline QMap<QString, QVariant> & auxData()     {return _auxData;}

    inline void setFile(const QString &s)          {_file = s;}
    inline void setMd5Hash(const QString &s)       {_md5hash = s;}
    inline void setSourceDir(const QString &s)     {_sourceDir = s;}
    inline void setComment(const QString &s)       {_comment = s;}
    inline void setPhase(const QString &s)         {_phase = s;}
    inline void setColor(const QString &s)         {_color = s;}
    inline void setDoIndex(bool b)                 {_doIndex = b;}
    inline void setFormula(const QString &s)       {_formula = s;}
    inline void setFavorite(int i)                 {_favorite = i;}
    inline void setStrongest(double d)             {_strongest = d;}
    inline void setStrongest2(double d)            {_strongest2 = d;}
    inline void setStrongest3(double d)            {_strongest3 = d;}
    inline void setLongest(double d)               {_longest  = d;}
    inline void setLongest2(double d)              {_longest2 = d;}
    inline void setLongest3(double d)              {_longest3 = d;}
    inline void setDensity(double d)               {_density = d;}
    inline void setAuxData(const QMap<QString, QVariant> &m) {_auxData = m;}

    inline void setHklData(const QByteArray &b)    {_hklData = b;}
    void setHklData(const QVector<Hkl> &);

    inline void setXyData(const QByteArray &b)     {_xyData = b;}
    void setXyData(const QVector<QVector<double> > &);

    inline void setElements(const QStringList &l)  {_elements = l;}

private:
    QString _file;
    QString _md5hash;
    QString _sourceDir;
    QString _comment;
    QString _phase;
    QString _color;
    QString _formula;
    int _favorite;
    double _strongest;
    double _strongest2;
    double _strongest3;
    double _longest;
    double _longest2;
    double _longest3;
    double _density;
    QByteArray _hklData;
    QMap<QString, QVariant> _auxData;
    QByteArray _xyData;
    QStringList _elements;
    bool _doIndex;

    void getDMetrics(const QVector<Hkl> &, double &, double &, double &, double &, double &, double &);
};


#endif // HKLPHASEDATA_H

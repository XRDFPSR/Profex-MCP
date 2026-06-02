/***************************************************************************
                          scan.h  -  description
                             -------------------
    begin                : Mon Jun 24 14:16:07 CEST 2005
    copyright            : (C) 2005 by Nicola Doebelin
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

#ifndef SCAN_H
#define SCAN_H

#include <QColor>
#include <QString>
#include <QHash>
#include <QVariant>
#include <QVector>
#include <QPair>
#include <QUuid>
#include <QFlags>

#include "hkl.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT Scan
{
  public:
    enum ScanType {
        XY         = 1 << 0,
        HKL        = 1 << 1,
        MEASURED   = 1 << 2,
        REFINED    = 1 << 3,
        SYNTHETIC  = 1 << 4,
        PHASE      = 1 << 5,
        BACKGROUND = 1 << 6,
        DIFF       = 1 << 7,
        TEMPORARY  = 1 << 8,
        ISABOVEBACKGROUND = 1 << 9
    };

    enum WavelengthMode {
        UNKNOWN,
        CHARACTERISTIC,
        SYNCHROTRON,
        NEUTRON
    };

    Q_DECLARE_FLAGS(ScanTypes, ScanType)

    Scan(const QString &name = QString(),
         const QColor &col = QColor(),
         int linewdth = 1);

    Scan(const Scan &);

    ~Scan();

    Scan operator=(const Scan &);

    /**
     *  set-functions 
     **/

    /* set type flags */
    inline void setTypes(const ScanTypes &s) { _flags = s; }

    inline void setUid(const QUuid &u) { _uid = u; }

    /* name of the scan, for display in the legend */
    inline void setName( const QString &s )       { _name = s; }
    inline void setOverrideName(const QString &s) { _overrideName = s; }

    inline void setSourceFileName( const QString &s ) { _sourceFile = s; }

    /* arbitrary comment, e.g. description */
    inline void setComment( const QString &s )    { _comment = s; }

    inline void setXAxisLabel( const QString &s ) { _xAxisLabel = s;}
    inline void setYAxisLabel( const QString &s ) { _yAxisLabel = s;}

    /* vector holding data (intensities) */
    inline void setDataInt( const QVector<double> &v ) { _dataInt = v; }
    inline void setDataAng( const QVector<double> &v ) { _dataAng = v; }

    /* color of the plot, specified as QColor */
    inline void setColor( const QColor &c )       { _color = c; }

    /* color of the plot, specified as QString */
    inline void setColor( const QString &s )      { _color = QColor(s); }

    /* line width of the plot, >= 1 */
    inline void setLineWidth(int i)      { _linewidth = uint(i); }

    /* set geometric transformations */
    inline void setXoffset(double f)      { _offset_x = f; }
    inline void setYoffset(double f)      { _offset_y = f; }
    inline void setScaleFactor(double f)  { _scale_factor = f; }
    void applyOffsetsPermanently();

    /* auxillary scan information, can be any kind of data, e.g. type of divergence slit */
    inline void setAuxInfo( const QString &s, const QVariant &v) { _auxInfo.insert(s, v); }
    inline void addAuxInfo( const QVariantHash &h ) { _auxInfo.insert(h); }

    /* set visibility */
    inline void setVisible(bool b)       { _is_visible = b; }

    /* set line width */
    inline void setLinewidth(uint i)     { _linewidth = i; }
    inline void setPointSymbol(uint i)   { _pointmode = i; }
    inline void setHklSymbol(uint i)     { _hklmode = i; }

    /* set the wavelength of the X-radiation */
    inline void setWaveLength(double f)   { _wavelength = f; }
    inline void setWaveLength2(double f)   { _wavelength2 = f; }
    inline void setWaveLength3(double f)   { _wavelength3 = f; }
    void setNamedWaveLength(const QString &);
    inline void setWavelengthMode(WavelengthMode w) {_wlMode = w;}

    /* set the stepsize */
    inline void setStepSize(double f)     {_stepsize = f; }

    /* sets the hkl vector */
    inline void setHklData(const QVector<Hkl> &v) { _dataHkl = v; }

    /* sets the counting time per step in seconds */
    inline void setTimePerStep(double f) {_tPerStep = f; }

    /* sets the x axis unit of hkl data ("tt" for two-theta, "da" for d in Angstrom, "dnm" for d in nm) */
    inline void setHklXunit(const QString &s) { _hklXunit = s; }

    /* applies a synthetic noise pattern to the scan */
    void addNoise();

    /* sets a flag if the scan is marked as "active" (e.g. for drawing in bold lines */
    void setActive(bool);

    /* sets the scans position in a list of scans. Used to sort scans for display */
    inline void setPosition(int i)         {_displayPosition = uint(i);}

    /**
     *  get-functions 
     **/

    Scan clone() const;

    inline ScanTypes scanTypes() const            { return _flags; }
    inline QUuid   uid() const                    { return _uid; }
    inline QString sourceFileName() const         { return _sourceFile; }
    inline QString comment() const                { return _comment; }
    inline QString xAxisLabel() const             { return _xAxisLabel; }
    inline QString yAxisLabel() const             { return _yAxisLabel; }

    QString name(bool withFileName = false) const;
    double startAngle() const;
    double endAngle() const;
    QVector<double> dataDspacing(bool &) const;
    Scan mid(double, double) const;
    bool isEmpty() const;

    inline QVector<double> & pDataIntensity()      { return _dataInt; }
    inline QVector<double> & pDataAngle()          { return _dataAng; }
    inline QVector<Hkl>    & pDataHkl()            { return _dataHkl; }

    inline const QVector<double> & pDataIntensity() const { return _dataInt; }
    inline const QVector<double> & pDataAngle()     const { return _dataAng; }
    inline const QVector<Hkl>    & pDataHkl()       const { return _dataHkl; }

    bool hasScanData() const;
    bool hasHklData() const;

    inline QColor color() const                   { return _color; }
    inline QString colorName() const              { return _color.name(); }
    inline int lineWidth() const                  { return int(_linewidth); }
    inline int size() const                       { return qMin(_dataInt.size(), _dataAng.size()); }
    double at(int i) const;
    double intensity(int i) const;
    double intensity(double) const;
    double angle(int) const;
    inline double xOffset() const                 { return _offset_x; }
    inline double yOffset() const                 { return _offset_y; }
    inline bool isVisible() const                 { return _is_visible; }
    inline uint linewidth() const                 { return _linewidth; }
    inline double waveLength() const              { return _wavelength; }
    inline double waveLength2() const             { return _wavelength2; }
    inline double waveLength3() const             { return _wavelength3; }
    inline uint pointSymbol() const               { return _pointmode; }
    inline uint hklSymbol() const                 { return _hklmode; }
    QMap<double, double> dataMap(bool withOperations = false) const;

    inline double scaleFactor()                   { return _scale_factor; }
    inline double scaleFactor() const             { return _scale_factor; }

    int indexOfAngle(double, int) const;
    double angleOfMaxIntensity() const;
    double angleOfMaxIntensity(double, double) const;
    int indexOfMaxIntensity() const;
    int indexOfMaxIntensity(double, double) const;
    double angleOfMinIntensity() const;
    double angleOfMinIntensity(double, double) const;
    int indexOfMinIntensity() const;
    int indexOfMinIntensity(double, double) const;

    int indexOfHkl(const QUuid &) const;
    inline int hklCount() const                   {return _dataHkl.size();}
    QPointF first() const;
    QPointF last() const;
    QPointF point(int, bool &) const;

    Hkl * getHkl(int);
    Hkl * getHkl(const QUuid &);
    const Hkl * getHkl(int) const;
    const Hkl * getHkl(const QUuid &) const;
    double getHklMaxIntensity() const;
    void setAllHklDisplayStatus(int prev, int cur);

    double stepSize() const;
    double minIntensity(double, double) const;
    double maxIntensity(double, double) const;
    double minIntensity() const;
    double maxIntensity() const;
    inline double minAngle() const                { return startAngle(); }
    inline double maxAngle() const                { return endAngle(); }
    double timePerStep() const                    { return _tPerStep; }
    double hklMaxIntensity() const;

    QVariant auxInfo(const QString &, bool &) const;
    inline const QVariantHash & allAuxInfo() const {return _auxInfo;}

    QPair<int, double> getRexpDenom(double wmin, double wmax, bool tubeTails);
    QVector<double> getScanWeighing(bool tubeTails);
    inline QString getHklXunit() const            {return _hklXunit;}

    bool isTemporary() const;
    inline bool isActive() const                  {return _is_active;}

    int getPosition() const                       {return int(_displayPosition);}

    inline WavelengthMode wavelengthMode() const {return _wlMode;}

    /**
     *  data operation functions
     **/

    void insertHkl(const Hkl &);
    bool removeHkl(const QUuid &);
    bool removeHkl(int);

    void reset();

private:
    QUuid         _uid;
    QString       _name;
    QString       _overrideName;
    QString       _sourceFile;
    QString       _comment;
    QString       _xAxisLabel;
    QString       _yAxisLabel;
    QString       _hklXunit;

    ScanTypes     _flags;

    QVector<double> _dataInt;
    QVector<double> _dataAng;

    QVector<Hkl>  _dataHkl;

    QColor        _color;

    double        _offset_x;
    double        _offset_y;
    double        _scale_factor;
    double        _wavelength;
    double        _wavelength2;
    double        _wavelength3;
    double        _stepsize;
    double        _tPerStep;
    double        _minIntens;
    double        _maxIntens;

    bool          _is_visible;
    bool          _is_active;

    uint          _linewidth;
    uint          _pointmode;
    uint          _hklmode;

    uint          _displayPosition;

    QVariantHash  _auxInfo;

    WavelengthMode _wlMode;

    QVector<double> korrw(const QVector<double> &w, bool tubeTails);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Scan::ScanTypes)

#endif

/** EOF **/

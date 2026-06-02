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

/**
 * \brief Represents a single X-ray diffraction scan (pattern) with associated metadata.
 *
 * The Scan class encapsulates all data and properties of an XRD measurement or
 * computation, including intensity-angle data, HKL reflection data, display
 * attributes (color, line width, visibility), geometric transformations
 * (offsets, scaling), wavelength settings, and auxiliary information.
 * Scan flags (ScanTypes) describe the origin and role of the scan, such as
 * whether it was measured, refined, synthetic, or a difference pattern.
 */
class XRDIO_EXPORT Scan
{
  public:
    /**
     * \brief Flags describing the origin and purpose of the scan.
     *
     * These are bitmask values that can be combined via ScanTypes.
     */
    enum ScanType {
        XY              = 1 << 0, /**< Standard xy (angle vs. intensity) data */
        HKL             = 1 << 1, /**< Contains HKL reflection data */
        MEASURED        = 1 << 2, /**< Experimentally measured scan */
        REFINED         = 1 << 3, /**< Result of a refinement calculation */
        SYNTHETIC       = 1 << 4, /**< Synthetically generated pattern */
        PHASE           = 1 << 5, /**< Represents a single phase contribution */
        BACKGROUND      = 1 << 6, /**< Background curve */
        DIFF            = 1 << 7, /**< Difference pattern (observed - calculated) */
        TEMPORARY       = 1 << 8, /**< Temporary scan, not intended for permanent storage */
        ISABOVEBACKGROUND = 1 << 9 /**< Marks points above the background */
    };

    /**
     * \brief Radiation wavelength mode.
     *
     * Describes the type of radiation source used for the measurement.
     */
    enum WavelengthMode {
        UNKNOWN,         /**< Wavelength mode not specified */
        CHARACTERISTIC,  /**< Characteristic X-ray tube (e.g. Cu K-alpha) */
        SYNCHROTRON,     /**< Synchrotron radiation */
        NEUTRON          /**< Neutron radiation */
    };

    Q_DECLARE_FLAGS(ScanTypes, ScanType)

    /**
     * \brief Constructs a new scan with a display name, colour and line width.
     * \param name Display name shown in legends and lists.
     * \param col Colour used for plotting the scan.
     * \param linewdth Line width in pixels (>= 1).
     */
    Scan(const QString &name = QString(),
         const QColor &col = QColor(),
         int linewdth = 1);

    /**
     * \brief Copy constructor.
     * \param other Scan to copy data from.
     */
    Scan(const Scan &);

    /**
     * \brief Destructor.
     */
    ~Scan();

    /**
     * \brief Assignment operator.
     * \param other Scan to assign from.
     * \return Reference to this scan.
     */
    Scan operator=(const Scan &);

    /**
     *  set-functions 
     **/

    /* set type flags */
    /**
     * \brief Sets the type flags for this scan.
     * \param s Combined ScanTypes flags to assign.
     */
    inline void setTypes(const ScanTypes &s) { _flags = s; }

    /**
     * \brief Sets the universally unique identifier for this scan.
     * \param u New UUID.
     */
    inline void setUid(const QUuid &u) { _uid = u; }

    /* name of the scan, for display in the legend */
    /**
     * \brief Sets the display name shown in the legend.
     * \param s New display name.
     */
    inline void setName( const QString &s )       { _name = s; }
    /**
     * \brief Sets an override name that takes precedence over the default name.
     * \param s Override name string.
     */
    inline void setOverrideName(const QString &s) { _overrideName = s; }

    /**
     * \brief Sets the source file name from which the scan was loaded.
     * \param s Source file path.
     */
    inline void setSourceFileName( const QString &s ) { _sourceFile = s; }

    /* arbitrary comment, e.g. description */
    /**
     * \brief Sets an arbitrary comment or description for the scan.
     * \param s Comment string.
     */
    inline void setComment( const QString &s )    { _comment = s; }

    /**
     * \brief Sets the label for the x-axis (e.g. "2θ [°]").
     * \param s X-axis label.
     */
    inline void setXAxisLabel( const QString &s ) { _xAxisLabel = s;}
    /**
     * \brief Sets the label for the y-axis (e.g. "Intensity [counts]").
     * \param s Y-axis label.
     */
    inline void setYAxisLabel( const QString &s ) { _yAxisLabel = s;}

    /* vector holding data (intensities) */
    /**
     * \brief Sets the vector of intensity values.
     * \param v Vector of double intensity values.
     */
    inline void setDataInt( const QVector<double> &v ) { _dataInt = v; }
    /**
     * \brief Sets the vector of angle values (in degrees 2θ).
     * \param v Vector of double angle values.
     */
    inline void setDataAng( const QVector<double> &v ) { _dataAng = v; }

    /* color of the plot, specified as QColor */
    /**
     * \brief Sets the plot colour from a QColor.
     * \param c New colour.
     */
    inline void setColor( const QColor &c )       { _color = c; }

    /* color of the plot, specified as QString */
    /**
     * \brief Sets the plot colour from a string specification.
     * \param s Colour name or hex string (e.g. "red" or "#FF0000").
     */
    inline void setColor( const QString &s )      { _color = QColor(s); }

    /* line width of the plot, >= 1 */
    /**
     * \brief Sets the line width in pixels.
     * \param i Line width in pixels (must be >= 1).
     */
    inline void setLineWidth(int i)      { _linewidth = uint(i); }

    /* set geometric transformations */
    /**
     * \brief Sets the x-axis offset (shift in 2θ).
     * \param f Offset value in degrees.
     */
    inline void setXoffset(double f)      { _offset_x = f; }
    /**
     * \brief Sets the y-axis offset (intensity shift).
     * \param f Offset value in intensity units.
     */
    inline void setYoffset(double f)      { _offset_y = f; }
    /**
     * \brief Sets the intensity scale factor.
     * \param f Multiplicative scale factor.
     */
    inline void setScaleFactor(double f)  { _scale_factor = f; }
    /**
     * \brief Permanently applies offsets by modifying the raw angle and
     *        intensity data, then resetting the offset values to zero.
     */
    void applyOffsetsPermanently();

    /* auxillary scan information, can be any kind of data, e.g. type of divergence slit */
    /**
     * \brief Stores an auxiliary information key-value pair (e.g. slit type).
     * \param s Key string.
     * \param v Value as QVariant.
     */
    inline void setAuxInfo( const QString &s, const QVariant &v) { _auxInfo.insert(s, v); }
    /**
     * \brief Merges a hash of auxiliary information into the scan.
     * \param h Hash of key-value pairs to add.
     */
    inline void addAuxInfo( const QVariantHash &h ) { _auxInfo.insert(h); }

    /* set visibility */
    /**
     * \brief Sets whether the scan is visible in plots.
     * \param b True to show, false to hide.
     */
    inline void setVisible(bool b)       { _is_visible = b; }

    /* set line width */
    /**
     * \brief Sets the line width in pixels (uint overload).
     * \param i Line width.
     */
    inline void setLinewidth(uint i)     { _linewidth = i; }
    /**
     * \brief Sets the point symbol mode for data-point display.
     * \param i Symbol type index.
     */
    inline void setPointSymbol(uint i)   { _pointmode = i; }
    /**
     * \brief Sets the symbol mode for HKL marker display.
     * \param i HKL symbol type index.
     */
    inline void setHklSymbol(uint i)     { _hklmode = i; }

    /* set the wavelength of the X-radiation */
    /**
     * \brief Sets the primary wavelength (usually Ka1).
     * \param f Wavelength in Angstrom.
     */
    inline void setWaveLength(double f)   { _wavelength = f; }
    /**
     * \brief Sets the secondary wavelength (usually Ka2).
     * \param f Wavelength in Angstrom.
     */
    inline void setWaveLength2(double f)   { _wavelength2 = f; }
    /**
     * \brief Sets the tertiary wavelength.
     * \param f Wavelength in Angstrom.
     */
    inline void setWaveLength3(double f)   { _wavelength3 = f; }
    /**
     * \brief Sets the wavelength(s) from a named radiation source (e.g. "Cu Ka").
     * \param name Name of the radiation source.
     */
    void setNamedWaveLength(const QString &);
    /**
     * \brief Sets the wavelength mode (type of radiation source).
     * \param w Wavelength mode enum value.
     */
    inline void setWavelengthMode(WavelengthMode w) {_wlMode = w;}

    /* set the stepsize */
    /**
     * \brief Sets the step size (angular increment between data points).
     * \param f Step size in degrees 2θ.
     */
    inline void setStepSize(double f)     {_stepsize = f; }

    /* sets the hkl vector */
    /**
     * \brief Sets the vector of HKL reflection data.
     * \param v Vector of Hkl objects.
     */
    inline void setHklData(const QVector<Hkl> &v) { _dataHkl = v; }

    /* sets the counting time per step in seconds */
    /**
     * \brief Sets the counting time per step in seconds.
     * \param f Time per step in seconds.
     */
    inline void setTimePerStep(double f) {_tPerStep = f; }

    /* sets the x axis unit of hkl data ("tt" for two-theta, "da" for d in Angstrom, "dnm" for d in nm) */
    /**
     * \brief Sets the x-axis unit for HKL data display.
     * \param s Unit string: "tt" (two-theta), "da" (d in Angstrom), or "dnm" (d in nm).
     */
    inline void setHklXunit(const QString &s) { _hklXunit = s; }

    /* applies a synthetic noise pattern to the scan */
    /**
     * \brief Applies synthetic noise to the intensity data based on Poisson statistics.
     *
     * The noise is calculated from the square root of each intensity value,
     * modelling typical counting-statistics fluctuations.
     */
    void addNoise();

    /* sets a flag if the scan is marked as "active" (e.g. for drawing in bold lines */
    /**
     * \brief Marks the scan as active (e.g. for highlighting with bold lines).
     * \param b True to activate, false to deactivate.
     */
    void setActive(bool);

    /* sets the scans position in a list of scans. Used to sort scans for display */
    /**
     * \brief Sets the display position index for ordering in lists/legends.
     * \param i Position index.
     */
    inline void setPosition(int i)         {_displayPosition = uint(i);}

    /**
     *  get-functions 
     **/

    /**
     * \brief Creates a deep copy of this scan.
     * \return A new Scan object with identical data.
     */
    Scan clone() const;

    /**
     * \brief Returns the type flags for this scan.
     * \return Combined ScanTypes flags.
     */
    inline ScanTypes scanTypes() const            { return _flags; }
    /**
     * \brief Returns the universally unique identifier.
     * \return UUID of this scan.
     */
    inline QUuid   uid() const                    { return _uid; }
    /**
     * \brief Returns the source file name.
     * \return File path string.
     */
    inline QString sourceFileName() const         { return _sourceFile; }
    /**
     * \brief Returns the comment/description string.
     * \return Comment string.
     */
    inline QString comment() const                { return _comment; }
    /**
     * \brief Returns the x-axis label.
     * \return Label string (e.g. "2θ [°]").
     */
    inline QString xAxisLabel() const             { return _xAxisLabel; }
    /**
     * \brief Returns the y-axis label.
     * \return Label string (e.g. "Intensity [counts]").
     */
    inline QString yAxisLabel() const             { return _yAxisLabel; }

    /**
     * \brief Returns the display name of the scan.
     * \param withFileName If true, appends the source file name to the result.
     * \return The display name string.
     */
    QString name(bool withFileName = false) const;
    /**
     * \brief Returns the smallest angle value (start of scan range).
     * \return Start angle in degrees 2θ.
     */
    double startAngle() const;
    /**
     * \brief Returns the largest angle value (end of scan range).
     * \return End angle in degrees 2θ.
     */
    double endAngle() const;
    /**
     * \brief Converts angle data to d-spacing values using Bragg's law.
     * \param ok Reference parameter set to true on success, false on failure.
     * \return Vector of d-spacing values in Angstrom.
     */
    QVector<double> dataDspacing(bool &) const;
    /**
     * \brief Extracts a sub-range of the scan between two angle values.
     * \param a Start angle.
     * \param b End angle.
     * \return New Scan containing only the data within [a, b].
     */
    Scan mid(double, double) const;
    /**
     * \brief Checks whether the scan contains any valid data points.
     * \return True if the scan has no intensity/angle data.
     */
    bool isEmpty() const;

    /**
     * \brief Returns a mutable reference to the raw intensity vector.
     * \return Reference to the internal QVector<double> of intensities.
     */
    inline QVector<double> & pDataIntensity()      { return _dataInt; }
    /**
     * \brief Returns a mutable reference to the raw angle vector.
     * \return Reference to the internal QVector<double> of angles.
     */
    inline QVector<double> & pDataAngle()          { return _dataAng; }
    /**
     * \brief Returns a mutable reference to the raw HKL data vector.
     * \return Reference to the internal QVector<Hkl>.
     */
    inline QVector<Hkl>    & pDataHkl()            { return _dataHkl; }

    /**
     * \brief Returns a const reference to the raw intensity vector.
     * \return Const reference to the internal QVector<double> of intensities.
     */
    inline const QVector<double> & pDataIntensity() const { return _dataInt; }
    /**
     * \brief Returns a const reference to the raw angle vector.
     * \return Const reference to the internal QVector<double> of angles.
     */
    inline const QVector<double> & pDataAngle()     const { return _dataAng; }
    /**
     * \brief Returns a const reference to the raw HKL data vector.
     * \return Const reference to the internal QVector<Hkl>.
     */
    inline const QVector<Hkl>    & pDataHkl()       const { return _dataHkl; }

    /**
     * \brief Checks whether the scan contains XY (angle-intensity) data.
     * \return True if both intensity and angle vectors are non-empty.
     */
    bool hasScanData() const;
    /**
     * \brief Checks whether the scan contains HKL reflection data.
     * \return True if the HKL data vector is non-empty.
     */
    bool hasHklData() const;

    /**
     * \brief Returns the plot colour.
     * \return QColor of the scan.
     */
    inline QColor color() const                   { return _color; }
    /**
     * \brief Returns the plot colour as a hex name string.
     * \return Colour string (e.g. "#FF0000").
     */
    inline QString colorName() const              { return _color.name(); }
    /**
     * \brief Returns the line width in pixels.
     * \return Line width.
     */
    inline int lineWidth() const                  { return int(_linewidth); }
    /**
     * \brief Returns the number of valid data points.
     * \return Minimum of intensity and angle vector sizes.
     */
    inline int size() const                       { return qMin(_dataInt.size(), _dataAng.size()); }
    /**
     * \brief Returns the intensity at a given index after applying offsets.
     * \param i Data point index.
     * \return Offset-corrected intensity value.
     */
    double at(int i) const;
    /**
     * \brief Returns the raw intensity at a given index.
     * \param i Data point index.
     * \return Raw intensity value.
     */
    double intensity(int i) const;
    /**
     * \brief Returns the intensity at a given angle, interpolating if necessary.
     * \param a Angle in degrees 2θ.
     * \return Interpolated intensity at that angle.
     */
    double intensity(double) const;
    /**
     * \brief Returns the raw angle at a given index.
     * \param i Data point index.
     * \return Angle in degrees 2θ.
     */
    double angle(int) const;
    /**
     * \brief Returns the current x-axis offset.
     * \return Offset in degrees 2θ.
     */
    inline double xOffset() const                 { return _offset_x; }
    /**
     * \brief Returns the current y-axis offset.
     * \return Offset in intensity units.
     */
    inline double yOffset() const                 { return _offset_y; }
    /**
     * \brief Returns whether the scan is set to visible.
     * \return True if visible.
     */
    inline bool isVisible() const                 { return _is_visible; }
    /**
     * \brief Returns the line width in pixels.
     * \return Line width as uint.
     */
    inline uint linewidth() const                 { return _linewidth; }
    /**
     * \brief Returns the primary wavelength.
     * \return Wavelength in Angstrom.
     */
    inline double waveLength() const              { return _wavelength; }
    /**
     * \brief Returns the secondary wavelength.
     * \return Wavelength in Angstrom.
     */
    inline double waveLength2() const             { return _wavelength2; }
    /**
     * \brief Returns the tertiary wavelength.
     * \return Wavelength in Angstrom.
     */
    inline double waveLength3() const             { return _wavelength3; }
    /**
     * \brief Returns the point symbol mode index.
     * \return Symbol type index.
     */
    inline uint pointSymbol() const               { return _pointmode; }
    /**
     * \brief Returns the HKL symbol mode index.
     * \return HKL symbol type index.
     */
    inline uint hklSymbol() const                 { return _hklmode; }
    /**
     * \brief Returns a map of angle -> intensity data, optionally with offsets applied.
     * \param withOperations If true, applies offsets and scaling to the data.
     * \return QMap of angle to intensity.
     */
    QMap<double, double> dataMap(bool withOperations = false) const;

    /**
     * \brief Returns the current scale factor.
     * \return Scale factor value.
     */
    inline double scaleFactor()                   { return _scale_factor; }
    /**
     * \brief Returns the current scale factor (const overload).
     * \return Scale factor value.
     */
    inline double scaleFactor() const             { return _scale_factor; }

    /**
     * \brief Finds the index of the nearest data point to a given angle, starting from a hint.
     * \param a Target angle in degrees 2θ.
     * \param hint Starting index for the search.
     * \return Index of the nearest data point, or -1 if no data.
     */
    int indexOfAngle(double, int) const;
    /**
     * \brief Returns the angle of the global maximum intensity.
     * \return Angle of the highest intensity data point.
     */
    double angleOfMaxIntensity() const;
    /**
     * \brief Returns the angle of the maximum intensity within a given angular range.
     * \param a Start of angular range.
     * \param b End of angular range.
     * \return Angle of the highest intensity in the range.
     */
    double angleOfMaxIntensity(double, double) const;
    /**
     * \brief Returns the index of the global maximum intensity.
     * \return Index of the highest intensity data point.
     */
    int indexOfMaxIntensity() const;
    /**
     * \brief Returns the index of the maximum intensity within a given angular range.
     * \param a Start of angular range.
     * \param b End of angular range.
     * \return Index of the highest intensity in the range.
     */
    int indexOfMaxIntensity(double, double) const;
    /**
     * \brief Returns the angle of the global minimum intensity.
     * \return Angle of the lowest intensity data point.
     */
    double angleOfMinIntensity() const;
    /**
     * \brief Returns the angle of the minimum intensity within a given angular range.
     * \param a Start of angular range.
     * \param b End of angular range.
     * \return Angle of the lowest intensity in the range.
     */
    double angleOfMinIntensity(double, double) const;
    /**
     * \brief Returns the index of the global minimum intensity.
     * \return Index of the lowest intensity data point.
     */
    int indexOfMinIntensity() const;
    /**
     * \brief Returns the index of the minimum intensity within a given angular range.
     * \param a Start of angular range.
     * \param b End of angular range.
     * \return Index of the lowest intensity in the range.
     */
    int indexOfMinIntensity(double, double) const;

    /**
     * \brief Finds the index of an HKL reflection by its UUID.
     * \param uuid UUID of the HKL reflection to find.
     * \return Index in the HKL vector, or -1 if not found.
     */
    int indexOfHkl(const QUuid &) const;
    /**
     * \brief Returns the number of HKL reflections.
     * \return Count of HKL objects.
     */
    inline int hklCount() const                   {return _dataHkl.size();}
    /**
     * \brief Returns the first data point (angle, intensity).
     * \return QPointF with (angle, intensity) of the first point.
     */
    QPointF first() const;
    /**
     * \brief Returns the last data point (angle, intensity).
     * \return QPointF with (angle, intensity) of the last point.
     */
    QPointF last() const;
    /**
     * \brief Returns the data point at a given index.
     * \param i Index of the data point.
     * \param ok Reference parameter set to true on success, false if index is out of range.
     * \return QPointF with (angle, intensity) at index i.
     */
    QPointF point(int, bool &) const;

    /**
     * \brief Returns a mutable pointer to the Hkl object at the given index.
     * \param i Index in the HKL vector.
     * \return Pointer to Hkl, or nullptr if index is out of range.
     */
    Hkl * getHkl(int);
    /**
     * \brief Returns a mutable pointer to the Hkl object with the given UUID.
     * \param uuid UUID of the HKL reflection.
     * \return Pointer to Hkl, or nullptr if not found.
     */
    Hkl * getHkl(const QUuid &);
    /**
     * \brief Returns a const pointer to the Hkl object at the given index.
     * \param i Index in the HKL vector.
     * \return Const pointer to Hkl, or nullptr if index is out of range.
     */
    const Hkl * getHkl(int) const;
    /**
     * \brief Returns a const pointer to the Hkl object with the given UUID.
     * \param uuid UUID of the HKL reflection.
     * \return Const pointer to Hkl, or nullptr if not found.
     */
    const Hkl * getHkl(const QUuid &) const;
    /**
     * \brief Returns the maximum intensity among all HKL reflections.
     * \return Maximum HKL intensity value.
     */
    double getHklMaxIntensity() const;
    /**
     * \brief Updates the display status of all HKL reflections.
     * \param prev Previous display status value.
     * \param cur New display status value to set.
     */
    void setAllHklDisplayStatus(int prev, int cur);

    /**
     * \brief Returns the step size of the scan.
     * \return Step size in degrees 2θ.
     */
    double stepSize() const;
    /**
     * \brief Returns the minimum intensity within a given angular range.
     * \param a Start of angular range.
     * \param b End of angular range.
     * \return Minimum intensity in the range.
     */
    double minIntensity(double, double) const;
    /**
     * \brief Returns the maximum intensity within a given angular range.
     * \param a Start of angular range.
     * \param b End of angular range.
     * \return Maximum intensity in the range.
     */
    double maxIntensity(double, double) const;
    /**
     * \brief Returns the global minimum intensity across all data points.
     * \return Minimum intensity value.
     */
    double minIntensity() const;
    /**
     * \brief Returns the global maximum intensity across all data points.
     * \return Maximum intensity value.
     */
    double maxIntensity() const;
    /**
     * \brief Returns the minimum angle (alias for startAngle()).
     * \return Start angle in degrees 2θ.
     */
    inline double minAngle() const                { return startAngle(); }
    /**
     * \brief Returns the maximum angle (alias for endAngle()).
     * \return End angle in degrees 2θ.
     */
    inline double maxAngle() const                { return endAngle(); }
    /**
     * \brief Returns the counting time per step.
     * \return Time per step in seconds.
     */
    double timePerStep() const                    { return _tPerStep; }
    /**
     * \brief Returns the maximum intensity among HKL reflections.
     * \return Maximum HKL intensity.
     */
    double hklMaxIntensity() const;

    /**
     * \brief Retrieves an auxiliary information value by key.
     * \param s Key string.
     * \param ok Reference parameter set to true if the key exists, false otherwise.
     * \return QVariant value, or invalid QVariant if key not found.
     */
    QVariant auxInfo(const QString &, bool &) const;
    /**
     * \brief Returns a const reference to all auxiliary information.
     * \return Const reference to the QVariantHash of auxiliary data.
     */
    inline const QVariantHash & allAuxInfo() const {return _auxInfo;}

    /**
     * \brief Computes the denominator for R-exp (R-factor) calculation.
     * \param wmin Minimum angle for the calculation range.
     * \param wmax Maximum angle for the calculation range.
     * \param tubeTails If true, apply tube-tail corrections.
     * \return Pair of (number of data points, weighted sum denominator).
     */
    QPair<int, double> getRexpDenom(double wmin, double wmax, bool tubeTails);
    /**
     * \brief Computes the weighing vector for weighted-profile R-factor calculations.
     * \param tubeTails If true, apply tube-tail corrections.
     * \return Vector of weights for each data point.
     */
    QVector<double> getScanWeighing(bool tubeTails);
    /**
     * \brief Returns the x-axis unit for HKL data display.
     * \return Unit string: "tt", "da", or "dnm".
     */
    inline QString getHklXunit() const            {return _hklXunit;}

    /**
     * \brief Checks whether the scan has the TEMPORARY flag set.
     * \return True if the scan is marked as temporary.
     */
    bool isTemporary() const;
    /**
     * \brief Checks whether the scan is marked as active.
     * \return True if active.
     */
    inline bool isActive() const                  {return _is_active;}

    /**
     * \brief Returns the display position index for sorting.
     * \return Position index.
     */
    int getPosition() const                       {return int(_displayPosition);}

    /**
     * \brief Returns the wavelength mode (radiation type).
     * \return WavelengthMode enum value.
     */
    inline WavelengthMode wavelengthMode() const {return _wlMode;}

    /**
     *  data operation functions
     **/

    /**
     * \brief Inserts a new HKL reflection into the scan.
     * \param h Hkl object to insert.
     */
    void insertHkl(const Hkl &);
    /**
     * \brief Removes an HKL reflection by its UUID.
     * \param uuid UUID of the reflection to remove.
     * \return True if the reflection was found and removed.
     */
    bool removeHkl(const QUuid &);
    /**
     * \brief Removes an HKL reflection at a given index.
     * \param i Index of the reflection to remove.
     * \return True if the reflection was found and removed.
     */
    bool removeHkl(int);

    /**
     * \brief Resets all scan data, clearing intensity, angle, and HKL vectors.
     */
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

    /**
     * \brief Applies the Korrection routine to correct for tube-tail effects.
     * \param w Input data vector to correct.
     * \param tubeTails If true, apply tube-tail correction; otherwise return a copy.
     * \return Corrected data vector.
     */
    QVector<double> korrw(const QVector<double> &w, bool tubeTails);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Scan::ScanTypes)

#endif

/** EOF **/

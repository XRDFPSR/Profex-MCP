/***************************************************************************
                          graphdatamodel.h  -  description
                             -------------------
    begin                : Tue Feb 11 18:00:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#ifndef GRAPHDATAMODEL_H
#define GRAPHDATAMODEL_H

#include <QVector>
#include <QFileInfo>
#include "../libXrdIO/scan.h"
#include "../libXrdIO/hkl.h"
#include "../libXrdIO/settingsmanager.h"

class GraphDataModel
{
public:
    explicit GraphDataModel();
    ~GraphDataModel();

    /* returns the number of scans in the model */
    int count() const;

    /* returns the number of scan files in the model.
     * This is not the same as ::count() if files with several scans were loaded. */
    int numberOfFiles() const {return nFiles;}

    bool hasData() const;
    bool hasScanData() const;
    bool hasHklData() const;
    QList<int> baseLines() const;
    inline QFileInfo fileInfo() const {return QFileInfo(completeFileName);}

    /* Returns the hard-coded wavelength triplet matching a certain Ka1 wavelength "fixwl" */
    bool getNearestCharacteristicWaveLength(double &ka1, double &ka2, double &kb, double fixwl);

    /* returns the Ka1 wavelength of the model. If no scan file was loaded, returns
     * fallback */
    double getWaveLength(double fallback);

    /* loads a scan file from disk.
     * s: absolute file path
     * uid: file format uid. If empty, it will be determined automatically
     * min = true: read minimum information only (e.g. for refinement cycles)
     * min = false: read complete information (e.g. for completed refinement)
     * sid: Sample Id. Will be ignored for all raw data formats, but will be used for *.prf and *.dia */
    int loadScanFile(const QString &s, const QString &uid, bool min, const QString &sid);
    int loadDiaFile(const QString &s, bool min, const QString &sid, int &ll, int &tl);

    bool saveScanFile(const QString &f, const QString &uid, int n, const QMap<QString, QVariant> &flags);

    /* appends a scan file to the model */
    int addScanFile(const QString &s, const QString &uid, bool min);
    int addScanFile(const QFileInfo &f, const QString &uid, bool min);
    int addDiaFile(const QString &s, const QString &uid, bool min, int &ll, int &tl);
    int addDiaFile(const QFileInfo &f, const QString &uid, bool min, int &ll, int &tl);

    /* reloads the loaded scan file. Appended scans will be discarded */
    int reloadScanFile(bool min);
    int reloadDiaFile(bool min, int &ll, int &tl);

    /* appends an XY scan to the model. The scan can also contain hkl data, but it must contain xy data. */
    void appendScan(const Scan &s);

    /* appends a hkl scan to the model. The scan must not contain xy data. */
    void appendHklScan(const Scan &s);

    /* change the display status of all hkl indices.
     * i = 0: hidden
     * i = 1: normal display
     * i = 2: emphasized
     */
    void setAllHklDisplayStatus(int i);

    /* replaces scan no n with scan s */
    void replaceScan(int n, const Scan &s);

    /* replaces hkl scan no n with hkl scan s */
    void replaceHklScan(int n, const Scan &s);

    /* removes scan no n from the model */
    void removeScan(int);

    /* removes scan with uid from the model */
    void removeScan(const QUuid &);

    /* removes all scans in uid list from the model */
    void removeScans(const QList<QUuid> &);

    /* changes all scans flagged as temporary to permament */
    void keepTemporary();
    void keepTemporary(const QList<QUuid> &);

    /* deletes all scans flagged as temporary */
    void clearTemporary();

    /* forces the model to read the scan color list from the settings
     * and apply the (new) colors */
    void updateScanColors(bool force);
    void updateScanStyles(bool force);

    /* returns the sample id string */
    inline QString getSampleId() const {return sampleId;}

    /* set the type of radiation / wavelength */
    inline void setWavelengthMode(Scan::WavelengthMode w) {wlMode = w;}

    /* sets a override wavelength, scan wavelengths will be ignored if set */
    void setOverrideWavelength(double, const Scan::WavelengthMode &);

    /* returns pointers to specific scans */
    Scan * getScan(int i);
    Scan * getScan(const QUuid &);
    Scan * getFirst();
    Scan * getLast();
    Scan * getFirstXy();
    Scan * getFirstHkl();

    const Scan * first();
    const Scan * last();
    const Scan * at(int);

    /* returns lists with all scans containing xy or hkl data.
     * scans containing both will appear in both vectors. */
    const QVector<const Scan *> xyScans();
    const QVector<const Scan *> hklScans();
    const QVector<const Scan *> allScans();
    const QVector<const Scan *> phaseScans();

    /* removes all data from the model */
    void clear();

    /* flags scan no i as the active scan */
    QVector<Scan *> activeScans();
    void setActiveScans(const QList<int> &);
    void setActiveScans(const QList<QUuid> &);
    QList<int> activeScanIndices() const;
    void clearActiveScans() const;

    /* returns the index of the first background scan, or -1 */
    int backgroundScanIndex();

    /* returns the index of a specific scan */
    int indexOf(const Scan &) const;
    int indexOf(const QUuid &) const;

    /* returns the uid of scan no i */
    QUuid scanUid(int) const;

    /* set and get the angular correction parameters EPS1, EPS2, and EPS3 */
    void setAngularCorrections(double, double, double);
    double getAngularCorrEPS1() const;
    double getAngularCorrEPS2() const;
    double getAngularCorrEPS3() const;

    /* set and get the custom wavelengths read from a lam file */
    inline void setCustomWaveLength(const QMap<QString, double> &m) {customWl = m;}
    inline void clearCustomWaveLength()                             {customWl.clear();}
    inline const QMap<QString, double> * getCustomWaveLength()      {return &customWl;}
    inline Scan::WavelengthMode getWavelengthMode()                 {return wlMode;}

    void normalizeHklScan(Scan *);

    bool sortScanOrder(const QVector<QUuid> &);

    void yOffsetUp(bool);
    void yOffsetDown(bool);
    void xOffsetRight();
    void xOffsetLeft();
    void resetOffset();

    inline bool isStacked() const {return stacked;}

    double getXmin() const;
    double getXmax() const;
    double getYmin() const;
    double getYmax() const;

private:
    SettingsManager *settings;
    QVector<Scan> *graphHeap, *graphBuffer;
    QVector<Hkl>  *strucRefls;
    QVector<QPointF> *integralRanges;
    QStringList integralRangeNames;
    QString completeFileName;
    QString fileUid;
    QString sampleId;
    QVector<QVector<double> > *waveLengthTable;
    double eps1, eps2, eps3;
    QMap<QString, double> customWl;
    double overrideWavelenght;
    bool stacked;
    int nFiles;
    Scan::WavelengthMode wlMode;

    QMap<QUuid, QVector<double> > scanMetrics, metricsBuffer;

    double globalXrangeMin;
    double globalXrangeMax;
    double globalYrangeMin;
    double globalYrangeMax;

    void updateMetrics();
    void countFiles();
    void getScanVisParameters(QList<bool> &, QList<bool> &, QList<double> &, QList<int> &);
    void applyScanVisParameters(const QList<bool> &, const QList<bool> &, const QList<double> &, const QList<int> &);
};

#endif // GRAPHDATAMODEL_H

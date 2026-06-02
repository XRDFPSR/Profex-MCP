/***************************************************************************
                          graphdatacontroller.h  -  description
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

#ifndef GRAPHDATACONTROLLER_H
#define GRAPHDATACONTROLLER_H

#include "graphdatamodel.h"
#include "../libXrdIO/structs.h"
#include <QString>
#include <QDomDocument>

class AbstractGraphView;

class GraphDataController : public QObject
{
    Q_OBJECT
public:
    explicit GraphDataController(GraphDataModel *m, QObject *parent = nullptr);

    void addView(AbstractGraphView *);

    void setViewStatus(const global::RefinementStatus &);

    int loadScanFile(const QString &s, const QString &uid, bool min, const QString &sid, bool upd = true);
    int loadDiaFile(const QString &s, bool min, const QString &sid, bool upd, int &ll, int &tl);
    bool saveScanFile(const QString &f, const QString &uid, int n, const QMap<QString, QVariant> &);
    int addScanFile(const QString &s, const QString &uid, bool min, bool upd = true);
    void appendScan(const Scan &s, bool upd = true);
    void appendHklScan(const Scan &s, bool upd = true);
    void replaceScan(int n, const Scan &s, bool upd = true);
    void replaceHklScan(int n, const Scan &s, bool upd = true);
    void removeScan(int, bool upd = true);
    void removeScan(const QUuid &, bool upd = true);
    void removeScans(const QList<QUuid> &);
    int reloadScanFile(bool min, bool upd);
    int reloadDiaFile(bool min, bool upd, int &ll, int &dl);
    void setAllHklDisplayStatus(int, bool upd);
    QString getSampleId() const;
    QString fileName() const;
    QFileInfo fileInfo() const;
    double getWaveLength(double);
    double getTimePerStep(int);
    int count() const;
    int countFiles() const;
    bool hasData() const;
    bool hasScanData() const;
    bool hasHklData() const;
    QList<int> baseLines() const;
    void updateScanColors(bool force) const;
    void updateScanStyles(bool force) const;
    void setWavelengthMode(const Scan::WavelengthMode &);
    Scan::WavelengthMode getWavelengthMode() const;
    void setOverrideWavelength(double, const Scan::WavelengthMode &);
    bool blockUpdates(bool);

    Scan * getScan(int i);
    Scan * getScan(const QUuid &);
    Scan * getScan(const QString &);
    Scan * getFirst();
    Scan * getLast();
    Scan * getFirstXy();
    Scan * getFirstHkl();
    const Scan * first();
    const Scan * last();
    const Scan * at(int);
    QStringList getAllScanNames() const;
    QStringList getXyScanNames() const;
    QStringList getHklScanNames() const;

    /* returns lists with all scans containing xy or hkl data.
     * scans containing both will appear in both vectors. */
    const QVector<const Scan *> xyScans();
    const QVector<const Scan *> hklScans();
    const QVector<const Scan *> allScans();
    const QVector<const Scan *> phaseScans();

    void setScanVisibility(int, bool);
    void setPhaseVisibility(bool);
    void togglePhaseVisibility();

    Scan * firstActiveScan();
    QVector<Scan *> activeScans();
    void setActiveScans(const QList<int> &);
    void setActiveScans(const QList<QUuid> &);
    QList<int> activeScanIncides() const;
    bool hasActiveScan() const;
    void clearActiveScans() const;

    int backgroundScanIndex() const;
    Scan * backgroundScan();

    int indexOf(const Scan &) const;
    int indexOf(const QUuid &) const;
    QUuid scanUid(int) const;
    QString scanName(const QUuid &);
    QString scanName(const Scan &) const;
    QString scanName(const Scan *) const;

    void setAngularCorrections(double, double, double);
    double getAngularCorrEPS1() const;
    double getAngularCorrEPS2() const;
    double getAngularCorrEPS3() const;

    bool getNearestCharacteristicWaveLength(double &ka1, double &ka2, double &kb, double fixwl);
    void setCustomWaveLength(const QMap<QString, double> &);
    void clearCustomWaveLength();
    const QMap<QString, double> * getCustomWaveLength();

    void normalizeHklScan(Scan *);

    bool sortScanOrder(const QVector<QUuid> &, bool upd = true, const AbstractGraphView *sender = nullptr);

    double getXmin() const;
    double getXmax() const;
    double getYmin() const;
    double getYmax() const;

    void updateViews(const QSet<global::ViewUpdateMode> &modes, const AbstractGraphView *sender = nullptr);
    void resetViews(const AbstractGraphView *sender = nullptr);

    void yOffsetUp(bool);
    void yOffsetDown(bool);
    void xOffsetRight();
    void xOffsetLeft();
    void resetOffset();

    bool isStacked() const;

    /* changes all scans flagged as temporary to permament */
    void keepTemporary();
    void keepTemporary(const QList<QUuid> &);

    /* deletes all scans flagged as temporary */
    void clearTemporary();

    void getPreset(QDomDocument &, const QStringList &skipList = QStringList());
    void applyPreset(const QDomElement &, const QString &dir);

private:
    GraphDataModel *graphModel;
    QList<AbstractGraphView *> graphViews;
    bool updateInProgress;
    bool updatesBlocked;

    void applyPresetV050500(const QDomElement &, const QString &dir);

signals:
    void dataUpdated();
};

#endif // GRAPHDATACONTROLLER_H


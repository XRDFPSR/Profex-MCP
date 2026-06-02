/***************************************************************************
                          scanlistwidget.h  -  description
                             -------------------
    begin                : Thu May 12 15:27:00 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#ifndef SCANLISTWIDGET_H
#define SCANLISTWIDGET_H

#include <QTreeWidget>
#include "scanlistwidgetitem.h"
#include "../graphWidget/abstractgraphview.h"
#include "../libXrdIO/scan.h"

/** \brief Widget listing and managing loaded diffraction scans.
 *
 *  Displays all loaded scans in a tree-like list, showing per-scan
 *   properties (visibility, offsets, scaling, color, style). Supports
 *   sorting, export, removal, copy, noise addition, and context-menu
 *   operations. Emits signals when the active scan changes. */
class ScanListWidget : public AbstractGraphView
{
    Q_OBJECT

public:
    ScanListWidget(GraphDataController *, QWidget *parent = 0);
    ~ScanListWidget();

    void updateView() override;
    inline void getPreset(QDomDocument &) override {};
    inline void applyPreset(const QDomElement &) override {};

    void adjustHeaderSize();
    int currentIndex();
    void initSettings();
    int countItems() const;
    QString getScanName(int) const;
    QStringList getScanNames() const;
    QStringList getActiveScanNames() const;

public slots:
    void sortScans(int);

private:
    QTreeWidget *treeWidget;
    QMenu *slContextMenu;
    QMenu *slHeaderMenu;
    bool headerStateRestored;
    bool redrawBlocked;

    QAction *actExportScan;
    QAction *actRemoveScan;
    QAction *actCopyScan;
    QAction *actCopyScanX;
    QAction *actCopyScanY;
    QAction *actCopyHkl;
    QAction *actExportScanOffset;
    QAction *actCopyScanOffset;
    QAction *actHidePhases;
    QAction *actShowPhases;
    QAction *actAddNoise;
    QAction *actChangeColor;
    QAction *actChangeStyle;
    QAction *actContextHelp;

    void initConnections();
    void storeTreeItemStates(QMap<int, bool> &checkStates, QMap<int, bool> &activeStates);
    void adjustNumberOfTreeItems();
    void generateTreeItems(const QMap<int, bool> &, const QMap<int, bool> &);
    void exportDoDisk(bool);
    bool getExportFileBaseName(const QString &scanName, QString &path, QString &basename, QString &extension, QString &filterUid);
    bool blockRedraw(bool);
    void keyPressEvent(QKeyEvent *e) override;
    void updateScan(QTreeWidgetItem *it, Scan *scan);

    // convenience functions
    ScanListWidgetItem * topLevelScanListItem(int) const;

private slots:
    void treeWidgetHeaderChanged();
    void showContextMenu(const QPoint &);
    void exportScans();
    void exportSingleScanWithAngularCorrections();
    void removeScan();
    void copyScans();
    void copyScansX();
    void copyScansY();
    void copyScansWithAngularCorrections();
    void copyHklData();
    void activeScanChanged();
    void scanParameterChanged(QTreeWidgetItem*, int);
    void changeScanVisibility(Scan *, int);
    void changeScanScaling(Scan *, const QString &);
    void changeScanYOffset(Scan *, const QString &);
    void changeScanXOffset(Scan *, const QString &);
    void addNoise();
    void updateScanOrder();
    void headerContextMenuRequested(QPoint);
    void toggleColumnVisibility(QAction*);
    void hideAllPhases();
    void showAllPhases();
    void changeScanColor();
    void changeScanStyle();
    void showHelp();

signals:
    void currentScanChanged(int);
    void helpText(QString);
};

#endif // SCANLISTWIDGET_H

/***************************************************************************
                          purifyscans.h  -  description
                             -------------------
    begin                : Sat Jun 24 11:00:00 CEST 2017
    copyright            : (C) 2017 by Nicola Doebelin
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

#ifndef PURIFYSCANS_H
#define PURIFYSCANS_H

#include <QDialog>
#include <QStringList>
#include <QColor>
#include <QSettings>
#include <QVector>
#include <QTreeWidgetItem>
#include <QFileInfo>

#include "../libXrdIO/scan.h"
#include "../libXrdIO/settingsmanager.h"
#include "xrdcustomplot.h"

namespace Ui {
class PurifyScans;
}

class PurifyScans : public QDialog
{
    Q_OBJECT

public:
    explicit PurifyScans(QWidget *parent = 0);
    ~PurifyScans();

    void loadFile(const QString &);
    void saveSettings();
    bool hasFile() {return diaFileName.exists();}

private:
    Ui::PurifyScans *ui;
    SettingsManager *settings;
    QVector<Scan> scanHeap;
    Scan purgedScan;
    QFileInfo diaFileName;
    XrdCustomPlot *plotWidget;

    void parseContent(const QString &);
    void writeToFile(const QString &, const QVector<double> &, const QVector<double> &);
    void updateSampleValues();
    void plotDia(int);
    void plotPurged();
    bool uiValuesCheck();
    void precheckItems();
    QList<int> getStripPhases();
    double getMainPhaseK();

private slots:
    void load();
    void saveAs();
    void purge();
    void itemChanged(QTreeWidgetItem *, int);
    void referenceValuesChanged();
};

#endif // PURIFYSCANS_H

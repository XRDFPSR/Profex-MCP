/***************************************************************************
                          atomicscatteringfactordialog.cpp  -  description
                             -------------------
    begin                : Mon Feb 09 08:50:00 CEST 2016
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

#ifndef ATOMICSCATTERINGFACTORDIALOG_H
#define ATOMICSCATTERINGFACTORDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVector>
#include <QColor>
#include <QMouseEvent>
#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/structs.h"
#include "../libXrdIO/functions.h"

namespace Ui {
class AtomicScatteringFactorDialog;
}

struct DataSet {
    QString name;
    QVector<double> data;
};

class AtomicScatteringFactorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AtomicScatteringFactorDialog(QWidget *parent = 0);
    ~AtomicScatteringFactorDialog();

private:
    Ui::AtomicScatteringFactorDialog *ui;

    int lineWidth;
    bool darkMode;

    SettingsManager *settings;
    QString afaParmDat;
    QList<QColor> colorTable;
    QVector<DataSet> xYYdata;
    double wavelength;
    double stepSizeTheta;

    void closeEvent(QCloseEvent *);
    void initSettings();
    void saveSettings();
    void initPlot();
    void initColorTable(const QList<QVariant> &);
    void initListWidgetItems();

    void getData(const global::ScatteringFactorFunction fct, double, DataSet &, DataSet &);
    QColor getColor(int);

    void drawData();
    void clearPlot();

    QString dataToCsv();

private slots:
    void atomSelectionChanged();
    void waveLengthChanged();
    void exportData();
    void savePdf();
    void savePng();
    void cursorCoordinates(QMouseEvent *);
    void bChanged(double);
    void resetB();
    void changeSettings();
};

#endif // ATOMICSCATTERINGFACTORDIALOG_H

/***************************************************************************
                          absorptioncoefficientdialog.h  -  description
                             -------------------
    begin                : Tue Aug 10 20:51:00 CEST 2017
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

#ifndef ABSORPTIONCOEFFICIENTDIALOG_H
#define ABSORPTIONCOEFFICIENTDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include <QStandardItemModel>
#include <QDataWidgetMapper>
#include <QCloseEvent>
#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/structs.h"
#include "../libXrdIO/elementscatteringdatamanager.h"

namespace Ui {
class AbsorptionCoefficientDialog;
}

class AbsorptionCoefficientDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AbsorptionCoefficientDialog(QWidget *parent = 0);
    ~AbsorptionCoefficientDialog();

    void setProjectData(const QString &lst, double wl);

private:
    Ui::AbsorptionCoefficientDialog *ui;
    SettingsManager *settings;
    ElementScatteringDataManager scatData;
    QMap<QString, double> mapMolWeight;
    QMap<QString, double> mapQuantities;
    QStringList headers;
    QString suffixRho;
    QString suffixMac;
    QString suffixLac;
    QString suffixMum;
    QString lstFile;
    QStandardItemModel *model;
    QDataWidgetMapper *mapper;
    double sampleMac;
    double sampleLac;

    void closeEvent(QCloseEvent *e);
    void initSettings();
    void saveSettings();
    void parseLstFile(const QString &);
    // double calcMac(const QMap<QString, double> &);
    // double calcLac(double, double);
    double calcSampleMac();
    double calcSampleLac();
    void recalcCoefficients();
    void recalcPenetration();

    void initData();
    QString selectWavelength(double);
    // QMap<QString, double> parseFormula(const QString &);
    // QMap<QString, double> fractionalFormula(const QMap<QString, double> &);
    double getQuantity(const QString &);
    QMap<QString, double> phaseQuantities(const QList<global::Result> &);
    void appendItem(const QStringList &);

private slots:
    void updateItems();
    void appendItem();
    void loadFile();
    void saveAs();
    void clearCurrent();
    void clearAll();
    void slotCurrentChanged(QModelIndex, QModelIndex);
};

#endif // ABSORPTIONCOEFFICIENTDIALOG_H

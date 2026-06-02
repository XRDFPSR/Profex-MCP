/***************************************************************************
                          tubetailsimulatordialog.h  -  description
                             -------------------
    begin                : Wed Jan 25 18:50:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#ifndef TUBETAILSIMULATORDIALOG_H
#define TUBETAILSIMULATORDIALOG_H

#include <QDialog>
#include "../libXrdIO/settingsmanager.h"
#include "../../3rdparty/qcustomplot/qcustomplot.h"

namespace Ui {
class TubeTailSimulatorDialog;
}

class TubeTailSimulatorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TubeTailSimulatorDialog(QWidget *parent = nullptr);
    ~TubeTailSimulatorDialog();

private:
    Ui::TubeTailSimulatorDialog *ui;
    SettingsManager *settings;
    QVector<double> xval;
    QVector<double> yval;
    QString workingDir;

    QSharedPointer<QCPAxisTickerText> yTicksLin;
    QSharedPointer<QCPAxisTickerText> yTicksSqrt;

    enum yScale {lin, sqrt, log};
    yScale yscale;

    void saveSettings();
    void initSettings();
    void closeEvent(QCloseEvent *);

private slots:
    void updatePlot();
    void exportData();
    void axisScaleChanged(int);
    void resetValues();
};

#endif // TUBETAILSIMULATORDIALOG_H

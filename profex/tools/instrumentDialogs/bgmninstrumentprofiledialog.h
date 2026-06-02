/***************************************************************************
                          bgmninstrumentprofiledialog.h  -  description
                             -------------------
    begin                : Mon May 21 09:30:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#ifndef BGMNINSTRUMENTPROFILEDIALOG_H
#define BGMNINSTRUMENTPROFILEDIALOG_H

#include "bgmngeqdisplay.h"
#include "bgmnlamdisplay.h"
#include "bgmnsampledisplay.h"
#include "bgmnconvolutiondisplay.h"
#include "../../../libXrdIO/settingsmanager.h"
#include "ui_sampdisplayform.h"

#include <QDialog>


namespace Ui {
class BgmnInstrumentProfileDialog;
}

class BgmnInstrumentProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BgmnInstrumentProfileDialog(QWidget *parent = 0);
    ~BgmnInstrumentProfileDialog();

private:
    Ui::BgmnInstrumentProfileDialog *ui;
    SettingsManager *settings;
    Ui::SampDisplayForm *sampControlPanel;

    BgmnGeqDisplay *geqDisplay;
    BgmnLamDisplay *lamDisplay;
    BgmnSampleDisplay *sampDisplay;
    BgmnConvolutionDisplay *convDisplay;

    QString currentLamFile;
    QString currentGeqFile;

private slots:
    void loadFile();
    void reloadFile();
    void exportCsv();
    void exportPdf();
    void exportPng();
    void tabChanged(int);
    void twoThetaChanged(int);
    void toggleSubCurves(bool);
    void b1Changed(double);
    void k1Changed(double);
    void k2Changed(double);
};

#endif // BGMNINSTRUMENTPROFILEDIALOG_H

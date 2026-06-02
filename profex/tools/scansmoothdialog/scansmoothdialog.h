/***************************************************************************
                          scansmoothdialog.h  -  description
                             -------------------
    begin                : Tue Jan 23 22:00:00 CEST 2018
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

#ifndef SCANSMOOTHDIALOG_H
#define SCANSMOOTHDIALOG_H

#include <QDialog>
#include <QVector>
#include <QTableWidgetItem>

#include "tools/abstracttooldialog.h"
#include "projectWidget/graphWidget/graphwindow.h"

#include "../libXrdIO/scan.h"
#include "../libXrdIO/scanops.h"

namespace Ui {
class ScanSmoothDialog;
}

const QString smAuxString = "isSmoothedScan";
enum SmoothAlgoType {SMA, EMA, LRMA, DCT};

class ScanSmoothDialog : public AbstractToolDialog
{
    Q_OBJECT

public:
    explicit ScanSmoothDialog(QWidget *parent = 0);
    ~ScanSmoothDialog();

    void clearTemporary();

private:
    Ui::ScanSmoothDialog *ui;
    QStringList headers;

    void preSetProject(ProjectWidget *);
    void postSetProject(ProjectWidget *);
    void clearGui();
    void initSettings();
    void saveSettings();
    void parseScans();
    void keepTemporary();
    void blockUpdateSignals(bool);
    void computeAlgo(SmoothAlgoType);
    QList<bool> getDCTcoefficients() const;
    void setDCTcoefficients(int);

private slots:
    void updateView();
    void scanChanged();
    void algoChanged(int);
    void compute();
    void computeSma();
    void computeEma();
    void computeLrma();
    void computeDct();
    void dctWindowChanged(int);
    void dctOverlapChanged(int);
    void dctComponentsChanged(int);
    void append();
    void resetSma();
    void resetEma();
    void resetLrma();
    void resetDct();
    void checkDCTcoefficients(int);
};

#endif // SCANSMOOTHDIALOG_H

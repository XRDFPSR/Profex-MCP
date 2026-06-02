/***************************************************************************
                          scanmathdialog.h  -  description
                             -------------------
    begin                : Wed Sep 27 15:10:00 CEST 2017
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

#ifndef SCANMATHDIALOG_H
#define SCANMATHDIALOG_H

#include <QDialog>
#include <QVector>
#include <QTreeWidgetItem>
#include "../../../libXrdIO/scan.h"
#include "../libXrdIO/settingsmanager.h"
#include "tools/abstracttooldialog.h"
#include "scanmathhelpdialog.h"

namespace Ui {
class ScanMathDialog;
}

class ScanMathDialog : public AbstractToolDialog
{
    Q_OBJECT

public:
    explicit ScanMathDialog(QWidget *parent = 0);
    ~ScanMathDialog();

private:
    Ui::ScanMathDialog *ui;
    ScanMathHelpDialog *helpDlg;
    QUuid uid2tt;
    QUuid uidd;

    const Scan * getAnchorScan();
    void clearGui();
    void postSetProject(ProjectWidget *);
    void parseScans();
    QVector<double> intensityQuantized(const QVector<double> &, const QMap<double, double> &);
    QVector<double> dvalues(const QVector<double> &, double wl);

private slots:
    void updateView();
    void compute();
    void scanSelected(QTreeWidgetItem *current, int);
    void scanListContextMenu(QPoint);
    void scanClicked(QTreeWidgetItem *current, int);
    void showInfo();

    void addPlus();
    void addMinus();
    void addMultiply();
    void addDivide();
    void addPow();
    void addSqrt();
    void addAbs();

signals:
    void redraw();
};

#endif // SCANMATHDIALOG_H

/***************************************************************************
                          calcdspacingdialog.h  -  description
                             -------------------
    begin                : Tue Jul 24 18:10:00 CEST 2023
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

#ifndef CALCDSPACINGDIALOG_H
#define CALCDSPACINGDIALOG_H

#include <QDialog>
#include <QCloseEvent>
#include "tools/abstracttooldialog.h"
#include "projectWidget/bgmnprojectwidget.h"

namespace Ui {
class CalcDspacingDialog;
}

class CalcDspacingDialog : public AbstractToolDialog
{
    Q_OBJECT

public:
    explicit CalcDspacingDialog(QWidget *parent = nullptr);
    ~CalcDspacingDialog();

private:
    Ui::CalcDspacingDialog *ui;
    QString workingDir;
    BgmnProjectWidget *project;

    enum CrystalSystem {TRICLINIC, MONOCLINICA, MONOCLINICB, MONOCLINICC, ORTHORHOMBIC, TETRAGONAL, RHOMBOHEDRAL, HEXAGONAL, CUBIC};

    void preSetProject(ProjectWidget *);
    void postSetProject(ProjectWidget *);
    void closeEvent(QCloseEvent *);
    void initSettings();
    void saveSettings();
    void getValues(double &a, double &b, double &c, double &al, double &be, double &ga, int &h, int &k, int &l);
    void calculateResults(double a, double b, double c, double al, double be, double ga, int h, int k, int l, double wl);
    double getLambda();
    QString getOutputText();

private slots:
    void updateData();
    void updateCrystalSystem();
    void clearList();
    void appendLine();
    void removeLine();
    void saveAs();
    void treeHeaderChanged();
    void copyData();
    void readFromProject();
};

#endif // CALCDSPACINGDIALOG_H

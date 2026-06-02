/***************************************************************************
                          bondlengthdialog.h  -  description
                             -------------------
    begin                : Mon Apr 22 20:51:00 CEST 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#ifndef BONDLENGTHDIALOG_H
#define BONDLENGTHDIALOG_H

#include "../libXrdIO/parser/bgmnlstparser.h"
#include "../libXrdIO/parser/bgmnstrparser.h"
#include "../abstracttooldialog.h"
#include "bondlengthdata.h"
#include "projectWidget/bgmnprojectwidget.h"
#include <QDialog>
#include <QProgressDialog>

namespace Ui {
class BondLengthDialog;
}

class BondLengthDialog : public AbstractToolDialog
{
    Q_OBJECT

public:
    explicit BondLengthDialog(QWidget *parent = nullptr);
    ~BondLengthDialog();

public slots:
    void updateData();

private:
    Ui::BondLengthDialog *ui;
    BondLengthData _data;
    BgmnProjectWidget *project;
    QProgressDialog *prgDlg;

    inline void preSetProject(ProjectWidget *) {}
    void postSetProject(ProjectWidget *);
    void closeEvent(QCloseEvent *);
    void initSettings();
    void saveSettings();
    void populateTable(const QString &);
    void clearGui();

private slots:
    void updateTable();
    void saveData();
    void updateProgress(int,QString);
};

#endif // BONDLENGTHDIALOG_H

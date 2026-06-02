/***************************************************************************
                          fpaddremovedialog.h  -  description
                             -------------------
    begin                : Tue June 14 17:00:00 CEST 2015
    copyright            : (C) 2015 by Nicola Doebelin
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

#ifndef FPADDREMOVEDIALOG_H
#define FPADDREMOVEDIALOG_H

#include <QDialog>
#include <QMap>
#include <QString>
#include <QStringList>
#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/parser/fppcrparser.h"

namespace Ui {
    class AddRemovePhaseDialog;
}

class FpAddRemoveDialog : public QDialog
{
    Q_OBJECT

public:
    FpAddRemoveDialog(QWidget *parent = 0);
    ~FpAddRemoveDialog();

    QString getString();
    void setText(const QString &);
    void setProjectDir(const QString &s)      {projectDir = s;}
    void setProjectBasename(const QString &s) {projectBasename = s;}
    void setProjectScanFile(const QString &s) {projectScanFile = s;}

private:
    void initSettings();
    void setStrDir(const QString &);
    void setDevDir(const QString &);
    void setFilters();

    Ui::AddRemovePhaseDialog *ui;
    SettingsManager *settings;
    QString text;
    QString projectDir;
    QString projectBasename;
    QStringList devExt;
    QStringList strExt;
    QString projectScanFile;

private slots:
    void applyFilter(QString);
    void filterOptions(QAction *);
};

#endif // FPADDREMOVEDIALOG_H

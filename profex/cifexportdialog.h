/***************************************************************************
                          cifexportdialog.h  -  description
                             -------------------
    begin                : Sun Jul 24 10:30:00 CEST 2016
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

#ifndef CIFEXPORTDIALOG_H
#define CIFEXPORTDIALOG_H

#include <QDialog>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class CifExportDialog;
}

enum CifExportMode{SINGLE, PROJECT, GLOBAL};

class CifExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CifExportDialog(QWidget *parent = 0);
    ~CifExportDialog();

    CifExportMode getOutputMode();
    QMap<QString, QVariant> getAuxData();

private:
    Ui::CifExportDialog *ui;
    SettingsManager *settings;

    QStringList getAllInstruments();

private slots:
    void accept();
    void addInstrument();
    void removeInstrument();
    void resetTemperature();
};

#endif // CIFEXPORTDIALOG_H

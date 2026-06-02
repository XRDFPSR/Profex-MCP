/***************************************************************************
                          wavelengthselectdialog.h  -  description
                             -------------------
    begin                : Tue Mar 16 21:30:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#ifndef WAVELENGTHSELECTDIALOG_H
#define WAVELENGTHSELECTDIALOG_H

#include <QDialog>
#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/scan.h"

namespace Ui {
class WavelengthSelectDialog;
}

class WavelengthSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WavelengthSelectDialog(QWidget *parent = 0);
    ~WavelengthSelectDialog();

    Scan::WavelengthMode wavelengthMode();
    double wavelength();

private:
    Ui::WavelengthSelectDialog *ui;
    SettingsManager *settings;

private slots:
    void accept();
};

#endif // WAVELENGTHSELECTDIALOG_H

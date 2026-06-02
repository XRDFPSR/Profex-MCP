/***************************************************************************
                          bgmninstrumentselectdialog.h  -  description
                             -------------------
    begin                : Mon Jan 21 16:00:00 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#ifndef BGMNINSTRUMENTSELECTDIALOG_H
#define BGMNINSTRUMENTSELECTDIALOG_H

#include <QDialog>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class BgmnInstrumentSelectDialog;
}

class BgmnInstrumentSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BgmnInstrumentSelectDialog(QWidget *parent = 0);
    ~BgmnInstrumentSelectDialog();

    QString getDevFile();
    QString getLamFile();
    double getSynchrotronValue();
    bool isLamSelected();

private:
    Ui::BgmnInstrumentSelectDialog *ui;
    SettingsManager *settings;

    void init();
    void initSettings();
    void saveSettings();
};

#endif // BGMNINSTRUMENTSELECTDIALOG_H

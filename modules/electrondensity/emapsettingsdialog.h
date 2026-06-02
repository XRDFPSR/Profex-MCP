/***************************************************************************
                          emapsettingsdialog.h  -  description
                             -------------------
    begin                : Wed Sep 24 20:35:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#ifndef EMAPSETTINGSDIALOG_H
#define EMAPSETTINGSDIALOG_H

#include <QDialog>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class EmapSettingsDialog;
}

class EmapSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EmapSettingsDialog(QWidget *parent = 0);
    ~EmapSettingsDialog();

    void setResolution(double);
    void setInterpolation(int);
    void setImageHeight(int);
    void setSuperSampling(int);

    double getResolution();
    int getInterpolation();
    int getImageHeight();
    int getSuperSampling();

private:
    Ui::EmapSettingsDialog *ui;
    SettingsManager *settings;

    void initGui();

private slots:
    void accept();
};



#endif // EMAPSETTINGSDIALOG_H

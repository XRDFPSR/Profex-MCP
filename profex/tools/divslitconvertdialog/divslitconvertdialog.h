/***************************************************************************
                          divslitconvertdialog.h  -  description
                             -------------------
    begin                : Fri Mar 22 21:00:00 CEST 2024
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

#ifndef DIVSLITCONVERTDIALOG_H
#define DIVSLITCONVERTDIALOG_H

#include <QDialog>
#include "tools/abstracttooldialog.h"
#include "projectWidget/graphWidget/graphwindow.h"

#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/scan.h"
#include "../libXrdIO/scanops.h"

namespace Ui {
class DivSlitConvertDialog;
}

class DivSlitConvertDialog : public AbstractToolDialog
{
    Q_OBJECT

public:
    explicit DivSlitConvertDialog(QWidget *parent = nullptr);
    ~DivSlitConvertDialog();

    void clearTemporary();

private:
    Ui::DivSlitConvertDialog *ui;

    enum class DsMode {ADSTOFDS, FDSTOADS};

    void preSetProject(ProjectWidget *);
    void postSetProject(ProjectWidget *);
    void clearGui();
    void initSettings();
    void saveSettings();
    void parseScans();
    void keepTemporary();
    void blockUpdateSignals(bool);

private slots:
    void updateView();
    void compute();
    void append();

};

#endif // DIVSLITCONVERTDIALOG_H

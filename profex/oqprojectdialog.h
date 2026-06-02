/***************************************************************************
                          OqProjectDialog.h  -  description
                             -------------------
    begin                : Thu Mar 05 10:49:00 CEST 2023
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

#ifndef OQPROJECTDIALOG_H
#define OQPROJECTDIALOG_H

#include <QDialog>
#include "oqprojecthandler.h"
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class OqProjectDialog;
}

class OqProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OqProjectDialog(QWidget *parent = nullptr);
    ~OqProjectDialog();

private:
    Ui::OqProjectDialog *ui;
    SettingsManager *settings;

    void initSettings();
    void saveSettings();
    void closeEvent(QCloseEvent *);

private slots:
    void createOqProject();

    void introNext();
    void createPrev();
    void createNext();
    void refStrPrev();
    void refStrNext();
    void presetPrev();
    void presetNext();
    void refinementPrev();
    void refinementNext();
    void finishedPrev();

signals:
    void sigOpenProject(QStringList, QString);
};

#endif // OQPROJECTDIALOG_H

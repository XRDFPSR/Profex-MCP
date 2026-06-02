/***************************************************************************
                          synchrotronpreferencesdialog.h  -  description
                             -------------------
    begin                : Tue Jan 21 18:39:00 CET 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#ifndef SYNCHROTRONPREFERENCESDIALOG_H
#define SYNCHROTRONPREFERENCESDIALOG_H

#include <QDialog>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class SynchrotronPreferencesDialog;
}

class SynchrotronPreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SynchrotronPreferencesDialog(QWidget *parent = nullptr);
    ~SynchrotronPreferencesDialog();

private:
    Ui::SynchrotronPreferencesDialog *ui;
    SettingsManager *settings;

    void initSettings();
    void saveSettings();
    void addDetectorList(const QList<QVariant> &);

private slots:
    void accept();
    void reset();
    void removeDetector();
    void addDetector();
};

#endif // SYNCHROTRONPREFERENCESDIALOG_H

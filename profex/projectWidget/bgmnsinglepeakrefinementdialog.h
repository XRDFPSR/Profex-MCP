/***************************************************************************
                          bgmnsinglepeakrefinementdialog.cpp  -  description
                             -------------------
    begin                : Wed Jan 24 23:00:00 CEST 2024
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

#ifndef BGMNSINGLEPEAKREFINEMENTDIALOG_H
#define BGMNSINGLEPEAKREFINEMENTDIALOG_H

#include <QDialog>
#include "settingsmanager.h"

namespace Ui {
class BgmnSinglePeakRefinementDialog;
}

class BgmnSinglePeakRefinementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BgmnSinglePeakRefinementDialog(QWidget *parent = nullptr);
    ~BgmnSinglePeakRefinementDialog();

    QString deviceFile() const;
    bool usePhaseName() const;
    QString useOtherName() const;
    bool overwrite() const;

private:
    Ui::BgmnSinglePeakRefinementDialog *ui;
    SettingsManager *settings;

    void setupDevFiles();
    void initSettings();
    void saveSettings();

public slots:
    void accept();

};

#endif // BGMNSINGLEPEAKREFINEMENTDIALOG_H

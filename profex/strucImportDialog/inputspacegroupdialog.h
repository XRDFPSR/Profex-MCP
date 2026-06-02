/***************************************************************************
                          inputspacegroupdialog.h  -  description
                             -------------------
    begin                : Tue Sept 01 20:30:00 CEST 2013
    copyright            : (C) 2013 by Nicola Doebelin
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

#ifndef INPUTSPACEGROUPDIALOG_H
#define INPUTSPACEGROUPDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QListWidgetItem>
#include "../libXrdIO/parser/bgmnsgdatparser.h"
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class InputSpacegroupDialog;
}

class InputSpacegroupDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit InputSpacegroupDialog(QWidget *parent = 0);
    ~InputSpacegroupDialog();

    void setSpaceGroups(const QMap<int, QMap<int, BgmnSpaceGroup> > &);
    void setLabelText(const QString &);

    int intTableNo() const;
    int settingNo() const;
    
private:
    Ui::InputSpacegroupDialog *ui;
    SettingsManager *settings;
    QMap<int, QMap<int, BgmnSpaceGroup> > sgMap;

    void toggleOkButtonState();

private slots:
    void currentNumberChanged(QListWidgetItem *, QListWidgetItem*);
    void currentSettingChanged(QListWidgetItem *, QListWidgetItem*);
};

#endif // INPUTSPACEGROUPDIALOG_H

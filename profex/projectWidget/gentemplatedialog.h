/***************************************************************************
                          gentemplatedialog.h  -  description
                             -------------------
    begin                : Tue Mar 12 11:00:00 CEST 2013
    copyright            : (C) 2012 by Nicola Doebelin
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

#ifndef GENTEMPLATEDIALOG_H
#define GENTEMPLATEDIALOG_H

#include <QDialog>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class GenTemplateDialog;
}

class GenTemplateDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit GenTemplateDialog(QWidget *parent = Q_NULLPTR);
    ~GenTemplateDialog();
    
    void setBgmnDir(const QString &);
    QString getString();

private:
    Ui::GenTemplateDialog *ui;

    SettingsManager *settings;
    QString tplString;
    QString polString;

    QStringList getWavelengths(const QString &);

private slots:
    void accept();
};

#endif // GENTEMPLATEDIALOG_H

/***************************************************************************
                          bgmnindexerrordialog.h  -  description
                             -------------------
    begin                : Tue May 07 16:20:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#ifndef BGMNINDEXINGERRORDIALOG_H
#define BGMNINDEXINGERRORDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class BgmnIndexingErrorDialog;
}

class BgmnIndexingErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BgmnIndexingErrorDialog(QWidget *parent = 0);
    ~BgmnIndexingErrorDialog();

    void setErrorLabel(const QString &);
    void setErrorString(const QStringList &);

private:
    Ui::BgmnIndexingErrorDialog *ui;
    SettingsManager *settings;

    void initSettings();
    void saveSettings();
    QString textToPlainText(const QString &);

private slots:
    void close();
    void fileSelectionChanged(QListWidgetItem *, QListWidgetItem *);
    void copyToClipboard();
};

#endif // BGMNINDEXINGERRORDIALOG_H

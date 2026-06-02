/***************************************************************************
                          helpaboutdialog.h  -  description
                             -------------------
    begin                : Sun Mar 29 12:00:00 CEST 2015
    copyright            : (C) 2015 by Nicola Doebelin
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

#ifndef HELPABOUTDIALOG_H
#define HELPABOUTDIALOG_H

#include <QDialog>

namespace Ui {
class HelpAboutDialog;
}

class HelpAboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpAboutDialog(QWidget *parent = 0);
    ~HelpAboutDialog();

    void setVersion(const QString &s) {version = s;}
    void setLogDestination(const QString &s) {logDest = s;}

public slots:
    int exec();

private:
    Ui::HelpAboutDialog *ui;
    QString version;
    QString logDest;

    void init();
    QString getAboutText();
    QString getSettingsText();
    QString getSysInfoText();
    QString getAckText();

private slots:
    void toClipboard();
};

#endif // HELPABOUTDIALOG_H

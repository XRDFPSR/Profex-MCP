/***************************************************************************
                          reportcustomsectiondialog.h  -  description
                             -------------------
    begin                : Jul 26 18:50:00 CET 2019
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

#ifndef REPORTCUSTOMSECTIONDIALOG_H
#define REPORTCUSTOMSECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class ReportCustomSectionDialog;
}

class ReportCustomSectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReportCustomSectionDialog(QWidget *parent = 0);
    ~ReportCustomSectionDialog();

    void clearData();
    void setContent(const QString &name, const QString &text);
    QString getName();
    QString getText();

private:
    Ui::ReportCustomSectionDialog *ui;

private slots:
    void loadFile();
};

#endif // REPORTCUSTOMSECTIONDIALOG_H

/***************************************************************************
                          internalstandarddialog.h  -  description
                             -------------------
    begin                : Thu Sep 03 21:07:00 CEST 2015
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

#ifndef INTERNALSTANDARDDIALOG_H
#define INTERNALSTANDARDDIALOG_H

#include <QStringList>
#include <QString>
#include <QDialog>

namespace Ui {
class InternalStandardDialog;
}

class InternalStandardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InternalStandardDialog(QWidget *parent = 0);
    ~InternalStandardDialog();

    void setPhases(const QStringList &);
    void setCurrentPhase(const QString &);
    void setCurrentQuantity(double);

    QString getPhase();
    double getQuantity();

private:
    Ui::InternalStandardDialog *ui;
};

#endif // INTERNALSTANDARDDIALOG_H

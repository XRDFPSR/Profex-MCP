/***************************************************************************
                          oxidedialog.h  -  description
                             -------------------
    begin                : Wed Aug 20 12:12:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#ifndef OXIDEDIALOG_H
#define OXIDEDIALOG_H

#include <QDialog>
#include <QHash>

namespace Ui {
class OxideDialog;
}

class OxideDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OxideDialog(QWidget *parent = 0);
    ~OxideDialog();

    void setIon(const QString &);
    void setOxide(const QString &);

    QString getOxide();
    double getMolWeight();

private:
    Ui::OxideDialog *ui;
    QString oxName;
    double mWeight;
    QHash<QString, double> atWeights;


private slots:
    void recalc();
};

#endif // OXIDEDIALOG_H

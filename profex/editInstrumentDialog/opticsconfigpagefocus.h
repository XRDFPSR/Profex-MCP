/***************************************************************************
                          optionsconfigpagefocus.h  -  description
                             -------------------
    begin                : Tue Jul 16 18:00:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#ifndef OPTICSCONFIGPAGEFOCUS_H
#define OPTICSCONFIGPAGEFOCUS_H

#include "abstractopticsconfigpage.h"
#include "ui_opticsconfigpagefocusform.h"
#include <QWidget>

namespace Ui {
class OpticsConfigPageFocusForm;
}

class OpticsConfigPageFocus : public AbstractOpticsConfigPage
{
    Q_OBJECT

public:
    OpticsConfigPageFocus(const QString &t, QWidget *parent = nullptr);

    QMap<QString, QString> setParameters(const QMap<QString, QString> &);
    QMap<QString, QString> getParameters();
    QString helpText();
    QString getLambda() const;
    double getSynchrotron() const;
    void setLambda(const QString &);
    void setSynchrotron(double);

    bool hasLambda() const;
    bool hasSynchrotron() const;

private:
    Ui::OpticsConfigPageFocusForm *ui;

private slots:
    void selectTubeTails();
    void clearTubeTails();
};

#endif // OPTICSCONFIGPAGEFOCUS_H

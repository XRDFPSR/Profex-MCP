/***************************************************************************
                          optionsconfigpagemonochromator.h  -  description
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

#ifndef OPTICSCONFIGPAGEMONOCHROMATOR_H
#define OPTICSCONFIGPAGEMONOCHROMATOR_H

#include "abstractopticsconfigpage.h"
#include "ui_opticsconfigpagemonochromatorform.h"
#include <QWidget>

namespace Ui {
class OpticsConfigPageMonochromatorForm;
}

class OpticsConfigPageMonochromator : public AbstractOpticsConfigPage
{
    Q_OBJECT

public:
    OpticsConfigPageMonochromator(const QString &t, QWidget *parent = nullptr);

    QMap<QString, QString> setParameters(const QMap<QString, QString> &);
    QMap<QString, QString> getParameters();
    QString helpText();

private:
    Ui::OpticsConfigPageMonochromatorForm *ui;
};

#endif // OPTICSCONFIGPAGEMONOCHROMATOR_H

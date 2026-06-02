/***************************************************************************
                          optionsconfigpagesslit.h  -  description
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

#ifndef OPTICSCONFIGPAGESSLIT_H
#define OPTICSCONFIGPAGESSLIT_H

#include "abstractopticsconfigpage.h"
#include "ui_opticsconfigpagesslitform.h"
#include <QWidget>

namespace Ui {
class OpticsConfigPageSslitForm;
}

class OpticsConfigPageSslit : public AbstractOpticsConfigPage
{
    Q_OBJECT

public:
    OpticsConfigPageSslit(const QString &t, QWidget *parent = nullptr);

    QMap<QString, QString> setParameters(const QMap<QString, QString> &);
    QMap<QString, QString> getParameters();
    QString helpText();

private:
    Ui::OpticsConfigPageSslitForm *ui;
    QString sSlitWstringFix;
    QString sSlitWstringVar;
};

#endif // OPTICSCONFIGPAGESSLIT_H

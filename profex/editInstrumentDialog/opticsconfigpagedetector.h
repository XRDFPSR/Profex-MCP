/***************************************************************************
                          optionsconfigpagedetector.h  -  description
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

#ifndef OPTICSCONFIGPAGEDETECTOR_H
#define OPTICSCONFIGPAGEDETECTOR_H

#include "abstractopticsconfigpage.h"
#include "ui_opticsconfigpagedetectorform.h"
#include <QWidget>

namespace Ui {
class OpticsConfigPageDetector;
}

class OpticsConfigPageDetector : public AbstractOpticsConfigPage
{
    Q_OBJECT

public:
    OpticsConfigPageDetector(const QString &t, QWidget *parent = nullptr);

    QMap<QString, QString> setParameters(const QMap<QString, QString> &);
    QMap<QString, QString> getParameters();
    QString helpText();

private:
    Ui::OpticsConfigPageDetectorForm *ui;

private slots:
    void setModel();
};

#endif // OPTICSCONFIGPAGEDETECTOR_H

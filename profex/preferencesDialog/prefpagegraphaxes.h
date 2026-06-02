/***************************************************************************
                          prefpagegraphaxes.h  -  description
                             -------------------
    begin                : Wed Apr 27 16:00:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#ifndef PREFPAGEGRAPHAXES_H
#define PREFPAGEGRAPHAXES_H

#include <QWidget>
#include "prefpagetemplate.h"

namespace Ui {
class PrefPageGraphAxes;
}

class PrefPageGraphAxes : public PrefPageTemplate
{
    Q_OBJECT

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "GraphAxes";}

public:
    explicit PrefPageGraphAxes(QWidget *parent = nullptr);
    ~PrefPageGraphAxes();

private:
    Ui::PrefPageGraphAxes *ui;
};

#endif // PREFPAGEGRAPHAXES_H

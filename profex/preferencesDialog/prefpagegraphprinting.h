/***************************************************************************
                          prefpagegraphprinting.h  -  description
                             -------------------
    begin                : Wed May 10 14:00:00 CEST 2017
    copyright            : (C) 2017 by Nicola Doebelin
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

#ifndef PREFPAGEGRAPHPRINTING_H
#define PREFPAGEGRAPHPRINTING_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageGraphPrinting;
}

class PrefPageGraphPrinting : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageGraphPrinting(QWidget *parent = 0);
    ~PrefPageGraphPrinting();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "GraphPrinting";}

private:
    Ui::PrefPageGraphPrinting *ui;
};

#endif // PREFPAGEGRAPHPRINTING_H

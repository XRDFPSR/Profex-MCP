/***************************************************************************
                          prefpagegeneral.h  -  description
                             -------------------
    begin                : Tue May 09 16:00:00 CEST 2017
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


#ifndef PREFPAGEGENERAL_H
#define PREFPAGEGENERAL_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageGeneral;
}

enum ColType {File, Phase, Comment};

class PrefPageGeneral : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageGeneral(QWidget *parent = 0);
    ~PrefPageGeneral();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "General";}

private:
    Ui::PrefPageGeneral *ui;
};

#endif // PREFPAGEGENERAL_H

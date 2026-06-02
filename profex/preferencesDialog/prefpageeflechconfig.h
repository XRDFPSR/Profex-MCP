/***************************************************************************
                          prefpageeflechconfig.h  -  description
                             -------------------
    begin                : Tue Jan 22 19:00:00 CET 2019
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

#ifndef PREFPAGEEFLECHCONFIG_H
#define PREFPAGEEFLECHCONFIG_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageEflechConfig;
}

class PrefPageEflechConfig : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageEflechConfig(QWidget *parent = 0);
    ~PrefPageEflechConfig();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "EflechConfig";}

private:
    Ui::PrefPageEflechConfig *ui;

private slots:
    void resetSettings();
};

#endif // PREFPAGEEFLECHCONFIG_H

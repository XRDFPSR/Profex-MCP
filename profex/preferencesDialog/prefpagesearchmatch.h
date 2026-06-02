/***************************************************************************
                          prefpagelimits.h  -  description
                             -------------------
    begin                : Wed May 10 10:00:00 CEST 2017
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

#ifndef PREFPAGESEARCHMATCH_H
#define PREFPAGESEARCHMATCH_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageSearchMatch;
}

class PrefPageSearchMatch : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageSearchMatch(QWidget *parent = 0);
    ~PrefPageSearchMatch();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "SearchMatch";}

private:
    Ui::PrefPageSearchMatch *ui;

private slots:
    void resetValues();

};

#endif // PREFPAGESEARCHMATCH_H

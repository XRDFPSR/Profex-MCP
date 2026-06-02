/***************************************************************************
                          prefpagegraphcursors.h  -  description
                             -------------------
    begin                : Fri Mar 04 16:00:00 CEST 2022
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

#ifndef PREFPAGEGRAPHCURSORS_H
#define PREFPAGEGRAPHCURSORS_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageGraphCursors;
}

class PrefPageGraphCursors : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageGraphCursors(QWidget *parent = nullptr);
    ~PrefPageGraphCursors();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "GraphCursors";}

private:
    Ui::PrefPageGraphCursors *ui;
};

#endif // PREFPAGEGRAPHCURSORS_H

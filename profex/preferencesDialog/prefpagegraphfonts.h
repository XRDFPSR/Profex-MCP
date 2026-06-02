/***************************************************************************
                          prefpagegraphfonts.h  -  description
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

#ifndef PREFPAGEGRAPHFONTS_H
#define PREFPAGEGRAPHFONTS_H

#include "prefpagetemplate.h"
#include <QFont>

namespace Ui {
class PrefPageGraphFonts;
}

class PrefPageGraphFonts : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageGraphFonts(QWidget *parent = 0);
    ~PrefPageGraphFonts();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "GraphFonts";}

private:
    Ui::PrefPageGraphFonts *ui;

    QFont fontTitle;
    QFont fontAxis;
    QFont fontLegend;
    QFont fontTick;

private slots:
    void selectFontTitle();
    void selectFontAxis();
    void selectFontTick();
    void selectFontLegend();

};

#endif // PREFPAGEGRAPHFONTS_H

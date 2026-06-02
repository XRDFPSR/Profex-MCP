/***************************************************************************
                          prefpagefullprofconfig.cpp  -  description
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

#ifndef PREFPAGECHEMICALCOMP_H
#define PREFPAGECHEMICALCOMP_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageChemicalComp;
}

class PrefPageChemicalComp : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageChemicalComp(QWidget *parent = 0);
    ~PrefPageChemicalComp();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "ChemicalComposition";}

private:
    Ui::PrefPageChemicalComp *ui;

    void setOxides(const QStringList &);
    QStringList getOxides();
    QStringList defaultOxideWeights();

private slots:
    void resetChemistry();
    void editChemTableCell(int, int);
    void currentMode(int);
};

#endif // PREFPAGECHEMICALCOMP_H

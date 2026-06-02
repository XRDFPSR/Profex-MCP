/***************************************************************************
                          prefpagegraphs.h  -  description
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

#ifndef PREFPAGEGRAPHS_H
#define PREFPAGEGRAPHS_H

#include "prefpagetemplate.h"
#include <QColor>

namespace Ui {
class PrefPageGraphs;
}

class PrefPageGraphs : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageGraphs(QWidget *parent = nullptr);
    ~PrefPageGraphs();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "Graphs";}

private:
    Ui::PrefPageGraphs *ui;
    QColor colBgActive;
    QColor colBgIdle;
    QColor colBgError;
    QColor colBgComplete;
    QColor colBgBackground;

private slots:
    void graphIdleColorChanged();
    void graphActiveColorChanged();
    void graphErrorColorChanged();
    void graphCompleteColorChanged();
    void customBackgroundColor();
};

#endif // PREFPAGEGRAPHS_H

/***************************************************************************
                          prefpagereferencelines.h  -  description
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

#ifndef PREFPAGEREFERENCELINES_H
#define PREFPAGEREFERENCELINES_H

#include "prefpagetemplate.h"
#include "../projectWidget/bgmnrefstructuremanager.h"

namespace Ui {
class PrefPageReferenceLines;
}

class PrefPageReferenceLines : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageReferenceLines(QWidget *parent = 0);
    ~PrefPageReferenceLines();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "ReferenceLines";}

private:
    Ui::PrefPageReferenceLines *ui;
    BgmnRefStructureManager *refStrManager;
    QColor hklLineCol;

    void initConnections();

private slots:
    void clearHklBuffer();
    void selectHklLineColor();
    void selectBufferFileLocation();
    void selectTempFileLocation();
    void showHklResetText();
};

#endif // PREFPAGEREFERENCELINES_H

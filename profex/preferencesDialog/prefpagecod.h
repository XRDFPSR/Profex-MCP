/***************************************************************************
                          prefpagecod.h  -  description
                             -------------------
    begin                : Wed Apr 05 19:00:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#ifndef PREFPAGECOD_H
#define PREFPAGECOD_H

#include "prefpagetemplate.h"
#include <../libXrdIO/coddbmanager.h>

namespace Ui {
class PrefPageCod;
}

class PrefPageCod : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageCod(QWidget *parent = 0);
    ~PrefPageCod();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "CodDatabase";}

private:
    Ui::PrefPageCod *ui;
    CodDbManager *codManager;

    bool openDb(const QString &);

private slots:
    void selectDatabase();
    void toggleUseDb(bool);
    void selectCifDir();
};

#endif // PREFPAGECOD_H

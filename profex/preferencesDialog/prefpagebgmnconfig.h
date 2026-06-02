/***************************************************************************
                          prefpagebgmnconfig.h  -  description
                             -------------------
    begin                : Wed May 10 09:00:00 CEST 2017
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

#ifndef PREFPAGEBGMNCONFIG_H
#define PREFPAGEBGMNCONFIG_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageBgmnConfig;
}

class PrefPageBgmnConfig : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageBgmnConfig(QWidget *parent = 0);
    ~PrefPageBgmnConfig();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "BgmnConfig";}

private:
    Ui::PrefPageBgmnConfig *ui;

    QString workingDir;

    void autoGuessBgmnExe(const QString &s);
    QString getExec(const QString &s);

private slots:
    void selectBgmnExec();
    void selectMakeGeqExec();
    void selectGeometExec();
    void selectTeilExec();
    void selectEflechExec();
    void toggleCustomBgmnEnabled(bool);
    void resetXYFormats();
};

#endif // PREFPAGEBGMNCONFIG_H

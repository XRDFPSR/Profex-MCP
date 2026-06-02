/***************************************************************************
                          prefpagefullprofconfig.h  -  description
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

#ifndef PREFPAGEFULLPROFCONFIG_H
#define PREFPAGEFULLPROFCONFIG_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageFullprofConfig;
}

class PrefPageFullprofConfig : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageFullprofConfig(QWidget *parent = 0);
    ~PrefPageFullprofConfig();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "FullprofConfig";}

private:
    Ui::PrefPageFullprofConfig *ui;
    QString workingDir;

    QString getExec(const QString &s);

private slots:
    void selectFpExec();
    void selectFpStrFiles();
    void selectFpDevFiles();
};

#endif // PREFPAGEFULLPROFCONFIG_H

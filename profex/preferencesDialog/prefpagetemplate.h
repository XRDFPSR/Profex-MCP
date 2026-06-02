/***************************************************************************
                          prefpagetemplate.h  -  description
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

#ifndef PREFPAGETEMPLATE_H
#define PREFPAGETEMPLATE_H

#include <QWidget>
#include <QDebug>
#include "../libXrdIO/settingsmanager.h"

class PrefPageTemplate : public QWidget
{
    Q_OBJECT
public:
    explicit PrefPageTemplate(QWidget *parent = 0);
    ~PrefPageTemplate();

    virtual void initUi() = 0;
    virtual void initSettings() = 0;
    virtual void saveSettings() = 0;
    virtual QString name() {return QString("Template");}

protected:
    SettingsManager *settings;

};

#endif // PREFPAGETEMPLATE_H

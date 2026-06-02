/***************************************************************************
                          prefpagebgmndirectories.h  -  description
                             -------------------
    begin                : Sun Aug 06 09:40:00 CEST 2017
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

#ifndef PREFPAGEBGMNDIRECTORIES_H
#define PREFPAGEBGMNDIRECTORIES_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageBgmnDirectories;
}

class PrefPageBgmnDirectories : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageBgmnDirectories(QWidget *parent = 0);
    ~PrefPageBgmnDirectories();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "BgmnDirectories";}

private:
    Ui::PrefPageBgmnDirectories *ui;
    bool strReposChanged;
    QString lastDir;

    void addBgmnStrDirs(const QStringList &, const QStringList &);
    void addBgmnDevDirs(const QStringList &);
    void addBgmnPresetDirs(const QStringList &);

private slots:
    void addBgmnStrDir();
    void removeBgmnStrDir();
    void addBgmnDevDir();
    void removeBgmnDevDir();
    void addBgmnPresetDir();
    void removeBgmnPresetDir();
    void indexingRequired();

};

#endif // PREFPAGEBGMNDIRECTORIES_H

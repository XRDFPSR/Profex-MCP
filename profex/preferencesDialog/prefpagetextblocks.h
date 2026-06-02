/***************************************************************************
                          prefpagetextblocks.h  -  description
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

#ifndef PREFPAGETEXTBLOCKS_H
#define PREFPAGETEXTBLOCKS_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageTextBlocks;
}

class PrefPageTextBlocks : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageTextBlocks(QWidget *parent = 0);
    ~PrefPageTextBlocks();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "TextBlocks";}

private:
    Ui::PrefPageTextBlocks *ui;
    int previousTextBlock;

    void initTextBlocks(const QMap<QString, QVariant> &);
    QMap<QString, QVariant> getTextBlocks();
    void saveTextBlockChanges();

private slots:
    void textBlockAdd();
    void textBlockRemove();
    void textBlockChanged(int);
};

#endif // PREFPAGETEXTBLOCKS_H

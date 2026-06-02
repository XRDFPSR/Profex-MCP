/***************************************************************************
                          prefpagetexteditor.h  -  description
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

#ifndef PREFPAGETEXTEDITOR_H
#define PREFPAGETEXTEDITOR_H

#include "prefpagetemplate.h"
#include <QFont>

namespace Ui {
class PrefPageTextEditor;
}

class PrefPageTextEditor : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageTextEditor(QWidget *parent = 0);
    ~PrefPageTextEditor();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "TextEditor";}

private:
    Ui::PrefPageTextEditor *ui;
    QFont editorFont;

private slots:
    void selectEdFont();
};

#endif // PREFPAGETEXTEDITOR_H

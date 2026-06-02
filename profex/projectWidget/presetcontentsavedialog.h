/***************************************************************************
                          PresetContentSaveDialog.cpp  -  description
                             -------------------
    begin                : Sat Sep 19 10:30:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#ifndef PRESETCONTENTSAVEDIALOG_H
#define PRESETCONTENTSAVEDIALOG_H

#include <QDialog>

namespace Ui {
class PresetContentSaveDialog;
}

class PresetContentSaveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PresetContentSaveDialog(QWidget *parent = nullptr);
    ~PresetContentSaveDialog();

    void setCheckStatus(bool, bool, bool, bool, bool, bool);
    void getCheckStatus(bool &, bool &, bool &, bool &, bool &, bool &);

private:
    Ui::PresetContentSaveDialog *ui;
};

#endif // PRESETCONTENTSAVEDIALOG_H

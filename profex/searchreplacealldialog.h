/***************************************************************************
                          searchreplacealldialog.h  -  description
                             -------------------
    begin                : Mon Mar 28 10:30:00 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#ifndef SEARCHREPLACEALLDIALOG_H
#define SEARCHREPLACEALLDIALOG_H

#include <QDialog>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class SearchReplaceAllDialog;
}

class SearchReplaceAllDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchReplaceAllDialog(QWidget *parent = 0);
    ~SearchReplaceAllDialog();

    SettingsManager *settings;
    QString findString();
    QString replaceString();
    bool caseSensitive();
    bool wholeWords();
    bool isRegExp();

public slots:
    void accept();

private:
    Ui::SearchReplaceAllDialog *ui;
};

#endif // SEARCHREPLACEALLDIALOG_H

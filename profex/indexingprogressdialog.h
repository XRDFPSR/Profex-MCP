/***************************************************************************
                          indexingprogressdialog.h  -  description
                             -------------------
    begin                : Fri Jan 07 18:30:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#ifndef INDEXINGPROGRESSDIALOG_H
#define INDEXINGPROGRESSDIALOG_H

#include <QDialog>
#include <QTimer>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class IndexingProgressDialog;
}

class IndexingProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IndexingProgressDialog(QWidget *parent = nullptr);
    ~IndexingProgressDialog();

private:
    Ui::IndexingProgressDialog *ui;
    SettingsManager *settings;

    QTimer _skipTimer;
    int _secKillTimer;
    int _secsSinceStart;

    void showEvent(QShowEvent *);

public slots:
    void setMaximum(int);
    void setValue(int);
    void setLabelText(QString);
    void reset();

private slots:
    void skip();
    void abort();
    void skipTimeUpdate();
    void stopTimer();

signals:
    void canceled();
    void skipped();
};

#endif // INDEXINGPROGRESSDIALOG_H

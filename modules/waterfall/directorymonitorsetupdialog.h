/***************************************************************************
                          directorymonitorsetupdialog.h  -  description
                             -------------------
    begin                : Wed Aug 25 23:00:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#ifndef DIRECTORYMONITORSETUPDIALOG_H
#define DIRECTORYMONITORSETUPDIALOG_H

#include <QDialog>

namespace Ui {
class DirectoryMonitorSetupDialog;
}

class DirectoryMonitorSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DirectoryMonitorSetupDialog(QWidget *parent = nullptr);
    ~DirectoryMonitorSetupDialog();

    void setDir(const QString &);
    void setWorkingDir(const QString &);
    void setFileFormats(const QStringList &, const QList<QStringList> &);
    void setCurrentFormat(int);

    QString getDir();
    int getFormat();
    QStringList getCurrentFilter();

private:
    Ui::DirectoryMonitorSetupDialog *ui;
    QString workingDir;

private slots:
    void selectDir();
};

#endif // DIRECTORYMONITORSETUPDIALOG_H

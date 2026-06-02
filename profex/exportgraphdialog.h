/***************************************************************************
                          exportgraphdialog.h  -  description
                             -------------------
    begin                : Sun Jun 02 11:00:00 CEST 2013
    copyright            : (C) 2012 by Nicola Doebelin
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

#ifndef EXPORTGRAPHDIALOG_H
#define EXPORTGRAPHDIALOG_H

#include "../libXrdIO/export/exporthandler.h"
#include "../libXrdIO/import/importhandler.h"
#include "../libXrdIO/settingsmanager.h"

#include <QDialog>
#include <QStringList>
#include <QFileInfo>

namespace Ui {
class ExportGraphDialog;
}

class ExportGraphDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ExportGraphDialog(QWidget *parent = 0);
    ~ExportGraphDialog();

    void setFiles(const QStringList &);
    
private:
    Ui::ExportGraphDialog *ui;
    SettingsManager *settings;
    QFileInfo currentFile;

    QVector<Scan> scanHeap;
    ImportHandler *iHandler;
    ExportHandler *exHandler;

private slots:
    void convert();
    void addFile();
    void removeFile();

};

#endif // EXPORTGRAPHDIALOG_H

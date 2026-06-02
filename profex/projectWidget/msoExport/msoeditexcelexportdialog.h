/***************************************************************************
                          msoeditexcelexport.h  -  description
                             -------------------
    begin                : Sun Aug 09 08:25:00 CEST 2020
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

#ifndef MSOEDITEXCELEXPORTDIALOG_H
#define MSOEDITEXCELEXPORTDIALOG_H

#include "../libXrdIO/structs.h"
#include "../libXrdIO/settingsmanager.h"
#include <QDialog>
#include <QMap>
#include <QList>
#include <QString>
#include <QVariant>

namespace Ui {
class MsoEditExcelExportDialog;
}

class MsoEditExcelExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MsoEditExcelExportDialog(QWidget *parent = nullptr);
    ~MsoEditExcelExportDialog();

    void initData(const QMap<QString, QVariant> &, const QMap<QString, QList<global::Result> > &, const QList<global::AxObjectExcel> &);
    QList<global::AxObjectExcel> getConfig();

private:
    Ui::MsoEditExcelExportDialog *ui;
    SettingsManager *settings;
    QMap<QString, QStringList> parameterMap;

    void checkFileButtons(const QList<global::AxObjectExcel> &);
    void updateParameterMap(const QMap<QString, QVariant> &, const QMap<QString, QList<global::Result> > &);
    void updateTable(const QList<global::AxObjectExcel> &);
    void addTableRow(const global::AxObjectExcel &);

private slots:
    void selectFile();
    void addParam();
    void removeParam();
    void updateParamCBox();
};

#endif // MSOEDITEXCELEXPORTDIALOG_H

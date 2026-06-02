/***************************************************************************
                          strfilebatcheditdialog.h  -  description
                             -------------------
    begin                : Sun Dec 23 11:00:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#ifndef STRFILEBATCHEDITDIALOG_H
#define STRFILEBATCHEDITDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class StrFileBatchEditDialog;
}

class StrFileBatchEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StrFileBatchEditDialog(QWidget *parent = nullptr);
    ~StrFileBatchEditDialog();

    void reset();
    void setStrFileList(const QStringList &);
    void setOpenFileList(const QStringList &l);
    void setCurrentFile(const QString &s);
    QMap<QString, QVariant> getParameters();

private:
    Ui::StrFileBatchEditDialog *ui;
    SettingsManager *settings;
    double coordLimits;
    double ucellLimits;
    double b1Limit;
    double k2Limit;
    double tdsLimit;
    QStringList openFileList;
    QString currentFile;

    void initSettings();
    void initParameterWidget();
    bool isToplevelItem(const QTreeWidgetItem *);
    void setParamsGroupCheckState(bool);

private slots:
    void apply();
    void filesCheckAll();
    void filesUncheckAll();
    void filesCheckCurrent();
    void filesCheckOpen();
    void parametersCheckGroup();
    void parametersUncheckGroup();
    void parametersToggleGroup();

signals:
    void sigApply();
};

#endif // STRFILEBATCHEDITDIALOG_H

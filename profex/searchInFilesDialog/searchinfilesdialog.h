/***************************************************************************
                          searchinfilesdialog.h  -  description
                             -------------------
    begin                : Thu Jul 02 18:00:00 CEST 2020
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

#ifndef SEARCHINFILESDIALOG_H
#define SEARCHINFILESDIALOG_H

#include <QUuid>
#include <QDialog>
#include <QTreeWidgetItem>
#include <QTreeView>
#include <QStandardItemModel>
#include <QLineEdit>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class SearchInFilesDialog;
}

class SearchInFilesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchInFilesDialog(QWidget *parent = 0);
    ~SearchInFilesDialog();

    QString getProjectSelection();
    QString getFileTypeSelection();
    QString getSearchExpression();

    void appendResults(const QString &, const QUuid &, const QMap<QString, QMap<int, QStringList> > &);

private:
    Ui::SearchInFilesDialog *ui;
    SettingsManager *settings;
    QStandardItemModel matchModel;
    QLineEdit *exprLineEdit;
    QStringList headerLabels;

    void initSettings();
    void saveSettings();
    void closeEvent(QCloseEvent *);
    QString getCsvString();

private slots:
    void search();
    void exportResults();
    void itemDoubleClicked(QModelIndex i);

signals:
    void runSearch();
    void projectFileSelected(QUuid, QString, int);
};

#endif // SEARCHINFILESDIALOG_H

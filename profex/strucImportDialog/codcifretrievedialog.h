/***************************************************************************
                          codcifretrievedialog.h  -  description
                             -------------------
    begin                : Thu Jan 11 19:42:15 CEST 2021
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

#ifndef CODCIFRETRIEVEDIALOG_H
#define CODCIFRETRIEVEDIALOG_H

#include <QDialog>
#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/coddbmanager.h"

namespace Ui {
class CodCifRetrieveDialog;
}

enum DbMode {LOCAL_SQLITE, ONLINE_MYSQL};

class CodCifRetrieveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CodCifRetrieveDialog(QWidget *parent = 0);
    ~CodCifRetrieveDialog();

    QStringList getRecords();

private:
    Ui::CodCifRetrieveDialog *ui;
    SettingsManager *settings;
    CodDbManager *codManager;
    QString localDbFile;
    DbMode databaseMode;

    void initSettings();
    void saveSettings();
    QString queryString();
    void clearTable(bool all = false);
    void setDbStatusLabel(bool);
    void checkCodConnection();

public slots:
    void accept();

private slots:
    void queryDatabase();
    void checkAll();
    void checkNone();
    void saveQueries();
    void clearForm();
};

#endif // CODCIFRETRIEVEDIALOG_H

/***************************************************************************
                          preferencesdialog.h  -  description
                             -------------------
    begin                : Thu Apr 14 18:00:00 CEST 2011
    copyright            : (C) 2011 by Nicola Doebelin
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

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "../libXrdIO/settingsmanager.h"
#include <QDialog>
#include <QFont>
#include <QStringList>
#include <QListWidgetItem>
#include <QTreeWidgetItem>
#include <QMap>
#include <QVariant>
#include <QElapsedTimer>

namespace Ui {
    class PreferencesDialog;
}

class PreferencesDialog : public QDialog {
    Q_OBJECT
public:
    PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

    void raisePage(const QString &);
    void updatePreferences();

private:
    Ui::PreferencesDialog *ui;
    SettingsManager *settings;
    bool doProfile;
    QElapsedTimer pageProfileTimer;

    QString workingDir;

    void changeEvent(QEvent *e);
    void setupPageList();
    void initSettings();
    void saveSettings();
    void savePageSettings();


private slots:
    void accept();
    void applyPreferences();
    void pageChanged(QTreeWidgetItem *, int);
    void pageChanged(QTreeWidgetItem *, QTreeWidgetItem *);

signals:
    void preferencesApplied();
};

#endif // PREFERENCESDIALOG_H

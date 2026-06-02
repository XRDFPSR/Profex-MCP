/***************************************************************************
                          bgmnappenddialog.h  -  description
                             -------------------
    begin                : Mon Jan 30 11:00:00 CEST 2012
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

#ifndef BGMNADDREMOVEDIALOG_H
#define BGMNADDREMOVEDIALOG_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QDialog>
#include <QMap>
#include <QDir>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QAction>
#include "../libXrdIO/settingsmanager.h"
#include "bgmnrefstructuremanager.h"

namespace Ui {
    class AddRemovePhaseDialog;
}

class BgmnAddRemoveDialog : public QDialog
{
    Q_OBJECT

public:
    BgmnAddRemoveDialog(QWidget *parent = 0);
    ~BgmnAddRemoveDialog();

    void initSettings();

    bool createDefaultControlFile();
    bool overwriteFiles();
    QString deviceFile();
    QStringList getCheckedAddFiles();
    void setSelectedPhases(const QStringList &);

    void setDeleteStrFiles(const QStringList &);
    QStringList getCheckedDeleteFiles();
    bool deleteStrFiles();

    void readDirectories();
    void setDeviceFile(bool, const QString &);
    void updateDialog(const QString &device, const QStringList &preselectStr, const QStringList &projStr);

public slots:
    int exec();
    void accept();

private:
    Ui::AddRemovePhaseDialog *ui;
    SettingsManager *settings;
    BgmnRefStructureManager *refStrManager;
    Qt::SortOrder sortOrder;
    int sortCol;

    QStringList devExt;
    QStringList strExt;
    bool expandState;
    bool wasShown;
    QMenu *filterOptionsMenu;
    QMenu *contextMenuHeader;
    QAction *actionFilterRegExp;
    QAction *actionFilterCaseSensitive;
    QAction *actionFilterFile;
    QAction *actionFilterPhase;
    QAction *actionFilterComment;

    void initGui();
    void saveSettings();
    void setStrDir();
    void setDevDir();
    QList<QTreeWidgetItem *> allStructureItems();
    QString itemText(const QTreeWidgetItem *, int);
    void unhideAllItems();

private slots:
    void toggleExpand();
    void toggleFavorites(bool);
    void applyFilter(QString);
    void updateFilter();
    void customMenuRequested(QPoint);
    void toggleColumnHidden(QAction *);
    void clearFavsLabel();
    void generateDefaultControlFileToggled(bool);
    void instrumentConfigChanged(int);
};

#endif // BGMNAPPENDDIALOG_H

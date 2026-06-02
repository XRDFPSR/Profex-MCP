/***************************************************************************
                          browsereferencestructuresdialog.h  -  description
                             -------------------
    begin                : Tue 16 21:00:00 CEST 2021
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

#ifndef BROWSEREFERENCESTRUCTURESDIALOG_H
#define BROWSEREFERENCESTRUCTURESDIALOG_H

#include <QDialog>
#include "../../projectWidget/bgmnrefstructuremanager.h"
#include "../../../libXrdIO/settingsmanager.h"

namespace Ui {
class BrowseReferenceStructuresDialog;
}

class BrowseReferenceStructuresDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BrowseReferenceStructuresDialog(QWidget *parent = nullptr);
    ~BrowseReferenceStructuresDialog();

private:
    Ui::BrowseReferenceStructuresDialog *ui;
    SettingsManager *settings;
    BgmnRefStructureManager *refStrManager;
    Qt::SortOrder sortOrder;
    int sortCol;
    bool itemsExpanded;
    QMenu *contextMenuHeader;
    QString currentFile;

    void showEvent(QShowEvent *);
    void closeEvent(QCloseEvent *);
    void initUi();
    void initSettings();
    void saveSettings();
    void setStrDir();

    QVector<double> hklToTwoTheta(const QVector<Hkl> &vec, double wl);
    QVector<double> hklToIntensity(const QVector<Hkl> &vec, double &maxI);
    void applyFilter(const QString &);
    void unhideAllItems();
    QString itemText(const QTreeWidgetItem *it, int n);

    double plotHkl(const QVector<Hkl> &);
    void updateTable(const QVector<Hkl> &, double);
    void clearDataWidgets();
    void updateDataWidgets();
    void updateStrEditor();

private slots:
    void currentChanged(QTreeWidgetItem*,QTreeWidgetItem*);
    void customMenuRequested(QPoint);
    void toggleColumnHidden(QAction *);
    void updateFilter();
    void wavelengthChanged(int);
    void setFavorites();
    void saveFavorites();
    void savePeakData();
    void saveStickPattern();
    void toggleExpand();

signals:
    void favoritesChanged();
};

#endif // BROWSEREFERENCESTRUCTURESDIALOG_H

/***************************************************************************
                          prefpagebgmndirectories.cpp  -  description
                             -------------------
    begin                : Sun Aug 06 09:40:00 CEST 2017
    copyright            : (C) 2017 by Nicola Doebelin
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

#include "prefpagebgmndirectories.h"
#include "ui_prefpagebgmndirectories.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QTreeWidgetItem>
#include <QDir>

PrefPageBgmnDirectories::PrefPageBgmnDirectories(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageBgmnDirectories)
{
    ui->setupUi(this);
}

PrefPageBgmnDirectories::~PrefPageBgmnDirectories()
{
    delete ui;
}

void PrefPageBgmnDirectories::initUi()
{
    connect(ui->toolButtonBgmnStrFilesAdd,    SIGNAL(clicked(bool)), this, SLOT(addBgmnStrDir()));
    connect(ui->toolButtonBgmnStrFilesRemove, SIGNAL(clicked(bool)), this, SLOT(removeBgmnStrDir()));
    connect(ui->toolButtonBgmnDevFilesAdd,    SIGNAL(clicked(bool)), this, SLOT(addBgmnDevDir()));
    connect(ui->toolButtonBgmnDevFilesRemove, SIGNAL(clicked(bool)), this, SLOT(removeBgmnDevDir()));
    connect(ui->toolButtonBgmnPresetsAdd,     SIGNAL(clicked(bool)), this, SLOT(addBgmnPresetDir()));
    connect(ui->toolButtonBgmnPresetsRemove,  SIGNAL(clicked(bool)), this, SLOT(removeBgmnPresetDir()));
    connect(ui->treeWidgetBgmnStrFiles,       SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(indexingRequired()));
    initSettings();
}

void PrefPageBgmnDirectories::initSettings()
{
    strReposChanged = false;
    ui->treeWidgetBgmnStrFiles->clear();
    ui->treeWidgetBgmnDevFiles->clear();
    ui->treeWidgetBgmnPresets->clear();

    addBgmnStrDirs(settings->value("bgmnProject/structureDatabaseIndexed", QStringList()).toStringList(),
                   settings->value("bgmnProject/structureDatabaseNonIndexed", QStringList()).toStringList());
    addBgmnDevDirs(settings->value("bgmnProject/deviceDatabase", QStringList()).toStringList());
    addBgmnPresetDirs(settings->value("bgmnProject/presetDirectory", QStringList()).toStringList());

    ui->treeWidgetBgmnStrFiles->header()->restoreState(settings->value("preferencesDialog/repositoriesStructureHeaderState", QByteArray()).toByteArray());
    ui->treeWidgetBgmnDevFiles->header()->restoreState(settings->value("preferencesDialog/repositoriesDeviceHeaderState", QByteArray()).toByteArray());
    ui->treeWidgetBgmnPresets->header()->restoreState(settings->value("preferencesDialog/repositoriesPresetHeaderState", QByteArray()).toByteArray());
}

void PrefPageBgmnDirectories::saveSettings()
{
    QStringList strFileDirsIndexed;
    QStringList strFileDirsNonIndexed;
    QStringList devFileDirs;
    QStringList presFileDirs;

    for (int i = 0; i < ui->treeWidgetBgmnStrFiles->topLevelItemCount(); ++i) {
        if (ui->treeWidgetBgmnStrFiles->topLevelItem(i)->checkState(0) == Qt::Checked) {
            strFileDirsIndexed.append(ui->treeWidgetBgmnStrFiles->topLevelItem(i)->text(1));
        } else {
            strFileDirsNonIndexed.append(ui->treeWidgetBgmnStrFiles->topLevelItem(i)->text(1));
        }
    }

    for (int i = 0; i < ui->treeWidgetBgmnDevFiles->topLevelItemCount(); ++i) {
        devFileDirs.append(ui->treeWidgetBgmnDevFiles->topLevelItem(i)->text(0));
    }

    for (int i = 0; i < ui->treeWidgetBgmnPresets->topLevelItemCount(); ++i) {
        presFileDirs.append(ui->treeWidgetBgmnPresets->topLevelItem(i)->text(0));
    }

    settings->setValue("bgmnProject/structureDatabaseIndexed", strFileDirsIndexed);
    settings->setValue("bgmnProject/structureDatabaseNonIndexed", strFileDirsNonIndexed);
    settings->setValue("bgmnProject/deviceDatabase", devFileDirs);
    settings->setValue("bgmnProject/presetDirectory", presFileDirs);

    settings->setValue("preferencesDialog/repositoriesStructureHeaderState", ui->treeWidgetBgmnStrFiles->header()->saveState());
    settings->setValue("preferencesDialog/repositoriesDeviceHeaderState", ui->treeWidgetBgmnDevFiles->header()->saveState());
    settings->setValue("preferencesDialog/repositoriesPresetHeaderState", ui->treeWidgetBgmnPresets->header()->saveState());

    settings->setValue("bgmnProject/strIndexingRequired", strReposChanged);
}

void PrefPageBgmnDirectories::addBgmnStrDir()
{
    if (lastDir.isEmpty()) {
        lastDir = QDir::homePath();
    } else {
        QDir d(lastDir);
        d.cdUp();
        lastDir = d.absolutePath();
    }

    lastDir = QFileDialog::getExistingDirectory(this, tr("Select structure file directory"), lastDir);

    if (!lastDir.isEmpty()) {
        addBgmnStrDirs(QStringList(lastDir), QStringList());
        strReposChanged = true;
    }
}

void PrefPageBgmnDirectories::addBgmnDevDir()
{
    if (lastDir.isEmpty()) {
        lastDir = QDir::homePath();
    } else {
        QDir d(lastDir);
        d.cdUp();
        lastDir = d.absolutePath();
    }

    lastDir = QFileDialog::getExistingDirectory(this, tr("Select device file directory"), lastDir);

    if (!lastDir.isEmpty()) {
        addBgmnDevDirs(QStringList(lastDir));
    }
}

void PrefPageBgmnDirectories::addBgmnPresetDir()
{
    if (lastDir.isEmpty()) {
        lastDir = QDir::homePath();
    } else {
        QDir d(lastDir);
        d.cdUp();
        lastDir = d.absolutePath();
    }

    lastDir = QFileDialog::getExistingDirectory(this, tr("Select preset directory"), lastDir);

    if (!lastDir.isEmpty()) {
        addBgmnPresetDirs(QStringList(lastDir));
    }
}

void PrefPageBgmnDirectories::removeBgmnStrDir()
{
    int idx = ui->treeWidgetBgmnStrFiles->indexOfTopLevelItem(ui->treeWidgetBgmnStrFiles->currentItem());
    QTreeWidgetItem *it = ui->treeWidgetBgmnStrFiles->takeTopLevelItem(idx);
    if (it) delete it;
    strReposChanged = true;
}

void PrefPageBgmnDirectories::removeBgmnDevDir()
{
    int idx = ui->treeWidgetBgmnDevFiles->indexOfTopLevelItem(ui->treeWidgetBgmnDevFiles->currentItem());
    QTreeWidgetItem *it = ui->treeWidgetBgmnDevFiles->takeTopLevelItem(idx);
    if (it) delete it;
}

void PrefPageBgmnDirectories::removeBgmnPresetDir()
{
    int idx = ui->treeWidgetBgmnPresets->indexOfTopLevelItem(ui->treeWidgetBgmnPresets->currentItem());
    QTreeWidgetItem *it = ui->treeWidgetBgmnPresets->takeTopLevelItem(idx);
    if (it) delete it;
}

void PrefPageBgmnDirectories::addBgmnStrDirs(const QStringList &idx, const QStringList &nidx)
{
    bool oldState = ui->treeWidgetBgmnStrFiles->blockSignals(true);

    for (int i = 0; i < idx.size(); ++i) {
        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetBgmnStrFiles, QStringList() << QString() << idx.at(i), 0);
        it->setFlags(it->flags () | Qt::ItemIsEditable);
        it->setCheckState(0, Qt::Checked);
        ui->treeWidgetBgmnStrFiles->addTopLevelItem(it);
    }

    for (int i = 0; i < nidx.size(); ++i) {
        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetBgmnStrFiles, QStringList() << QString() << nidx.at(i), 0);
        it->setFlags(it->flags () | Qt::ItemIsEditable);
        it->setCheckState(0, Qt::Unchecked);
        ui->treeWidgetBgmnStrFiles->addTopLevelItem(it);
    }

    ui->treeWidgetBgmnStrFiles->blockSignals(oldState);
}

void PrefPageBgmnDirectories::addBgmnDevDirs(const QStringList &l)
{
    for (int i = 0; i < l.size(); ++i) {
        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetBgmnDevFiles, QStringList() << l.at(i), 0);
        it->setFlags(it->flags () | Qt::ItemIsEditable);
        ui->treeWidgetBgmnDevFiles->addTopLevelItem(it);
    }
}

void PrefPageBgmnDirectories::addBgmnPresetDirs(const QStringList &l)
{
    for (int i = 0; i < l.size(); ++i) {
        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetBgmnPresets, QStringList() << l.at(i), 0);
        it->setFlags(it->flags () | Qt::ItemIsEditable);
        ui->treeWidgetBgmnPresets->addTopLevelItem(it);
    }
}

void PrefPageBgmnDirectories::indexingRequired()
{
    strReposChanged = true;
}

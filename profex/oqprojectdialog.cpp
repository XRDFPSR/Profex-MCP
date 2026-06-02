/***************************************************************************
                          OqProjectDialog.cpp  -  description
                             -------------------
    begin                : Thu Mar 05 10:49:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#include "oqprojectdialog.h"
#include "ui_oqprojectdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>

OqProjectDialog::OqProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OqProjectDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();

    ui->stackedWidgetText->setCurrentIndex(0);
    ui->stackedWidgetPreview->setCurrentIndex(0);

    connect(ui->pushButtonIntroNext,      SIGNAL(clicked()), this, SLOT(introNext()));
    connect(ui->pushButtonCreatePrev,     SIGNAL(clicked()), this, SLOT(createPrev()));
    connect(ui->pushButtonCreateNext,     SIGNAL(clicked()), this, SLOT(createNext()));
    connect(ui->pushButtonRefStrPrev,     SIGNAL(clicked()), this, SLOT(refStrPrev()));
    connect(ui->pushButtonRefStrNext,     SIGNAL(clicked()), this, SLOT(refStrNext()));
    connect(ui->pushButtonPresetsPrev,    SIGNAL(clicked()), this, SLOT(presetPrev()));
    connect(ui->pushButtonPresetsNext,    SIGNAL(clicked()), this, SLOT(presetNext()));
    connect(ui->pushButtonRefinementPrev, SIGNAL(clicked()), this, SLOT(refinementPrev()));
    connect(ui->pushButtonRefinementNext, SIGNAL(clicked()), this, SLOT(refinementNext()));
    connect(ui->pushButtonFinishedPrev,   SIGNAL(clicked()), this, SLOT(finishedPrev()));

    initSettings();
}

OqProjectDialog::~OqProjectDialog()
{
    delete ui;
}

void OqProjectDialog::closeEvent(QCloseEvent *e)
{
    saveSettings();
    e->accept();
}

void OqProjectDialog::initSettings()
{
    restoreGeometry(settings->value("OqProjectDialog/geometry", QByteArray()).toByteArray());

    QByteArray ba = settings->value("OqProjectDialog/splitter", QByteArray()).toByteArray();

    if (ba.isEmpty()) {
        ui->splitter->setStretchFactor(0, 1);
        ui->splitter->setStretchFactor(1, 10);
    } else {
        ui->splitter->restoreState(ba);
    }
}

void OqProjectDialog::saveSettings()
{
    settings->setValue("OqProjectDialog/geometry", saveGeometry());
    settings->setValue("OqProjectDialog/splitter", ui->splitter->saveState());
}

void OqProjectDialog::createOqProject()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Create OQ project"), QDir::homePath());
    if (dir.isEmpty()) return;

    QString scanFile, filter, errors;
    OqProjectHandler ihandler;

    bool ok = ihandler.createEmptyProject("lab6", dir, scanFile, filter, errors);

    if (ok) {
        emit sigOpenProject(QStringList(scanFile), filter);
    } else {
        QMessageBox::critical(this, "OQ Project", errors);
    }
}

void OqProjectDialog::introNext()
{
    int n = ui->stackedWidgetText->currentIndex();
    ui->stackedWidgetText->setCurrentIndex(n+1);
    ui->stackedWidgetPreview->setCurrentIndex(n+1);
}

void OqProjectDialog::createPrev()
{
    int n = ui->stackedWidgetText->currentIndex();
    ui->stackedWidgetText->setCurrentIndex(n-1);
    ui->stackedWidgetPreview->setCurrentIndex(n-1);
}

void OqProjectDialog::createNext()
{
    int n = ui->stackedWidgetText->currentIndex();
    ui->stackedWidgetText->setCurrentIndex(n+1);
    ui->stackedWidgetPreview->setCurrentIndex(n+1);
}

void OqProjectDialog::refStrPrev()
{
    int n = ui->stackedWidgetText->currentIndex();
    ui->stackedWidgetText->setCurrentIndex(n-1);
    ui->stackedWidgetPreview->setCurrentIndex(n-1);
}

void OqProjectDialog::refStrNext()
{
    int n = ui->stackedWidgetText->currentIndex();
    ui->stackedWidgetText->setCurrentIndex(n+1);
    ui->stackedWidgetPreview->setCurrentIndex(n+1);
}

void OqProjectDialog::presetPrev()
{
    int n = ui->stackedWidgetText->currentIndex();
    ui->stackedWidgetText->setCurrentIndex(n-1);
    ui->stackedWidgetPreview->setCurrentIndex(n-1);
}

void OqProjectDialog::presetNext()
{
    int n = ui->stackedWidgetText->currentIndex();
    ui->stackedWidgetText->setCurrentIndex(n+1);
    ui->stackedWidgetPreview->setCurrentIndex(n+1);
}

void OqProjectDialog::refinementPrev()
{
    int n = ui->stackedWidgetText->currentIndex();
    ui->stackedWidgetText->setCurrentIndex(n-1);
    ui->stackedWidgetPreview->setCurrentIndex(n-1);
}

void OqProjectDialog::refinementNext()
{
    int n = ui->stackedWidgetText->currentIndex();
    ui->stackedWidgetText->setCurrentIndex(n+1);
    ui->stackedWidgetPreview->setCurrentIndex(n+1);
}

void OqProjectDialog::finishedPrev()
{
    int n = ui->stackedWidgetText->currentIndex();
    ui->stackedWidgetText->setCurrentIndex(n-1);
    ui->stackedWidgetPreview->setCurrentIndex(n-1);
}

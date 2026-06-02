/***************************************************************************
                          prefpagegeneral.cpp  -  description
                             -------------------
    begin                : Tue May 09 16:00:00 CEST 2017
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

#include <QThread>
#include <QListWidget>
#include <QListWidgetItem>
#include "prefpagegeneral.h"
#include "ui_prefpagegeneral.h"

PrefPageGeneral::PrefPageGeneral(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageGeneral)
{
    ui->setupUi(this);
}

PrefPageGeneral::~PrefPageGeneral()
{
    delete ui;
}

void PrefPageGeneral::initUi()
{
    ui->comboBoxToolbarLayout->addItem(tr("Icons only"), QVariant("ToolButtonIconOnly"));
    ui->comboBoxToolbarLayout->addItem(tr("Text only"), QVariant("ToolButtonTextOnly"));
    ui->comboBoxToolbarLayout->addItem(tr("Text beside Icons"), QVariant("ToolButtonTextBesideIcon"));
    ui->comboBoxToolbarLayout->addItem(tr("Text under Icons"), QVariant("ToolButtonTextUnderIcon"));
    ui->comboBoxToolbarLayout->addItem(tr("Follow Style"), QVariant("ToolButtonFollowStyle"));

    // removed in v5.2.2
    // ui->comboBoxIconTheme->addItem(tr("Light"), QVariant("profex-light"));
    // ui->comboBoxIconTheme->addItem(tr("Light color"), QVariant("profex-light-colored"));
    // ui->comboBoxIconTheme->addItem(tr("Dark"), QVariant("profex-dark"));

    ui->comboBoxDefaultProjectType->addItem(tr("FullProf"));
    ui->comboBoxDefaultProjectType->addItem(tr("BGMN"));

    ui->comboBoxNThreads->addItem("Automatic");
    ui->comboBoxNThreads->setItemData(0, 0);
    for (int i = 1; i <= QThread::idealThreadCount(); ++i) {
        ui->comboBoxNThreads->addItem(QString("%1").arg(i));
        ui->comboBoxNThreads->setItemData(i, i);
    }

    ui->comboBoxDebugVerbose->addItem("sparse", QVariant(1));
    ui->comboBoxDebugVerbose->addItem("medium", QVariant(2));
    ui->comboBoxDebugVerbose->addItem("verbose", QVariant(3));

    // removed in v5.2.2
    // ui->comboBoxGuiTheme->addItem("Automatic");
    // ui->comboBoxGuiTheme->addItem("Light");
    // ui->comboBoxGuiTheme->addItem("Dark");

    initSettings();
}


void PrefPageGeneral::initSettings()
{
    QStringList fpFext;
    QStringList bgmnFext;

    fpFext << "prf" << "pcr" << "sum";
    bgmnFext << "dia" << "sav" << "lst" << "par" << "val";

    ui->doubleSpinBoxDefaultWl->setValue(settings->defaultWavelength());
    ui->checkBoxDefaultWl->setChecked(settings->value("config/useDefaultWl", false).toBool());

    int pType = settings->value("config/defaultProjectType", 1).toInt();

    switch (pType) {
        case 0: ui->comboBoxDefaultProjectType->setCurrentIndex(0);
                break;
        case 1: ui->comboBoxDefaultProjectType->setCurrentIndex(1);
                break;
    }

    ui->checkBoxRestoreOpenProjects->setChecked(settings->value("config/restoreProjects", false).toBool());
    ui->comboBoxToolbarLayout->setCurrentIndex(settings->value("window/toolButtonLayout", Qt::ToolButtonIconOnly).toInt());
    ui->lineEditFpFext->setText(settings->value("fpProject/fileExtensions", fpFext).toStringList().join(" "));
    ui->lineEditBgmnFext->setText(settings->value("bgmnProject/fileExtensions", bgmnFext).toStringList().join(" "));
    ui->comboBoxNThreads->setCurrentIndex(settings->value("bgmnProject/nThreads", 0).toInt());
    ui->spinBoxParallelBatch->setValue(settings->value("config/batchParallelJobs", 1).toInt());

    /* removed in v5.2.2
    ui->comboBoxGuiTheme->setCurrentIndex(settings->value("config/guiTheme", -1).toInt() + 1);

    QString iconMode = settings->value("config/iconTheme", QString()).toString();

    if (iconMode.isEmpty()) {
        if (settings->isDarkMode()) {
            // assuming dark gui mode
            iconMode = QString("profex-dark");
        } else {
            // assuming light gui mode
            iconMode = QString("profex-light-colored");
        }
    }

    if (iconMode == "profex-light")         ui->comboBoxIconTheme->setCurrentIndex(0);
    if (iconMode == "profex-light-colored") ui->comboBoxIconTheme->setCurrentIndex(1);
    if (iconMode == "profex-dark")          ui->comboBoxIconTheme->setCurrentIndex(2);
    */

    ui->comboBoxDebugVerbose->setCurrentIndex(settings->value("config/debugVerboseLevel", 3).toInt() - 1);
}

void PrefPageGeneral::saveSettings()
{
    settings->setValue("config/defaultProjectType", ui->comboBoxDefaultProjectType->currentIndex());
    settings->setValue("config/defaultWl", ui->doubleSpinBoxDefaultWl->value());
    settings->setValue("config/useDefaultWl", ui->checkBoxDefaultWl->isChecked());
    settings->setValue("config/restoreProjects", ui->checkBoxRestoreOpenProjects->isChecked());
    settings->setValue("window/toolButtonLayout", ui->comboBoxToolbarLayout->currentIndex());
    settings->setValue("fpProject/fileExtensions", ui->lineEditFpFext->text().split(QRegularExpression("[\\s;,]+")));
    settings->setValue("bgmnProject/fileExtensions", ui->lineEditBgmnFext->text().split(QRegularExpression("[\\s;,]+")));
    settings->setValue("bgmnProject/nThreads", ui->comboBoxNThreads->itemData(ui->comboBoxNThreads->currentIndex()).toInt());
    settings->setValue("config/batchParallelJobs", ui->spinBoxParallelBatch->value());
    settings->setValue("config/debugVerboseLevel", ui->comboBoxDebugVerbose->currentData().toInt());

    // removed in v5.2.2
    // settings->setValue("config/guiTheme", ui->comboBoxGuiTheme->currentIndex() - 1);
    // settings->setValue("config/iconTheme", ui->comboBoxIconTheme->currentData(Qt::UserRole));
}

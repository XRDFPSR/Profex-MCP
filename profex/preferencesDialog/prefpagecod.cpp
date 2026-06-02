/***************************************************************************
                          prefpagecod.cpp  -  description
                             -------------------
    begin                : Wed Apr 05 19:00:00 CEST 2023
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

#include "prefpagecod.h"
#include "ui_prefpagecod.h"

#include <QFile>
#include <QtSql>
#include <QFileDialog>

PrefPageCod::PrefPageCod(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageCod)
{
    ui->setupUi(this);
    codManager = CodDbManager::getInstance();
}

PrefPageCod::~PrefPageCod()
{
    delete ui;
}

void PrefPageCod::initUi()
{
    ui->toolButtonSelectDbFile->setIcon(QIcon::fromTheme("profex-folder"));

    initSettings();

    if (ui->checkBoxLocalCod->isChecked() && codManager->isConnected()) {
        ui->plainTextEditDbMessages->setPlainText(codManager->probeDb());
    }

    connect(ui->checkBoxLocalCod, SIGNAL(toggled(bool)), ui->lineEditCifDir, SLOT(setEnabled(bool)));
    connect(ui->checkBoxLocalCod, SIGNAL(toggled(bool)), ui->toolButtonSelectDbFile, SLOT(setEnabled(bool)));
    connect(ui->checkBoxLocalCod, SIGNAL(toggled(bool)), ui->plainTextEditDbMessages, SLOT(setEnabled(bool)));
    connect(ui->checkBoxLocalCod, SIGNAL(toggled(bool)), this, SLOT(toggleUseDb(bool)));
}

void PrefPageCod::initSettings()
{
    // no need to initialize the codManager here. It should already be connected to the
    // correct DB at this point.

    bool useDb = settings->value("config/useLocalCodDb", false).toBool();
    ui->checkBoxLocalCod->setChecked(useDb);
    ui->lineEditDbFile->setEnabled(useDb);
    ui->plainTextEditDbMessages->setEnabled(useDb);

    bool onlineCif = settings->value("config/useOnlineCifDir", true).toBool();
    ui->lineEditCifDir->setText(settings->value("config/localCodCifDir", QString()).toString());
    ui->radioButtonCifServer->setChecked(onlineCif);
    ui->radioButtonCifDir->setChecked(!onlineCif);

    ui->lineEditCifDir->setEnabled(!onlineCif);
    ui->toolButtonSelectCifDir->setEnabled(!onlineCif);

    // deprecated setting name used for fallback
    QString oldDbSetting = settings->value("codQueryDialog/localDbFile", QString()).toString();
    ui->lineEditDbFile->setText(settings->value("config/localCodDbFile", oldDbSetting).toString());
}

void PrefPageCod::saveSettings()
{
    bool useDb = ui->checkBoxLocalCod->isChecked();
    bool onlineCif = ui->radioButtonCifServer->isChecked();

    settings->setValue("config/useLocalCodDb", useDb);
    settings->setValue("config/localCodDbFile", useDb ? ui->lineEditDbFile->text() : QString());

    settings->setValue("config/useOnlineCifDir", onlineCif);
    settings->setValue("config/localCodCifDir", onlineCif ? QString() : ui->lineEditCifDir->text());
}

void PrefPageCod::selectDatabase()
{
    QFileInfo fi(ui->lineEditDbFile->text());

    QString fn = QFileDialog::getOpenFileName(this, tr("Select SQLITE3 database file"),
                                              fi.absolutePath(), tr("SQLITE3 file (*.db3 *.DB3)"));

    if (fn.isEmpty()) return;

    ui->lineEditDbFile->setText(fn);
    ui->checkBoxLocalCod->setChecked(true);
}

void PrefPageCod::toggleUseDb(bool b)
{
    if (b) {
        openDb(ui->lineEditDbFile->text());
    } else {
        codManager->shutdown();
    }
}

bool PrefPageCod::openDb(const QString &f)
{
    QString err;
    bool ok = codManager->init(f, &err);

    if (ok) {
        ui->plainTextEditDbMessages->setPlainText(codManager->probeDb());
    } else {
        ui->plainTextEditDbMessages->setPlainText(err);
    }

    return ok;
}

void PrefPageCod::selectCifDir()
{
    QString cd = QFileDialog::getExistingDirectory(this, tr("Select CIF file directory"), ui->lineEditCifDir->text());

    if (cd.isEmpty()) return;

    ui->lineEditCifDir->setText(cd);
}

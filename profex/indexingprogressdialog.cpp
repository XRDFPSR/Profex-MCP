/***************************************************************************
                          indexingprogressdialog.cpp  -  description
                             -------------------
    begin                : Fri Jan 07 18:30:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#include "indexingprogressdialog.h"
#include "ui_indexingprogressdialog.h"

IndexingProgressDialog::IndexingProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IndexingProgressDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();

    _skipTimer.setSingleShot(false);
    ui->labelStructure->clear();
    ui->labelTime->clear();
    ui->pushButtonSkip->setEnabled(false);
    ui->pushButtonStopTimer->setEnabled(false);

    connect(&_skipTimer, SIGNAL(timeout()), this, SLOT(skipTimeUpdate()));
}

IndexingProgressDialog::~IndexingProgressDialog()
{
    delete ui;
}

void IndexingProgressDialog::showEvent(QShowEvent *)
{
    _secKillTimer = settings->value("config/strucIndexingTimer", 30).toInt();

    if (_secKillTimer > 0) {
        ui->pushButtonStopTimer->show();
    } else {
        ui->pushButtonStopTimer->hide();
    }
}

void IndexingProgressDialog::setMaximum(int i)
{
    ui->progressBar->setMaximum(i);
}

void IndexingProgressDialog::setValue(int i)
{
    ui->progressBar->setValue(i);

    ui->labelTime->clear();
    ui->pushButtonSkip->setEnabled(false);
    ui->pushButtonStopTimer->setEnabled(false);
    _secsSinceStart = _secKillTimer;

    _skipTimer.start(1000);
}

void IndexingProgressDialog::setLabelText(QString s)
{
    ui->labelStructure->setText(s);
}

void IndexingProgressDialog::reset()
{
    _skipTimer.stop();
    ui->progressBar->reset();
    ui->labelStructure->clear();
    ui->labelTime->clear();
}

void IndexingProgressDialog::abort()
{
    emit canceled();
}

void IndexingProgressDialog::skip()
{
    emit skipped();
}

void IndexingProgressDialog::skipTimeUpdate()
{
    if (_secKillTimer == 0) {
        _secsSinceStart++;
        ui->labelTime->setText(QString("%1 sec").arg(_secsSinceStart));
        ui->pushButtonSkip->setEnabled(true);
    } else if (_secsSinceStart > 0) {
        _secsSinceStart--;
        ui->labelTime->setText(QString("Skipping in %1 sec").arg(_secsSinceStart + 1));
        ui->pushButtonSkip->setEnabled(true);
        ui->pushButtonStopTimer->setEnabled(true);
    } else {
        skip();
    }
}

void IndexingProgressDialog::stopTimer()
{
    _skipTimer.stop();
    ui->labelTime->clear();
}

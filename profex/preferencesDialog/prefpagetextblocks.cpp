/***************************************************************************
                          prefpagetextblocks.cpp  -  description
                             -------------------
    begin                : Wed May 10 10:00:00 CEST 2017
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

#include "prefpagetextblocks.h"
#include "../libXrdIO/structs.h"
#include "ui_prefpagetextblocks.h"

#include <QMessageBox>
#include <QInputDialog>

PrefPageTextBlocks::PrefPageTextBlocks(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageTextBlocks)
{
    ui->setupUi(this);
}

PrefPageTextBlocks::~PrefPageTextBlocks()
{
    delete ui;
}

void PrefPageTextBlocks::initUi()
{
    previousTextBlock = -1;
    ui->textEditTextBlocks->setEnabled(false);

    connect(ui->toolButtonAddTextBlock, SIGNAL(clicked(bool)), this, SLOT(textBlockAdd()));
    connect(ui->toolButtonRemoveTextBlock, SIGNAL(clicked(bool)), this, SLOT(textBlockRemove()));
    connect(ui->comboBoxTextBlocks, SIGNAL(currentIndexChanged(int)), this, SLOT(textBlockChanged(int)));
    initSettings();
}

void PrefPageTextBlocks::initSettings()
{
    QMap<QString, QVariant> textBlocksFallback;
    for (int i = 0; i < 2*global::defaultTextBlockNumber; i += 2) {
        textBlocksFallback[global::defaultTextBlocks[i]] = global::defaultTextBlocks[i+1];
    }

    initTextBlocks(settings->value("config/textBlocks", textBlocksFallback).toMap());
}

void PrefPageTextBlocks::saveSettings()
{
    saveTextBlockChanges();
    settings->setValue("config/textBlocks", getTextBlocks());
}


void PrefPageTextBlocks::textBlockRemove()
{
    if (!ui->comboBoxTextBlocks->count()) return;

    if (QMessageBox::question(this,
                              tr("Delete current text block"),
                              QString("Do you really want to delete text block:\n%1?").arg(ui->comboBoxTextBlocks->currentText())) == QMessageBox::No) {
        return;

    }

    bool oldState = ui->comboBoxTextBlocks->blockSignals(true);

    ui->comboBoxTextBlocks->removeItem(ui->comboBoxTextBlocks->currentIndex());
    ui->textEditTextBlocks->clear();

    if (!ui->comboBoxTextBlocks->count()) {
        ui->textEditTextBlocks->setEnabled(false);
    }

    if (ui->comboBoxTextBlocks->currentIndex() >= 0) {
        ui->textEditTextBlocks->setPlainText(ui->comboBoxTextBlocks->currentData(Qt::UserRole).toString());
    }

    previousTextBlock = ui->comboBoxTextBlocks->currentIndex();

    ui->comboBoxTextBlocks->blockSignals(oldState);
}

void PrefPageTextBlocks::textBlockChanged(int i)
{
    if (previousTextBlock < ui->comboBoxTextBlocks->count()) {
        ui->comboBoxTextBlocks->setItemData(previousTextBlock, QVariant(ui->textEditTextBlocks->toPlainText()), Qt::UserRole);
    }

    previousTextBlock = i;

    ui->textEditTextBlocks->setPlainText(ui->comboBoxTextBlocks->currentData(Qt::UserRole).toString());
}

void PrefPageTextBlocks::saveTextBlockChanges()
{
    if (ui->comboBoxTextBlocks->count()) {
        int i = ui->comboBoxTextBlocks->currentIndex();
        ui->comboBoxTextBlocks->setItemData(i, QVariant(ui->textEditTextBlocks->toPlainText()));
    }
}

void PrefPageTextBlocks::initTextBlocks(const QMap<QString, QVariant> &blocks)
{
    bool oldState = ui->comboBoxTextBlocks->blockSignals(true);

    ui->comboBoxTextBlocks->clear();
    ui->textEditTextBlocks->clear();

    QMap<QString, QVariant>::ConstIterator it = blocks.constBegin();

    while (it != blocks.constEnd()) {
        ui->comboBoxTextBlocks->addItem(it.key(), it.value());
        ++it;
    }

    ui->textEditTextBlocks->setPlainText(ui->comboBoxTextBlocks->currentData(Qt::UserRole).toString());
    ui->comboBoxTextBlocks->blockSignals(oldState);

    if (ui->comboBoxTextBlocks->count()) {
        previousTextBlock = 0;
        ui->textEditTextBlocks->setEnabled(true);
    }
}

QMap<QString, QVariant> PrefPageTextBlocks::getTextBlocks()
{
    QMap<QString, QVariant> blocks;

    for (int i = 0; i < ui->comboBoxTextBlocks->count(); ++i) {
        blocks.insert(ui->comboBoxTextBlocks->itemText(i), ui->comboBoxTextBlocks->itemData(i));
    }

    return blocks;
}

void PrefPageTextBlocks::textBlockAdd()
{
    QString name = QInputDialog::getText(this, tr("Text Block Name"), tr("Text Block Name"));
    if (name.isEmpty()) return;

    bool oldState = ui->comboBoxTextBlocks->blockSignals(true);

    saveTextBlockChanges();

    ui->textEditTextBlocks->clear();
    ui->comboBoxTextBlocks->addItem(name, QVariant());
    ui->comboBoxTextBlocks->setCurrentText(name);
    ui->textEditTextBlocks->setEnabled(true);

    previousTextBlock = ui->comboBoxTextBlocks->currentIndex();

    ui->comboBoxTextBlocks->blockSignals(oldState);
}

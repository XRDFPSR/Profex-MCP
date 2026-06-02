/***************************************************************************
                          inputspacegroupdialog.cpp  -  description
                             -------------------
    begin                : Tue Sept 01 20:30:00 CEST 2013
    copyright            : (C) 2013 by Nicola Doebelin
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

#include "inputspacegroupdialog.h"
#include "ui_inputspacegroupdialog.h"

#include <QPushButton>

InputSpacegroupDialog::InputSpacegroupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InputSpacegroupDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    ui->splitter->restoreState(settings->value("inputSpaceGroupDialog/splitter", QByteArray()).toByteArray());
    restoreGeometry(settings->value("inputSpaceGroupDialog/geometry", QByteArray()).toByteArray());
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

InputSpacegroupDialog::~InputSpacegroupDialog()
{
    settings->setValue("inputSpaceGroupDialog/splitter", ui->splitter->saveGeometry());
    settings->setValue("inputSpaceGroupDialog/geometry", saveGeometry());
    delete ui;
}

void InputSpacegroupDialog::setLabelText(const QString &s)
{
    ui->labelMessage->setText(s);
}

void InputSpacegroupDialog::setSpaceGroups(const QMap<int, QMap<int, BgmnSpaceGroup> > &m)
{
    sgMap = m;

    bool oldStateNum = ui->listWidgetNumber->blockSignals(true);
    bool oldStateSet = ui->listWidgetSettings->blockSignals(true);

    ui->listWidgetNumber->clear();
    ui->listWidgetSettings->clear();

    QMapIterator<int, QMap<int, BgmnSpaceGroup> > it(sgMap);
    while (it.hasNext()) {
        it.next();

        QListWidgetItem *numItem = new QListWidgetItem(QString("%1").arg(it.key()));
        numItem->setData(Qt::UserRole, it.key());
        ui->listWidgetNumber->addItem(numItem);
    }

    if (ui->listWidgetNumber->count()) {
        ui->listWidgetNumber->setCurrentItem(ui->listWidgetNumber->item(0));
        currentNumberChanged(ui->listWidgetNumber->item(0), NULL);
    }

    ui->listWidgetNumber->blockSignals(oldStateNum);
    ui->listWidgetSettings->blockSignals(oldStateSet);
}

int InputSpacegroupDialog::intTableNo() const
{
    if (!ui->listWidgetNumber->currentItem()) return 0;
    return ui->listWidgetNumber->currentItem()->data(Qt::UserRole).toInt();
}

int InputSpacegroupDialog::settingNo() const
{
    if (!ui->listWidgetSettings->currentItem()) return 0;
    return ui->listWidgetSettings->currentItem()->data(Qt::UserRole).toInt();
}

void InputSpacegroupDialog::currentNumberChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);

    ui->listWidgetSettings->clear();
    toggleOkButtonState();

    if (!current) return;

    QMapIterator<int, BgmnSpaceGroup> it(sgMap[current->data(Qt::UserRole).toInt()]);

    while (it.hasNext()) {
        it.next();

        QListWidgetItem *settingItem = new QListWidgetItem(it.value().HermannMauguin);
        settingItem->setData(Qt::UserRole, it.key());
        ui->listWidgetSettings->addItem(settingItem);
    }
}

void InputSpacegroupDialog::currentSettingChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);

    QListWidgetItem *itNum = ui->listWidgetNumber->currentItem();
    ui->lineEditSettingLine->clear();
    toggleOkButtonState();

    if (!current || !itNum) return;

    int num = itNum->data(Qt::UserRole).toInt();
    int set = current->data(Qt::UserRole).toInt();
    ui->lineEditSettingLine->setText(sgMap[num][set].settingLine);
}

void InputSpacegroupDialog::toggleOkButtonState()
{
    QListWidgetItem *itNum = ui->listWidgetNumber->currentItem();
    QListWidgetItem *itSet = ui->listWidgetSettings->currentItem();

    if (itNum && itSet) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->buttonBox->button(QDialogButtonBox::Cancel)->setFocus();
    }
}

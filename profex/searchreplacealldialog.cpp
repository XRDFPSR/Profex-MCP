/***************************************************************************
                          searchreplacealldialog.cpp  -  description
                             -------------------
    begin                : Mon Mar 28 10:30:00 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#include <QLineEdit>
#include "searchreplacealldialog.h"
#include "ui_searchreplacealldialog.h"

SearchReplaceAllDialog::SearchReplaceAllDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchReplaceAllDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();

    ui->comboBoxSearch->addItems(settings->value("config/recentSearchStrings", 0).toStringList());
    ui->comboBoxReplace->addItems(settings->value("config/recentReplaceStrings", 0).toStringList());

    ui->comboBoxSearch->insertItem(0, QString());
    ui->comboBoxReplace->insertItem(0, QString());

    ui->comboBoxSearch->setCurrentIndex(0);
    ui->comboBoxReplace->setCurrentIndex(0);
}

SearchReplaceAllDialog::~SearchReplaceAllDialog()
{
    delete ui;
}

void SearchReplaceAllDialog::accept()
{
    QStringList recentSearchStrings(ui->comboBoxSearch->lineEdit()->text().trimmed());
    QStringList recentReplaceStrings(ui->comboBoxReplace->lineEdit()->text().trimmed());

    for (int i = 1; i < qMin(ui->comboBoxSearch->count(), 5); ++i) {
        QString searchString = ui->comboBoxSearch->itemText(i).trimmed();
        if (!searchString.isEmpty()) recentSearchStrings.append(searchString);
    }

    for (int i = 1; i < qMin(ui->comboBoxReplace->count(), 5); ++i) {
        QString replaceString = ui->comboBoxReplace->itemText(i).trimmed();
        if (!replaceString.isEmpty()) recentReplaceStrings.append(replaceString);
    }

    recentSearchStrings.removeAll(QString());
    recentReplaceStrings.removeAll(QString());

    settings->setValue("config/recentSearchStrings", recentSearchStrings);
    settings->setValue("config/recentReplaceStrings", recentReplaceStrings);

    QDialog::accept();
}

QString SearchReplaceAllDialog::findString()
{
    return ui->comboBoxSearch->currentText();
}

QString SearchReplaceAllDialog::replaceString()
{
    return ui->comboBoxReplace->currentText();
}

bool SearchReplaceAllDialog::caseSensitive()
{
    return ui->checkBoxMatchCase->isChecked();
}

bool SearchReplaceAllDialog::wholeWords()
{
    return ui->checkBoxWholeWords->isChecked();
}

bool SearchReplaceAllDialog::isRegExp()
{
    return ui->checkBoxRegExp->isChecked();
}

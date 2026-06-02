/***************************************************************************
                          reportcustomsectiondialog.cpp  -  description
                             -------------------
    begin                : Jul 26 18:50:00 CET 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#include "reportcustomsectiondialog.h"
#include "../libXrdIO/bgmnfileio.h"

#include <QFileDialog>
#include "ui_reportcustomsectiondialog.h"

ReportCustomSectionDialog::ReportCustomSectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReportCustomSectionDialog)
{
    ui->setupUi(this);
}

ReportCustomSectionDialog::~ReportCustomSectionDialog()
{
    delete ui;
}

void ReportCustomSectionDialog::clearData()
{
    ui->lineEditName->clear();
    ui->plainTextEdit->clear();
}

void ReportCustomSectionDialog::setContent(const QString &name, const QString &text)
{
    ui->lineEditName->setText(name);
    ui->plainTextEdit->setPlainText(text);
}

QString ReportCustomSectionDialog::getName()
{
    return ui->lineEditName->text();
}

QString ReportCustomSectionDialog::getText()
{
    return ui->plainTextEdit->toPlainText();
}

void ReportCustomSectionDialog::loadFile()
{
    QString s = QFileDialog::getOpenFileName(this, tr("Select file"), QDir::homePath());
    if (!QFile::exists(s)) return;

    ui->plainTextEdit->setPlainText(BgmnFileIO::readTextFile(s));
}

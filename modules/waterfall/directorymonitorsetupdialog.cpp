/***************************************************************************
                          directorymonitorsetupdialog.cpp  -  description
                             -------------------
    begin                : Wed Aug 25 23:00:00 CEST 2021
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


#include "directorymonitorsetupdialog.h"
#include "ui_directorymonitorsetupdialog.h"
#include <QFileDialog>

DirectoryMonitorSetupDialog::DirectoryMonitorSetupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DirectoryMonitorSetupDialog)
{
    ui->setupUi(this);
}

DirectoryMonitorSetupDialog::~DirectoryMonitorSetupDialog()
{
    delete ui;
}

void DirectoryMonitorSetupDialog::selectDir()
{
    QDir d(ui->lineEditDir->text());
    if (!d.exists()) d = QDir(workingDir);

    QString s = QFileDialog::getExistingDirectory(this, tr("Open directory"), d.dirName());
    if (s.isEmpty()) return;

    workingDir = s;
    ui->lineEditDir->setText(workingDir);
}

void DirectoryMonitorSetupDialog::setDir(const QString &d)
{
    ui->lineEditDir->setText(d);
}

void DirectoryMonitorSetupDialog::setWorkingDir(const QString &w)
{
    workingDir = w;
}

void DirectoryMonitorSetupDialog::setFileFormats(const QStringList &f, const QList<QStringList> &e)
{
    ui->comboBoxFormat->clear();

    for (int i = 0; i < qMin(f.size(), e.size()); ++i) {
        ui->comboBoxFormat->addItem(f.at(i), e.at(i));
    }
}

void DirectoryMonitorSetupDialog::setCurrentFormat(int c)
{
    ui->comboBoxFormat->setCurrentIndex(c);
}

QString DirectoryMonitorSetupDialog::getDir()
{
    return ui->lineEditDir->text();
}

int DirectoryMonitorSetupDialog::getFormat()
{
    return ui->comboBoxFormat->currentIndex();
}

QStringList DirectoryMonitorSetupDialog::getCurrentFilter()
{
    return ui->comboBoxFormat->currentData(Qt::UserRole).toStringList();
}


/***************************************************************************
                          bgmnindexerrordialog.cpp  -  description
                             -------------------
    begin                : Tue May 07 16:20:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#include <QClipboard>
#include "bgmnindexingerrordialog.h"
#include "ui_bgmnindexingerrordialog.h"

BgmnIndexingErrorDialog::BgmnIndexingErrorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BgmnIndexingErrorDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    initSettings();
    ui->labelLabel->setText(tr("The following files could not be indexed:"));
}

BgmnIndexingErrorDialog::~BgmnIndexingErrorDialog()
{
    delete ui;
}

void BgmnIndexingErrorDialog::initSettings()
{
    this->restoreGeometry(settings->value("bgmnIndexingErrorDialog/geometry", QByteArray()).toByteArray());
    ui->splitter->restoreState(settings->value("bgmnIndexingErrorDialog/splitter", QByteArray()).toByteArray());
}

void BgmnIndexingErrorDialog::saveSettings()
{
    settings->setValue("bgmnIndexingErrorDialog/geometry", this->saveGeometry());
    settings->setValue("bgmnIndexingErrorDialog/splitter", ui->splitter->saveState());
}

void BgmnIndexingErrorDialog::close()
{
    saveSettings();
    QDialog::close();
}

void BgmnIndexingErrorDialog::setErrorString(const QStringList &l)
{
    if (l.size() < 2) return;

    for (int i = 0; i < l.size() - 1; i += 2) {
        QListWidgetItem *it = new QListWidgetItem(l.at(i));
        it->setData(Qt::UserRole, textToPlainText(l.at(i + 1)));
        ui->listWidgetFiles->addItem(it);
        ui->textEditErrorMessage->setPlainText(it->data(Qt::UserRole).toString());
    }
}

QString BgmnIndexingErrorDialog::textToPlainText(const QString &s)
{
    QRegularExpression rx("[\\s\\S]+no\\s+measuring\\s+values\\s+read\\s+([\\s\\S]+)");
    QRegularExpressionMatch rm = rx.match(s);

    return rm.captured(1);
}

void BgmnIndexingErrorDialog::setErrorLabel(const QString &s)
{
    ui->labelLabel->setText(s);
}

void BgmnIndexingErrorDialog::fileSelectionChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    ui->textEditErrorMessage->clear();
    ui->textEditErrorMessage->setPlainText(current->data(Qt::UserRole).toString());
}

void BgmnIndexingErrorDialog::copyToClipboard()
{
    QString str;

    for (int i = 0; i < ui->listWidgetFiles->count(); ++i) {
        QListWidgetItem *it = ui->listWidgetFiles->item(i);
        str += it->text() + ":\n\n";
        str += it->data(Qt::UserRole).toString() + "\n\n";
    }

    QClipboard *clip = QGuiApplication::clipboard();
    clip->setText(str);
}

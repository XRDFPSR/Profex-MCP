/***************************************************************************
                          searchinfilesdialog.cpp  -  description
                             -------------------
    begin                : Thu Jul 02 18:00:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "searchinfilesdialog.h"
#include "ui_searchinfilesdialog.h"
#include "../libXrdIO/bgmnfileio.h"
#include <QDebug>
#include <QDir>
#include <QFileDialog>

SearchInFilesDialog::SearchInFilesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchInFilesDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();

    exprLineEdit = new QLineEdit;
    ui->comboBoxRegExp->setLineEdit(exprLineEdit);

    ui->comboBoxProject->addItem(tr("Current"), QString("cur"));
    ui->comboBoxProject->addItem(tr("All"),     QString("all"));

    ui->comboBoxFileTypes->addItem(tr("Current file"),         QString("cur"));
    ui->comboBoxFileTypes->addItem(tr("All open files"),       QString("all"));
    ui->comboBoxFileTypes->addItem(tr("Open control files"),   QString("sav"));
    ui->comboBoxFileTypes->addItem(tr("Open structure files"), QString("str"));
    ui->comboBoxFileTypes->addItem(tr("Open list files"),      QString("lst"));

    ui->pushButtonClose->setAutoDefault(false);
    ui->pushButtonClose->setDefault(false);

    headerLabels << tr("Line") << tr("Match");

    matchModel.setColumnCount(2);
    matchModel.setHorizontalHeaderLabels(headerLabels);
    ui->treeViewMatches->setModel(&matchModel);

    initSettings();

    connect(ui->treeViewMatches, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(itemDoubleClicked(QModelIndex)));
    connect(exprLineEdit, SIGNAL(returnPressed()), this, SLOT(search()));
}

SearchInFilesDialog::~SearchInFilesDialog()
{
    delete ui;
}

void SearchInFilesDialog::closeEvent(QCloseEvent *)
{
    saveSettings();
}

void SearchInFilesDialog::initSettings()
{
    ui->treeViewMatches->header()->restoreState(settings->value("searchInFiles/header", QByteArray()).toByteArray());
    ui->comboBoxProject->setCurrentIndex(settings->value("searchInFiles/project", 0).toInt());
    ui->comboBoxFileTypes->setCurrentIndex(settings->value("searchInFiles/fileTypes", 0).toInt());

    QStringList history = settings->value("searchInFiles/history", QStringList()).toStringList();
    history.removeAll("");

    ui->comboBoxRegExp->addItems(history);
    ui->comboBoxRegExp->insertItem(0, QString());
    ui->comboBoxRegExp->setCurrentIndex(0);
}

void SearchInFilesDialog::saveSettings()
{
    settings->setValue("searchInFiles/header", ui->treeViewMatches->header()->saveState());
    settings->setValue("searchInFiles/project", ui->comboBoxProject->currentIndex());
    settings->setValue("searchInFiles/fileTypes", ui->comboBoxFileTypes->currentIndex());

    QStringList history;
    for (int i = 0; i < qMin(ui->comboBoxRegExp->count(), 10); ++i) {
        history.append(ui->comboBoxRegExp->itemText(i));
    }

    settings->setValue("searchInFiles/history", history);
}

void SearchInFilesDialog::search()
{
    QByteArray headerState = ui->treeViewMatches->header()->saveState();

    matchModel.clear();
    matchModel.setColumnCount(2);
    matchModel.setHorizontalHeaderLabels(headerLabels);

    ui->treeViewMatches->header()->restoreState(headerState);

    QString expression = ui->comboBoxRegExp->currentText();

    if (expression.isEmpty()) {
        return;
    } else {
        if (ui->comboBoxRegExp->itemText(0).isEmpty()) {
            ui->comboBoxRegExp->setItemText(0, expression);
        } else if (ui->comboBoxRegExp->itemText(0) != expression) {
            ui->comboBoxRegExp->insertItem(0, expression);
        }
    }

    emit runSearch();
}

QString SearchInFilesDialog::getProjectSelection()
{
    return ui->comboBoxProject->currentData(Qt::UserRole).toString();
}

QString SearchInFilesDialog::getFileTypeSelection()
{
    return ui->comboBoxFileTypes->currentData(Qt::UserRole).toString();
}

QString SearchInFilesDialog::getSearchExpression()
{
    return ui->comboBoxRegExp->currentText();
}

/*
 * structure of the function arguments:
 *
 * pname:               project name
 * uid:                 project uid
 * QString (key):       file name
 * int (key):           line
 * QStringList (value): capture ; entire line
 */
void SearchInFilesDialog::appendResults(const QString &pname, const QUuid &uid, const QMap<QString, QMap<int, QStringList> > &m)
{
    QStandardItem *projectItem = new QStandardItem(pname);
    projectItem->setData(uid);
    projectItem->setFlags(projectItem->flags() & (~Qt::ItemIsEditable));
    matchModel.invisibleRootItem()->appendRow(projectItem);
    ui->treeViewMatches->setFirstColumnSpanned(projectItem->row(), QModelIndex(), true);
    ui->treeViewMatches->setExpanded(matchModel.indexFromItem(projectItem), true);

    QMapIterator<QString, QMap<int, QStringList> > itFiles(m);

    while (itFiles.hasNext()) {
        itFiles.next();

        QStandardItem *fileItem = new QStandardItem(itFiles.key());
        projectItem->appendRow(fileItem);
        ui->treeViewMatches->setFirstColumnSpanned(fileItem->row(), matchModel.indexFromItem(projectItem), true);
        ui->treeViewMatches->setExpanded(matchModel.indexFromItem(fileItem), true);

        fileItem->setData(uid);
        fileItem->setFlags(fileItem->flags() & (~Qt::ItemIsEditable));

        QMapIterator<int, QStringList> itLines(itFiles.value());

        while (itLines.hasNext()) {
            itLines.next();

            QString mtch = itLines.value().at(0).isEmpty() ? itLines.value().at(1) : itLines.value().at(0);

            QStandardItem *matchItem = new QStandardItem(QString("Line %1:").arg(itLines.key()));
            QStandardItem *lineItem  = new QStandardItem(mtch);
            matchItem->setData(itLines.key());
            lineItem->setData(itLines.key());

            matchItem->setFlags(matchItem->flags() & (~Qt::ItemIsEditable));
            lineItem->setFlags(lineItem->flags() & (~Qt::ItemIsEditable));

            fileItem->appendRow(QList<QStandardItem*>() << matchItem << lineItem);
        }
    }
}

void SearchInFilesDialog::itemDoubleClicked(QModelIndex i)
{
    QStandardItem *it = matchModel.itemFromIndex(i);
    if (!it) return;

    QUuid uid;
    QString file;
    int line = 0;

    int level = 0;

    // attention: it->parent()->parent() causes a segfault if it->parent() doesn't exist
    if (it->parent()) {
        level++;
        if (it->parent()->parent()) {
            level++;
        }
    }


    if (level == 2) {
        uid = it->parent()->parent()->data().toUuid();
        file = it->parent()->text();
        line = it->data().toInt() - 1;
    } else if (level == 1) {
        uid = it->parent()->data().toUuid();
        file = it->text();
    } else {
        uid = it->data().toUuid();
    }

    emit projectFileSelected(uid, file, line);
}

void SearchInFilesDialog::exportResults()
{
    if (!matchModel.rowCount()) return;

    QString workingDir = settings->value("searchInFiles/workingDir", QDir::homePath()).toString();
    QString file = QFileDialog::getSaveFileName(this, tr("Export results"), workingDir, tr("CSV files (*.csv *.CSV)"));

    if (file.isEmpty()) return;

    BgmnFileIO::writeTextFile(file, getCsvString());

    settings->setValue("searchInFiles/workingDir", workingDir);
}

QString SearchInFilesDialog::getCsvString()
{
    QString out("Project;File;Line;Match\n");

    for (int p = 0; p < matchModel.rowCount(QModelIndex()); ++p) {
        QModelIndex projectIdx = matchModel.index(p, 0, QModelIndex());
        QString projectTxt = matchModel.itemFromIndex(projectIdx)->text();

        for (int f = 0; f < matchModel.rowCount(projectIdx); ++f) {
            QModelIndex fileIdx = matchModel.index(f, 0, projectIdx);
            QString fileTxt = matchModel.itemFromIndex(fileIdx)->text();

            for (int l = 0; l < matchModel.rowCount(fileIdx); ++l) {
                QModelIndex lineIdx = matchModel.index(l, 0, fileIdx);
                QModelIndex matchIdx = matchModel.index(l, 1, fileIdx);
                QString lineTxt = QString("%1").arg(matchModel.itemFromIndex(lineIdx)->data().toInt());
                QString matchTxt = matchModel.itemFromIndex(matchIdx)->text();

                out += QString("%1;%2;%3;%4\n").arg(projectTxt).arg(fileTxt).arg(lineTxt).arg(matchTxt);
            }
        }
    }

    return out;
}

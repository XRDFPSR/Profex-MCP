/***************************************************************************
                          exportgraphdialog.cpp  -  description
                             -------------------
    begin                : Sun Jun 02 11:00:00 CEST 2013
    copyright            : (C) 2012 by Nicola Doebelin
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

#include "exportgraphdialog.h"
#include "ui_exportgraphdialog.h"

#include <QRadioButton>
#include <QTreeWidget>
#include <QComboBox>
#include <QProgressBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

ExportGraphDialog::ExportGraphDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportGraphDialog)
{
    // initialize the GUI
    ui->setupUi(this);
    ui->progressBar->setEnabled(false);
    ui->progressBar->reset();

    settings = SettingsManager::getInstance();

    // create an export handler, import handler, and scanHeap
    exHandler = new ExportHandler();
    iHandler = new ImportHandler();

    // populate the GUI elements with information from exHandler
    QMap<QString, QString> formats = exHandler->uidsByFilter();

    // insert all formats supported by exHandler into the formats combobox
    QMap<QString, QString>::const_iterator i = formats.constBegin();
    while (i != formats.constEnd()) {
        ui->comboBoxFormat->addItem(i.key(), QVariant(i.value()));
        ++i;
    }

    ui->comboBoxFormat->setCurrentIndex(settings->value("batchConversion/outputFormat", 0).toInt());
}

ExportGraphDialog::~ExportGraphDialog()
{
    // clean up
    while (ui->treeWidget->topLevelItemCount()) {
        delete ui->treeWidget->takeTopLevelItem(0);
    }

    settings->setValue("batchConversion/outputFormat", ui->comboBoxFormat->currentIndex());

    delete ui;
    delete exHandler;
    delete iHandler;
}

/*
 * Receives a stringlist with file names to convert
 */
void ExportGraphDialog::setFiles(const QStringList &l)
{
    ui->treeWidget->clear();

    // append the files to the treewidget
    for (int i = 0; i < l.size(); ++i) {
        QFileInfo fi(l.at(i));
        qDebug() << QString("ExportGraphDialog::setFiles(): file added %1").arg(l.at(i));
        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidget, QStringList(fi.fileName()));
        it->setData(0, Qt::UserRole, QVariant(fi.canonicalFilePath()));
        ui->treeWidget->addTopLevelItem(it);
    }
}

/*
 * runs the conversion
 */
void ExportGraphDialog::convert()
{
    QMap<QString, QVariant> flags;
    flags["fieldSeparator"] = settings->value("config/asciiFieldSeparator", QVariant(QString(" ")));
    flags["fixBgmnZero"]    = QVariant(false);

    // extract the file extension
    QString uId_out = ui->comboBoxFormat->itemData(ui->comboBoxFormat->currentIndex()).toString();
    QString index = "";

    // set up the progressbar
    ui->progressBar->setEnabled(true);
    ui->progressBar->setMaximum(ui->treeWidget->topLevelItemCount() * 100);

    // get a pointer to the exporter
    bool ok;
    GenericExport *exporter = static_cast<GenericExport *>(exHandler->exporter(uId_out, &ok));

    // check if the pointer is valid
    if (!ok) {
        QMessageBox::information(this, tr("Graph Export"), QString(tr("Error\nNo suitable export filter found.\n(id = %1)").arg(uId_out)));
        return;
    }

    // needed to calculate the progress
    int j = 0;

    // loop through all widget items
    while(ui->treeWidget->topLevelItemCount()) {
        // take the item and read the filename
        QTreeWidgetItem *it = ui->treeWidget->takeTopLevelItem(0);
        QFileInfo file(it->data(0, Qt::UserRole).toString());
        delete it;

        // erase scanHeap
        scanHeap.clear();

        // load the file into scanHeap
        QString uId_in = iHandler->uidByFileName(file.absoluteFilePath());
        int mx = iHandler->load(file.absoluteFilePath(), uId_in, scanHeap, false);
        ui->progressBar->setMaximum(mx);

        // check if the export file format supports multiscan files, and if the option of multiscans was selected
        if (exporter->hasMultiScanSupport() && ui->radioButtonMultiScanFiles->isChecked()) {
            // the export filter supports multiple scans in one file. so do it like this
            QString fn = QString("%1/%2.%3").arg(file.absolutePath(), file.completeBaseName(), exporter->extension());

            qDebug() << QString("ExportGraphDialog::convert(): Exporting file %1").arg(fn);

            // write the whole scanHeap into file 'fn'
            exporter->save(fn, scanHeap, flags);
            ui->progressBar->setValue(mx);
            qApp->processEvents();
        } else {
            // write each scan to a separate file with "_i" added to the file name
            for (int i = 0; i < (int)scanHeap.size(); ++i) {
                // only append "_i" to the file name if scanHeap contains several scans
                if (scanHeap.size() > 1) {
                    index = QString("_%1").arg(i);
                }

                // compose the file name
                QString fn = QString("%1/%2%3.%4").arg(file.absolutePath()).arg(file.completeBaseName()).arg(index).arg(exporter->extension());
                qDebug() << QString("ExportGraphDialog::convert(): Batch exporting file %1").arg(fn);

                // write the current scan from scanHeap to file 'fn_i'
                exporter->save(fn, scanHeap[i], flags);
                ui->progressBar->setValue(j * 100 + i * 100 / scanHeap.size());
                qApp->processEvents();
            }

            // go to the next scan in scanHeap
            j++;
            ui->progressBar->setValue(j * 100);
            qApp->processEvents();
        }

        // go to the next file in the treeWidget
    }

    accept();
}

/*
 * Calls a fileOpenDialog and adds a file, e.g. when the "add file" button is pressed
 */
void ExportGraphDialog::addFile()
{
    QString dir = settings->value("batchConversion/sourceDir", QString()).toString();

    if (ui->treeWidget->topLevelItemCount()) {
        // determine the dir of the last file in the treewidget, and start from this directory
        QString tdir = ui->treeWidget->topLevelItem(ui->treeWidget->topLevelItemCount()-1)->data(0, Qt::UserRole).toString();
        QFileInfo dfi(tdir);
        dir = dfi.absolutePath();
    }

    // collect the import filters
    ImportHandler *iHandler = new ImportHandler();
    QStringList filter = iHandler->filters();
    delete iHandler;

    // get the new files to be added
    QStringList lst = QFileDialog::getOpenFileNames(this, tr("Add files"), dir, filter.join(";;"));

    if (!lst.size()) {
        return;
    }

    // add the new files to the treewidget
    for (int i = 0; i < lst.size(); ++i) {
        QFileInfo fi(lst.at(i));
        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidget, QStringList(fi.fileName()));
        it->setData(0, Qt::UserRole, QVariant(fi.canonicalFilePath()));
        ui->treeWidget->addTopLevelItem(it);
    }

    QFileInfo fi(lst.first());
    settings->setValue("batchConversion/sourceDir", fi.absolutePath());
}

/*
 * removes all selected files from the treewidget
 */
void ExportGraphDialog::removeFile()
{
    QList<QTreeWidgetItem *> lst = ui->treeWidget->selectedItems();

    for (int i = 0; i < lst.count(); ++i) {
        delete lst.at(i);
    }
}

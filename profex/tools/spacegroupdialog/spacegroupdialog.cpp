/***************************************************************************
                          spacegroupdialog.h  -  description
                             -------------------
    begin                : Feb 07 11:00:00 CEST 2013
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

#include "spacegroupdialog.h"
#include "ui_spacegroupdialog.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QLineEdit>
#include <QListView>
#include <QItemSelectionModel>
#include <QList>
#include <QFileInfo>
#include <QDebug>

SpacegroupDialog::SpacegroupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SpacegroupDialog)
{
    ui->setupUi(this);
    hdata = false;

    model = new QStandardItemModel(this);
    spgrModel = new QSortFilterProxyModel(this);
    hmModel = new QSortFilterProxyModel(this);
    wyckModel = new QSortFilterProxyModel(this);
    symmModel = new QSortFilterProxyModel(this);

    ui->listViewSpgrNo->setModel(spgrModel);
    ui->listViewHM->setModel(hmModel);
    ui->listViewWyckoff->setModel(wyckModel);
    ui->listViewSymmetry->setModel(symmModel);

    ui->comboBoxFilterHM->addItem(QString("(show all)"), QVariant(QString(".*")));
    ui->comboBoxFilterHM->addItem(QString("P"), QVariant(QString("^\\d+$|^P.+$")));
    ui->comboBoxFilterHM->addItem(QString("A"), QVariant(QString("^\\d+$|^A.+$")));
    ui->comboBoxFilterHM->addItem(QString("B"), QVariant(QString("^\\d+$|^B.+$")));
    ui->comboBoxFilterHM->addItem(QString("C"), QVariant(QString("^\\d+$|^C.+$")));
    ui->comboBoxFilterHM->addItem(QString("I"), QVariant(QString("^\\d+$|^I.+$")));
    ui->comboBoxFilterHM->addItem(QString("R"), QVariant(QString("^\\d+$|^R.+$")));
    ui->comboBoxFilterHM->addItem(QString("F"), QVariant(QString("^\\d+$|^F.+$")));

    ui->comboBoxFilterNo->addItem(QString("(show all)"),
                                  QVariant(QString("^\\d+$")));                                    // 1 - 230
    ui->comboBoxFilterNo->addItem(QString("Triclinic"),
                                  QVariant(QString("^([1-2])$")));                                 // 1 - 2
    ui->comboBoxFilterNo->addItem(QString("Monoclinic"),
                                  QVariant(QString("^([3-9]?|1[0-5])$")));                         // 3 - 15
    ui->comboBoxFilterNo->addItem(QString("Orthorhombic"),
                                  QVariant(QString("^(1[6-9]|[2-6][0-9]|7[0-4])$")));              // 16 - 74
    ui->comboBoxFilterNo->addItem(QString("Tetragonal"),
                                  QVariant(QString("^(7[5-9]|[8-9][0-9]|1[0-3][0-9]|14[0-2])$"))); // 75 - 142
    ui->comboBoxFilterNo->addItem(QString("Trigonal"),
                                  QVariant(QString("^(14[3-9]|15[0-9]|16[0-7])$")));               // 143 - 167
    ui->comboBoxFilterNo->addItem(QString("Hexagonal"),
                                  QVariant(QString("^(16[8-9]|1[7-8][0-9]|19[0-4])$")));           // 168 - 194
    ui->comboBoxFilterNo->addItem(QString("Cubic"),
                                  QVariant(QString("^(19[5-9]|2[0-2][0-9]|230)$")));               // 195 - 230

    QItemSelectionModel *selectionModelSpgrNo = ui->listViewSpgrNo->selectionModel();
    QItemSelectionModel *selectionModelHM = ui->listViewHM->selectionModel();
    QItemSelectionModel *selectionModelWyckoff = ui->listViewWyckoff->selectionModel();

    connect(selectionModelSpgrNo, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(spgrSelectionChanged(QModelIndex,QModelIndex)));
    connect(selectionModelHM, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(hmSelectionChanged(QModelIndex,QModelIndex)));
    connect(selectionModelWyckoff, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(wyckoffSelectionChanged(QModelIndex,QModelIndex)));
    connect(ui->comboBoxFilterHM, SIGNAL(currentIndexChanged(int)), this, SLOT(hmFilterChanged(int)));
    connect(ui->comboBoxFilterNo, SIGNAL(currentIndexChanged(int)), this, SLOT(spgrFilterChanged(int)));
}

SpacegroupDialog::~SpacegroupDialog()
{
    delete spgrModel;
    delete hmModel;
    delete wyckModel;
    delete symmModel;
    delete model;
    delete ui;
}

void SpacegroupDialog::clear()
{
    model->clear();
    hdata = false;
    ui->comboBoxFilterNo->setCurrentIndex(0);
    ui->comboBoxFilterHM->setCurrentIndex(0);
}

/*
 * get the directory or the full file name with path to be parsed (SPACEGRP.DAT)
 */
void SpacegroupDialog::setDirectory(const QString &s)
{
    time.start();

    // clear old data
    clear();

    // we accept either a directory or a complete file name
    QFileInfo fi(s);
    QString fd;

    if (fi.isFile()) {
        fd = fi.absolutePath();
    } else if (fi.isDir()) {
        fd = s;
    }

    QString fn = QString("%1/%2").arg(fd).arg("spacegrp.dat");

    // read the file content into a stringlist
    QFile f(fn);

    if (!f.open(QFile::ReadOnly)) {
        qDebug() << QString("SpacegroupDialog::setDirectory(): No valid file name: %1").arg(fn);
        hdata = false;
        return;
    }

    QTextStream str(&f);
    QString content = str.readAll();
    f.close();

    qDebug() << QString("SpacegroupDialog::setDirectory(): File read in %1 ms").arg(time.elapsed());
    time.start();

    // parse the content
    parseModel(content);
    hdata = true;

    qDebug() << QString("SpacegroupDialog::setDirectory(): File parsed in %1 ms").arg(time.elapsed());
}

/*
 * parse the content and build the data model
 */
void SpacegroupDialog::parseModel(const QString &content)
{
    QRegularExpression rxSpgNo("^(\\d+)");
    QRegularExpression rxHM("HermannMauguin=(\\S+)");
    QRegularExpression rxWyckoff("^([a-z]+)\\s+N=(\\d+)");

    // we split the file content at the string "SpacegroupNo=", so every
    // item starts with the number of the space group
    QStringList lstSpgroups = content.split("SpacegroupNo=");

    QStandardItem *modSpgrNo = new QStandardItem();
    QStandardItem *modHM = new QStandardItem();
    QStandardItem *modWyckoff = new QStandardItem();

    model->blockSignals(true);

    // loop over all spacegroups
    for (int i = 0; i < lstSpgroups.size(); ++i) {
        // we store the "SpacegroupNo=" line for later use
        QString line = lstSpgroups.at(i).left(lstSpgroups.at(i).indexOf(QRegularExpression("\\n"), 0));

        // check if a spacegroupNo model item with the same text already exists.
        // if not, append a new one and set the pointer "modSpgrNo" to it
        QRegularExpressionMatch matchSpgrNo = rxSpgNo.match(lstSpgroups.at(i));
        if (matchSpgrNo.hasMatch()) {
            if (matchSpgrNo.captured(1) != modSpgrNo->text()) {
                modSpgrNo = new QStandardItem(matchSpgrNo.captured(1));
                model->appendRow(modSpgrNo);
            }
        }

        // check if a HM symbol model item with the same text already exists.
        // if not, append a new one and set the pointer "modHM" to it
        QRegularExpressionMatch matchHM = rxHM.match(lstSpgroups.at(i));
        if (matchHM.hasMatch()) {
            modHM = new QStandardItem(matchHM.captured(1));
            modHM->setData(QVariant(QString("SpacegroupNo=%1").arg(line)));
            modSpgrNo->appendRow(modHM);
        }

        // now we split the rest of the block at the "Wyckoff=" position. Each item
        // will start with the Wyckoff symbol
        QStringList lstWyckoff = lstSpgroups.at(i).split("Wyckoff=");
        for (int j = 1; j < lstWyckoff.size(); ++j) {
            QRegularExpressionMatch matchWyckoff = rxWyckoff.match(lstWyckoff.at(j));
            if (matchWyckoff.hasMatch()) {
                modWyckoff = new QStandardItem(QString("%1 (N=%2)").arg(matchWyckoff.captured(1)).arg(matchWyckoff.captured(2)));
                modWyckoff->setData(QVariant(QString("SpacegroupNo=%1").arg(line)));
                modHM->appendRow(modWyckoff);
            }

            // now we split the block of the wyckoff sequence into lines
            QStringList lstPos = lstWyckoff.at(j).split("\n");
            for (int k = 1; k < lstPos.size(); ++k) {
                if (lstPos.at(k).isEmpty()) {
                    continue;
                }

                QStandardItem *modPos = new QStandardItem(lstPos.at(k));
                modPos->setData(QVariant(QString("SpacegroupNo=%1").arg(line)));
                modWyckoff->appendRow(modPos);
            }
        }
    }

    model->blockSignals(false);

    spgrModel->setSourceModel(model);
    hmModel->setSourceModel(model);
    wyckModel->setSourceModel(model);
    symmModel->setSourceModel(model);

    QItemSelection spgrSelection(spgrModel->index(0, 0), spgrModel->index(0, 0));
    ui->listViewSpgrNo->selectionModel()->select(spgrSelection, QItemSelectionModel::ClearAndSelect);
    spgrSelectionChanged(spgrModel->index(0, 0), QModelIndex());
}

/*
 * updates the views when the spacegroup number selection has changed
 * Attention: The QModelIndex i belongs to spgrModel, not model
 */
void SpacegroupDialog::spgrSelectionChanged(const QModelIndex &current, const QModelIndex &)
{
    QModelIndex hmIdx(hmModel->mapFromSource(spgrModel->mapToSource(current)));

    ui->listViewHM->setRootIndex(hmIdx);

    if (hmModel->hasChildren(hmIdx)) {
        QItemSelection selHM(hmModel->index(0, 0, hmIdx), hmModel->index(0, 0, hmIdx));
        ui->listViewHM->selectionModel()->select(selHM, QItemSelectionModel::Select);
        hmSelectionChanged(hmModel->index(0, 0, hmIdx), QModelIndex());
    }
}

/*
 * updates the views when the Hermann Mauguin selection has changed
 * Attention: The QModelIndex "current" belongs to hmModel, not model
 */
void SpacegroupDialog::hmSelectionChanged(const QModelIndex &current, const QModelIndex &)
{
    QModelIndex sourceIdx(hmModel->mapToSource(current));
    QModelIndex wyckIdx(wyckModel->mapFromSource(sourceIdx));
    QString prevSelectedWyckoff(getSelectedWyckoffText());

    ui->listViewWyckoff->setRootIndex(wyckIdx);

    if (wyckIdx.isValid()) {
        ui->lineEditLine->setText(model->itemFromIndex(sourceIdx)->data().toString());
    }

    if (wyckModel->hasChildren(wyckIdx)) {
        QModelIndex selectedWyckoff = selectWyckoff(prevSelectedWyckoff);
        if (selectedWyckoff.isValid()) {
            wyckoffSelectionChanged(selectedWyckoff, QModelIndex());
        }
    }
}

/*
 * updates the views when the Wyckoff selection has changed
 */
void SpacegroupDialog::wyckoffSelectionChanged(const QModelIndex &current, const QModelIndex &)
{
    ui->listViewSymmetry->setRootIndex(symmModel->mapFromSource(wyckModel->mapToSource(current)));
}

/*
 * applies a filter to the spacegroups
 */
void SpacegroupDialog::spgrFilterChanged(int i)
{
    spgrModel->setFilterRegularExpression(QRegularExpression(ui->comboBoxFilterNo->itemData(i).toString()));
    ui->listViewSpgrNo->selectionModel()->select(spgrModel->index(0, 0, QModelIndex()), QItemSelectionModel::ClearAndSelect);
    spgrSelectionChanged(spgrModel->index(0, 0), QModelIndex());
}

/*
 * applies a filter to the hm symbols
 */
void SpacegroupDialog::hmFilterChanged(int i)
{
    hmModel->setFilterRegularExpression(QRegularExpression(ui->comboBoxFilterHM->itemData(i).toString()));

    if (hmModel->hasChildren(ui->listViewHM->rootIndex())) {
        ui->listViewHM->selectionModel()->select(hmModel->index(0, 0, ui->listViewHM->rootIndex()), QItemSelectionModel::ClearAndSelect);
        hmSelectionChanged(hmModel->index(0, 0, ui->listViewHM->rootIndex()), QModelIndex());
    }
}

QString SpacegroupDialog::getSelectedWyckoffText()
{
    QModelIndexList selected = ui->listViewWyckoff->selectionModel()->selectedIndexes();

    if (selected.size()) {
        return model->itemFromIndex(wyckModel->mapToSource(selected.first()))->text().left(1);
    }

    return QString("a");
}

QModelIndex SpacegroupDialog::selectWyckoff(const QString &s)
{
    QModelIndex wyckRoot = ui->listViewWyckoff->rootIndex();
    QModelIndex wyckIdx = wyckModel->index(0, 0, wyckRoot);

    if (!wyckIdx.isValid()) return QModelIndex();

    int rows = model->itemFromIndex(wyckModel->mapToSource(wyckRoot))->rowCount();

    for (int n = 0; n < rows; ++n) {
        wyckIdx = wyckModel->index(n, 0, wyckRoot);

        QModelIndex curWyckIdx(wyckModel->mapToSource(wyckIdx));
        QStandardItem *curWyckItm = model->itemFromIndex(curWyckIdx);

        if (curWyckItm->text().left(1) == s) break;
    }

    QItemSelection selWyck(wyckIdx, wyckIdx);
    ui->listViewWyckoff->selectionModel()->select(selWyck, QItemSelectionModel::Select);
    return wyckIdx;
}

/***************************************************************************
                          strfilebatcheditdialog.cpp  -  description
                             -------------------
    begin                : Sun Dec 23 11:00:00 CEST 2018
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

#include "strfilebatcheditdialog.h"
#include "ui_strfilebatcheditdialog.h"

#include <QComboBox>
#include <QFileInfo>
#include <QDebug>

StrFileBatchEditDialog::StrFileBatchEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StrFileBatchEditDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    initSettings();
    initParameterWidget();
}

StrFileBatchEditDialog::~StrFileBatchEditDialog()
{
    QList<QVariant> wlst;
    for (int i = 0; i < ui->treeWidgetParameters->columnCount(); ++i) {
        wlst.append(QVariant(ui->treeWidgetParameters->columnWidth(i)));
    }

    settings->setValue("strEditDialog/columnWidths", wlst);

    settings->setValue("strEditDialog/geometry", saveGeometry());
    settings->setValue("strEditDialog/splitter", ui->splitter->saveState());
    delete ui;
}

void StrFileBatchEditDialog::apply()
{
    emit sigApply();
}

void StrFileBatchEditDialog::initSettings()
{
    restoreGeometry(settings->value("strEditDialog/geometry", QByteArray()).toByteArray());

    QByteArray baSplitter = settings->value("strEditDialog/splitter", QByteArray()).toByteArray();

    if (baSplitter.isEmpty()) {
        QList<int> spSizes;
        spSizes << 100 << 500;
        ui->splitter->setSizes(spSizes);
    } else {
        ui->splitter->restoreState(baSplitter);
    }

    QStringList headersFiles;
    QStringList headersParams;

    headersFiles << tr("File");
    headersParams << tr("Modified Parameter") << tr("State");

    ui->treeWidgetFiles->setHeaderLabels(headersFiles);
    ui->treeWidgetParameters->setHeaderLabels(headersParams);

    QList<QVariant> wlst = settings->value("strEditDialog/columnWidths", QVariant()).toList();

    QFontMetrics fm(this->font());
    int minColWdt = fm.horizontalAdvance("Parameter");

    for (int i = 0; i < ui->treeWidgetParameters->columnCount(); ++i) {
        int dw = i ? minColWdt : 2 * minColWdt;
        ui->treeWidgetParameters->setColumnWidth(i, i < wlst.size() ? wlst.at(i).toInt() : dw);
    }

    ucellLimits = settings->value("cifimport/limits", 0.01).toDouble();
    coordLimits = settings->value("bgmnProject/coordinatelimits", 0.05).toDouble();
    b1Limit     = settings->value("bgmnProject/b1limits", 0.01).toDouble();
    k2Limit     = settings->value("bgmnProject/k2limits", 0.0001).toDouble();
    tdsLimit    = settings->value("bgmnProject/tdslimits", 0.02).toDouble();
}

void StrFileBatchEditDialog::initParameterWidget()
{
    QStringList stateIso;
    QStringList stateAniso;
    QStringList stateSphar;

    stateIso << "Fix" << "Refined";
    stateAniso << "Fix" << "Isotropic" << "Anisotropic";
    stateSphar << "SPHAR0" << "SPHAR2" << "SPHAR4" << "SPHAR6" << "SPHAR8" << "SPHAR10";

    // create toplevel items for groups
    QTreeWidgetItem *tlUnitCell = new QTreeWidgetItem(ui->treeWidgetParameters, QStringList(tr("Unit Cell")));
    QTreeWidgetItem *tlProfile = new QTreeWidgetItem(ui->treeWidgetParameters, QStringList(tr("Profile")));
    QTreeWidgetItem *tlAtoms = new QTreeWidgetItem(ui->treeWidgetParameters, QStringList(tr("Atomic Sites")));

    tlUnitCell->setData(0, Qt::UserRole, QVariant("UC"));
    tlProfile->setData(0, Qt::UserRole, QVariant("PR"));
    tlAtoms->setData(0, Qt::UserRole, QVariant("AT"));

    tlUnitCell->setExpanded(true);
    tlProfile->setExpanded(true);
    tlAtoms->setExpanded(true);

    ui->treeWidgetParameters->addTopLevelItem(tlUnitCell);
    ui->treeWidgetParameters->addTopLevelItem(tlProfile);
    ui->treeWidgetParameters->addTopLevelItem(tlAtoms);

    // child items for Unit cell
    QComboBox *cboxUcA = new QComboBox();
    QComboBox *cboxUcB = new QComboBox();
    QComboBox *cboxUcC = new QComboBox();
    QComboBox *cboxUcAl = new QComboBox();
    QComboBox *cboxUcBe = new QComboBox();
    QComboBox *cboxUcGa = new QComboBox();

    cboxUcA->addItems(stateIso);
    cboxUcB->addItems(stateIso);
    cboxUcC->addItems(stateIso);
    cboxUcAl->addItems(stateIso);
    cboxUcBe->addItems(stateIso);
    cboxUcGa->addItems(stateIso);

    QTreeWidgetItem *itUcA = new QTreeWidgetItem(tlUnitCell, QStringList("A"));
    QTreeWidgetItem *itUcB = new QTreeWidgetItem(tlUnitCell, QStringList("B"));
    QTreeWidgetItem *itUcC = new QTreeWidgetItem(tlUnitCell, QStringList("C"));
    QTreeWidgetItem *itUcAl = new QTreeWidgetItem(tlUnitCell, QStringList("ALPHA"));
    QTreeWidgetItem *itUcBe = new QTreeWidgetItem(tlUnitCell, QStringList("BETA"));
    QTreeWidgetItem *itUcGa = new QTreeWidgetItem(tlUnitCell, QStringList("GAMMA"));

    itUcA->setCheckState(0, Qt::Unchecked);
    itUcB->setCheckState(0, Qt::Unchecked);
    itUcC->setCheckState(0, Qt::Unchecked);
    itUcAl->setCheckState(0, Qt::Unchecked);
    itUcBe->setCheckState(0, Qt::Unchecked);
    itUcGa->setCheckState(0, Qt::Unchecked);

    tlUnitCell->addChild(itUcA);
    tlUnitCell->addChild(itUcB);
    tlUnitCell->addChild(itUcC);
    tlUnitCell->addChild(itUcAl);
    tlUnitCell->addChild(itUcBe);
    tlUnitCell->addChild(itUcGa);

    ui->treeWidgetParameters->setItemWidget(itUcA, 1, cboxUcA);
    ui->treeWidgetParameters->setItemWidget(itUcB, 1, cboxUcB);
    ui->treeWidgetParameters->setItemWidget(itUcC, 1, cboxUcC);
    ui->treeWidgetParameters->setItemWidget(itUcAl, 1, cboxUcAl);
    ui->treeWidgetParameters->setItemWidget(itUcBe, 1, cboxUcBe);
    ui->treeWidgetParameters->setItemWidget(itUcGa, 1, cboxUcGa);


    // child items for Profile
    QComboBox *cboxGw = new QComboBox();
    QComboBox *cboxB1 = new QComboBox();
    QComboBox *cboxk2 = new QComboBox();
    QComboBox *cboxk1 = new QComboBox();

    cboxGw->addItems(stateSphar);
    cboxB1->addItems(stateAniso);
    cboxk2->addItems(stateAniso);
    cboxk1->addItems(stateIso);

    QTreeWidgetItem *itGw = new QTreeWidgetItem(tlProfile, QStringList("GEWICHT"));
    QTreeWidgetItem *itB1 = new QTreeWidgetItem(tlProfile, QStringList("B1"));
    QTreeWidgetItem *itk2 = new QTreeWidgetItem(tlProfile, QStringList("k2"));
    QTreeWidgetItem *itk1 = new QTreeWidgetItem(tlProfile, QStringList("k1"));

    itGw->setCheckState(0, Qt::Unchecked);
    itB1->setCheckState(0, Qt::Unchecked);
    itk2->setCheckState(0, Qt::Unchecked);
    itk1->setCheckState(0, Qt::Unchecked);

    tlProfile->addChild(itGw);
    tlProfile->addChild(itB1);
    tlProfile->addChild(itk2);
    tlProfile->addChild(itk1);

    ui->treeWidgetParameters->setItemWidget(itGw, 1, cboxGw);
    ui->treeWidgetParameters->setItemWidget(itB1, 1, cboxB1);
    ui->treeWidgetParameters->setItemWidget(itk2, 1, cboxk2);
    ui->treeWidgetParameters->setItemWidget(itk1, 1, cboxk1);


    // Atoms
    QComboBox *cboxAtX = new QComboBox();
    QComboBox *cboxAtY = new QComboBox();
    QComboBox *cboxAtZ = new QComboBox();
    QComboBox *cboxAtT = new QComboBox();

    cboxAtX->addItems(stateIso);
    cboxAtY->addItems(stateIso);
    cboxAtZ->addItems(stateIso);
    cboxAtT->addItems(stateAniso);

    QTreeWidgetItem *itAtX = new QTreeWidgetItem(tlAtoms, QStringList("x"));
    QTreeWidgetItem *itAtY = new QTreeWidgetItem(tlAtoms, QStringList("y"));
    QTreeWidgetItem *itAtZ = new QTreeWidgetItem(tlAtoms, QStringList("z"));
    QTreeWidgetItem *itAtT = new QTreeWidgetItem(tlAtoms, QStringList("TDS"));

    itAtX->setCheckState(0, Qt::Unchecked);
    itAtY->setCheckState(0, Qt::Unchecked);
    itAtZ->setCheckState(0, Qt::Unchecked);
    itAtT->setCheckState(0, Qt::Unchecked);

    tlAtoms->addChild(itAtX);
    tlAtoms->addChild(itAtY);
    tlAtoms->addChild(itAtZ);
    tlAtoms->addChild(itAtT);

    ui->treeWidgetParameters->setItemWidget(itAtX, 1, cboxAtX);
    ui->treeWidgetParameters->setItemWidget(itAtY, 1, cboxAtY);
    ui->treeWidgetParameters->setItemWidget(itAtZ, 1, cboxAtZ);
    ui->treeWidgetParameters->setItemWidget(itAtT, 1, cboxAtT);
}

void StrFileBatchEditDialog::reset()
{
    ucellLimits = settings->value("cifimport/limits", 0.01).toDouble();
    coordLimits = settings->value("bgmnProject/coordinatelimits", 0.05).toDouble();
    b1Limit     = settings->value("bgmnProject/b1limits", 0.01).toDouble();
    k2Limit     = settings->value("bgmnProject/k2limits", 0.0001).toDouble();
    tdsLimit    = settings->value("bgmnProject/tdslimits", 0.02).toDouble();

    ui->treeWidgetFiles->clear();
    openFileList.clear();
    currentFile.clear();

    for (int t = 0; t < ui->treeWidgetParameters->topLevelItemCount(); ++t) {
        QTreeWidgetItem *tit = ui->treeWidgetParameters->topLevelItem(t);

        if (!tit) continue;

        for (int c = 0; c < tit->childCount(); ++c) {
            QTreeWidgetItem *cit = tit->child(c);
            if (!cit) continue;

            QComboBox *cbx = static_cast<QComboBox*>(ui->treeWidgetParameters->itemWidget(cit, 1));
            cit->setCheckState(0, Qt::Unchecked);
            cbx->setCurrentIndex(0);
        }
    }
}

void StrFileBatchEditDialog::setStrFileList(const QStringList &l)
{
    ui->treeWidgetFiles->clear();

    for (int i = 0; i < l.size(); ++i) {
        QTreeWidgetItem *it = new QTreeWidgetItem();
        QFileInfo fi(l.at(i));

        it->setCheckState(0, Qt::Checked);
        it->setText(0, fi.fileName());
        it->setData(0, Qt::UserRole, QVariant(fi.absoluteFilePath()));
        it->setToolTip(0, fi.absoluteFilePath());

        ui->treeWidgetFiles->addTopLevelItem(it);
    }
}

void StrFileBatchEditDialog::setCurrentFile(const QString &s)
{
    currentFile = s;
    ui->buttonFilesCurrent->setDisabled(currentFile.isEmpty());
}

void StrFileBatchEditDialog::setOpenFileList(const QStringList &l)
{
    openFileList = l;
    ui->buttonFilesOpen->setDisabled(openFileList.isEmpty());
}

/*
 * returns a map with parameters and values.
 * Keys are composed of "Category:Parameter:Limits", with category being one of:
 *
 * "UC", "PR", "AT"
 *
 * parameter being one of:
 *
 *  "A", "B", "C", "ALPHA", "BETA", "GAMMA", "GEWICHT", "B1", "k2", "k1", "x", "y", "z", "TDS"
 *
 * limits being the limits for refined parameters used in the preferences, or QString().
 *
 * values of the map are just integers representing the following:
 *
 * 0: fix (or SPHAR0)
 * 1: isotropic (or SPHAR2)
 * 2: anisotropic (or SPHAR4)
 * n: SPHARn
 */
QMap<QString, QVariant> StrFileBatchEditDialog::getParameters()
{
    QMap<QString, QVariant> params;

    // collect all checked files
    QList<QVariant> files;

    for (int i = 0; i < ui->treeWidgetFiles->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetFiles->topLevelItem(i);

        if (it->checkState(0) == Qt::Checked) {
            files.append(it->data(0, Qt::UserRole));
        }
    }

    params["files"] = QVariant(files);

    for (int i = 0; i < ui->treeWidgetParameters->topLevelItemCount(); ++i) {
        QTreeWidgetItem *tit = ui->treeWidgetParameters->topLevelItem(i);
        for (int j = 0; j < tit->childCount(); ++j) {
            QTreeWidgetItem *cit = tit->child(j);
            QComboBox *cbx = static_cast<QComboBox*>(ui->treeWidgetParameters->itemWidget(cit, 1));

            QString lim;
            if (tit->data(0, Qt::UserRole).toString() == "UC") lim = QString("%1").arg(ucellLimits, 0, 'f', 6);
            if (tit->data(0, Qt::UserRole).toString() == "AT") lim = QString("%1").arg(coordLimits, 0, 'f', 6);
            if (cit->text(0) == "B1") lim = QString("%1").arg(b1Limit, 0, 'f', 2);
            if (cit->text(0) == "k2") lim = QString("%1").arg(k2Limit, 0, 'f', 6);
            if (cit->text(0) == "TDS") lim = QString("%1").arg(tdsLimit, 0, 'f', 2);

            QString key(tit->data(0, Qt::UserRole).toString() + ":" + cit->text(0) + ":" + lim);
            QVariant val(cbx->currentIndex());

            if (cit->checkState(0) == Qt::Checked) {
                params[key] = val;
            }
        }
    }

    return params;
}

void StrFileBatchEditDialog::filesCheckAll()
{
    for (int i = 0; i < ui->treeWidgetFiles->topLevelItemCount(); ++i) {
        ui->treeWidgetFiles->topLevelItem(i)->setCheckState(0, Qt::Checked);
    }
}

void StrFileBatchEditDialog::filesUncheckAll()
{
    for (int i = 0; i < ui->treeWidgetFiles->topLevelItemCount(); ++i) {
        ui->treeWidgetFiles->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
    }
}

void StrFileBatchEditDialog::filesCheckOpen()
{
    for (int i = 0; i < ui->treeWidgetFiles->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetFiles->topLevelItem(i);

        if (openFileList.contains(it->data(0, Qt::UserRole).toString())) {
            it->setCheckState(0, Qt::Checked);
        } else {
            it->setCheckState(0, Qt::Unchecked);
        }
    }
}

void StrFileBatchEditDialog::filesCheckCurrent()
{
    for (int i = 0; i < ui->treeWidgetFiles->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetFiles->topLevelItem(i);

        if (currentFile == it->data(0, Qt::UserRole).toString()) {
            it->setCheckState(0, Qt::Checked);
        } else {
            it->setCheckState(0, Qt::Unchecked);
        }
    }
}

void StrFileBatchEditDialog::parametersCheckGroup()
{
    setParamsGroupCheckState(true);
}

void StrFileBatchEditDialog::parametersUncheckGroup()
{
    setParamsGroupCheckState(false);
}

void StrFileBatchEditDialog::setParamsGroupCheckState(bool b)
{
    // if no current item is selected, all child items are changed, else
    // only the child items of the current group

    QList<QTreeWidgetItem*> tlItems;

    // get a list of the current or all toplevelitems
    if (ui->treeWidgetParameters->currentItem()) {
        if (isToplevelItem(ui->treeWidgetParameters->currentItem())) {
            tlItems.append(ui->treeWidgetParameters->currentItem());
        } else {
            tlItems.append(ui->treeWidgetParameters->currentItem()->parent());
        }
    } else {
        for (int i = 0; i < ui->treeWidgetParameters->topLevelItemCount(); ++i) {
            tlItems.append(ui->treeWidgetParameters->topLevelItem(i));
        }
    }

    // change the state of all child items of the gathered toplevelitems
    for (int i = 0; i < tlItems.count(); ++i) {
        for (int n = 0; n < tlItems.at(i)->childCount(); ++n) {
            QTreeWidgetItem *it = tlItems.at(i)->child(n);
            it->setCheckState(0, b ? Qt::Checked : Qt::Unchecked);
        }
    }
}

void StrFileBatchEditDialog::parametersToggleGroup()
{
    QTreeWidgetItem *tit = Q_NULLPTR;
    QTreeWidgetItem *cit = ui->treeWidgetParameters->currentItem();

    if (!cit) return;

    // assign tit to the current toplevelitem, and cit either to the selected child, or to the first child
    if (isToplevelItem(cit)) {
        if (!cit->childCount()) return;
        tit = cit;
        cit = tit->child(0);
    } else {
        tit = cit->parent();
    }

    int max = 0;
    int cur = 0;

    // find the highest combo box item count in all child items
    for (int i = 0; i < tit->childCount(); ++i) {
        QComboBox *cbxc = static_cast<QComboBox*>(ui->treeWidgetParameters->itemWidget(tit->child(i), 1));
        max = qMax(max, cbxc->count() - 1);
        cur = qMax(cur, cbxc->currentIndex());
    }

    cur = cur < max ? cur + 1 : 0;

    for (int i = 0; i < tit->childCount(); ++i) {
        QComboBox *cbxc = static_cast<QComboBox*>(ui->treeWidgetParameters->itemWidget(tit->child(i), 1));
        cbxc->setCurrentIndex(cur >= cbxc->count() ? cbxc->count() - 1 : cur);
    }
}

bool StrFileBatchEditDialog::isToplevelItem(const QTreeWidgetItem *it)
{
    return it->parent() == Q_NULLPTR;
}

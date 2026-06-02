/***************************************************************************
                          prefpagescanstyle.cpp  -  description
                             -------------------
    begin                : Tue May 09 16:00:00 CEST 2017
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

#include "prefpagescanstyle.h"
#include "ui_prefpagescanstyle.h"

#include <QComboBox>
#include <QColorDialog>
#include <QMessageBox>

PrefPageScanStyle::PrefPageScanStyle(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageScanStyle)
{
    ui->setupUi(this);
}

PrefPageScanStyle::~PrefPageScanStyle()
{
    delete ui;
}

void PrefPageScanStyle::initUi()
{
    ui->toolButtonResetList->setIcon(QIcon::fromTheme("profex-undo"));
    connect(ui->toolButtonAddColor,    SIGNAL(clicked(bool)), this, SLOT(addColor()));
    connect(ui->toolButtonRemoveColor, SIGNAL(clicked(bool)), this, SLOT(removeColor()));
    connect(ui->treeWidgetColors,      SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    connect(ui->treeWidgetColors,      SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(colorChanged(QTreeWidgetItem*,int)));
    connect(ui->pushButtonColIObs,     SIGNAL(clicked(bool)), this, SLOT(iObsColorChanged()));
    connect(ui->pushButtonColICalc,    SIGNAL(clicked(bool)), this, SLOT(iCalcColorChanged()));
    connect(ui->pushButtonColIDiff,    SIGNAL(clicked(bool)), this, SLOT(iDiffColorChanged()));
    connect(ui->pushButtonColBkgr,     SIGNAL(clicked(bool)), this, SLOT(iBkgrColorChanged()));
    connect(ui->toolButtonResetList,   SIGNAL(clicked(bool)), this, SLOT(resetList()));
    initSettings();
}

void PrefPageScanStyle::initSettings()
{
    pointStyles = settings->getScanStyleNames();
    setColorTable(settings->getColorList(10));
    setStyleTable(settings->getScanStyleList(10));

    colIobs  = QColor(settings->value("graph/iobsColor", "#000000").toString());
    colIcalc = QColor(settings->value("graph/icalcColor", "#ff0000").toString());
    colIdiff = QColor(settings->value("graph/idiffColor", "#bbbbbb").toString());
    colIbkgr = QColor(settings->value("graph/ibkgrColor", "#0000ff").toString());

    ui->pushButtonColIObs->setStyleSheet(QString("background-color:%1").arg(colIobs.name()));
    ui->pushButtonColICalc->setStyleSheet(QString("background-color:%1").arg(colIcalc.name()));
    ui->pushButtonColIDiff->setStyleSheet(QString("background-color:%1").arg(colIdiff.name()));
    ui->pushButtonColBkgr->setStyleSheet(QString("background-color:%1").arg(colIbkgr.name()));

    ui->treeWidgetColors->header()->restoreState(settings->value("preferencesDialog/scanStyleHeaderState", QByteArray()).toByteArray());
}


void PrefPageScanStyle::saveSettings()
{
    settings->setColorList(getColorTable());
    settings->setScanStyleList(getStyleTable());

    settings->setValue("graph/iobsColor",  colIobs);
    settings->setValue("graph/icalcColor", colIcalc);
    settings->setValue("graph/idiffColor", colIdiff);
    settings->setValue("graph/ibkgrColor", colIbkgr);

    settings->setValue("preferencesDialog/scanStyleHeaderState", ui->treeWidgetColors->header()->saveState());
}


void PrefPageScanStyle::addColor()
{
    int n = ui->treeWidgetColors->topLevelItemCount();

    QColor c = QColorDialog::getColor(settings->getRandomColor(n), this);
    if (!c.isValid()) return;

    QTreeWidgetItem *it = new QTreeWidgetItem(QStringList(QString("%1").arg(n+1)));
    it->setBackground(0, c);

    if (c.lightness() < 100) {
        it->setForeground(0, QBrush(Qt::white));
    }

    ui->treeWidgetColors->addTopLevelItem(it);

    QComboBox *cb = new QComboBox(this);
    ui->treeWidgetColors->setItemWidget(it, 1, cb);
    cb->addItems(pointStyles);
}

void PrefPageScanStyle::removeColor()
{
    if (!ui->treeWidgetColors->currentItem()) {
        return;
    }

    QTreeWidgetItem *it = ui->treeWidgetColors->takeTopLevelItem(ui->treeWidgetColors->indexOfTopLevelItem(ui->treeWidgetColors->currentItem()));
    delete it;

      for (int i = 0; i < ui->treeWidgetColors->topLevelItemCount(); ++i) {
        QTreeWidgetItem *lwi = ui->treeWidgetColors->topLevelItem(i);
        lwi->setText(0, QString("%1").arg(i+1));
    }
}

// we don't want the color of the current item to be changed, otherwise it is not possible anymore
// to recognize what color it was set to
void PrefPageScanStyle::itemSelectionChanged()
{
    if (!ui->treeWidgetColors->currentItem()) {
        return;
    }

    QPalette p = ui->treeWidgetColors->palette();
    p.setColor(QPalette::Highlight, ui->treeWidgetColors->currentItem()->background(0).color());
    ui->treeWidgetColors->setPalette(p);
}

void PrefPageScanStyle::setColorTable(const QList<QColor> &lCol)
{
    ui->treeWidgetColors->clear();

    // construct the treewidgetItems and their embedded combobox widgets
    for (int i = 0; i < lCol.size(); ++i) {
        QColor c(lCol.at(i));
        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetColors, QStringList(QString("%1").arg(i+1)));

        QBrush b(c);
        it->setBackground(0, b);

        if (c.lightness() < 100) {
            QBrush f(Qt::white);
            it->setForeground(0, f);
        }

        QComboBox *cb = new QComboBox(this);
        ui->treeWidgetColors->setItemWidget(it, 1, cb);
        cb->addItems(pointStyles);
    }
}

/*
 * call this function AFTER calling setColorTable(), the treewidget must be populated first
 */
void PrefPageScanStyle::setStyleTable(const QList<int> &l)
{
    for (int i = 0; i < ui->treeWidgetColors->topLevelItemCount(); ++i) {
        QComboBox *cb = (QComboBox*)ui->treeWidgetColors->itemWidget(ui->treeWidgetColors->topLevelItem(i), 1);

        if (!cb) continue;

        if (i < l.size()) cb->setCurrentIndex(l.at(i));
        else              cb->setCurrentIndex(0);
    }
}

QList<QColor> PrefPageScanStyle::getColorTable()
{
    QList<QColor> l;

    for (int i = 0; i < ui->treeWidgetColors->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetColors->topLevelItem(i);
        QBrush b(it->background(0));
        l.append(b.color());
    }

    return l;
}

QList<int> PrefPageScanStyle::getStyleTable()
{
    QList<int> l;

    for (int i = 0; i < ui->treeWidgetColors->topLevelItemCount(); ++i) {
        QComboBox *cb = (QComboBox*)ui->treeWidgetColors->itemWidget(ui->treeWidgetColors->topLevelItem(i), 1);

        if (cb) l.append(cb->currentIndex());
    }

    return l;
}

void PrefPageScanStyle::colorChanged(QTreeWidgetItem *it, int i)
{
    Q_UNUSED(i);
    QBrush b = it->background(0);
    QColor c = b.color();

    c = QColorDialog::getColor(c, this);

    if (!c.isValid()) {
        return;
    }

    b.setColor(c);

    it->setBackground(0, b);
    itemSelectionChanged();
}

void PrefPageScanStyle::iObsColorChanged()
{
    QColor c = QColorDialog::getColor(colIobs, this, tr("Color for I observed"));

    if (c.isValid()) colIobs = c;

    ui->pushButtonColIObs->setStyleSheet(QString("background-color:%1").arg(colIobs.name()));
}

void PrefPageScanStyle::iCalcColorChanged()
{
    QColor c = QColorDialog::getColor(colIcalc, this, tr("Color for I calculated"));

    if (c.isValid()) colIcalc = c;

    ui->pushButtonColICalc->setStyleSheet(QString("background-color:%1").arg(colIcalc.name()));
}

void PrefPageScanStyle::iDiffColorChanged()
{
    QColor c = QColorDialog::getColor(colIdiff, this, tr("Color for I difference"));

    if (c.isValid()) colIdiff = c;

    ui->pushButtonColIDiff->setStyleSheet(QString("background-color:%1").arg(colIdiff.name()));
}

void PrefPageScanStyle::iBkgrColorChanged()
{
    QColor c = QColorDialog::getColor(colIbkgr, this, tr("Color for Background"));

    if (c.isValid()) colIbkgr = c;

    ui->pushButtonColBkgr->setStyleSheet(QString("background-color:%1").arg(colIbkgr.name()));
}

void PrefPageScanStyle::resetList()
{
    if (QMessageBox::question(this,
                              tr("Reset style list"),
                              tr("Do you really want to reset the style list to default values?"),
                              QMessageBox::Yes | QMessageBox::No)
            == QMessageBox::No) return;

    settings->setColorList(QList<QColor>());
    settings->setScanStyleList(QList<int>());

    settings->setValue("graph/iobsColor", "#000000");
    settings->setValue("graph/icalcColor", "#ff0000");
    settings->setValue("graph/idiffColor", "#bbbbbb");
    settings->setValue("graph/ibkgrColor", "#0000ff");

    initSettings();
}

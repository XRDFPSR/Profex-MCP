/***************************************************************************
                          copycontrolfiledialog.cpp  -  description
                             -------------------
    begin                : Tue Sep 03 08:00:00 CEST 2013
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

#include "projectselectdialog.h"
#include "ui_projectselectdialog.h"
#include <QFileInfo>
#include <QMap>
#include <QMapIterator>
#include <QDebug>

ProjectSelectDialog::ProjectSelectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProjectSelectDialog)
{
    ui->setupUi(this);

    ui->checkBoxFollowActiveRefinement->hide();

    QStringList header;
    header << "" << "Project";
    ui->treeWidget->setColumnCount(header.size());
    ui->treeWidget->setHeaderLabels(header);
    currentUid = QUuid();
    connect(ui->buttonAll, SIGNAL(clicked()), this, SLOT(checkAll()));
    connect(ui->buttonCurrent, SIGNAL(clicked()), this, SLOT(checkCurrent()));
    connect(ui->buttonNone, SIGNAL(clicked()), this, SLOT(checkNone()));
}

ProjectSelectDialog::~ProjectSelectDialog()
{
    delete ui;
}

void ProjectSelectDialog::setCurrentUid(const QUuid &s)
{
    currentUid = s;
    qDebug() << QString("ProjectSelectDialog::setCurrentUid: Current ID set to %1").arg(currentUid.toString());

    if (currentUid.isNull()) {
        ui->buttonCurrent->setEnabled(false);
    } else {
        ui->buttonCurrent->setEnabled(true);
    }
}

void ProjectSelectDialog::setFiles(const QList<ProjectWidget *> &pwlst, const QList<QUuid> &ulst)
{
    for (int i = 0; i < pwlst.size(); ++i)  {
        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidget);
        it->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        it->setData(0, Qt::UserRole, pwlst.at(i)->uId());
        it->setText(1, pwlst.at(i)->baseName());

        if (ulst.size() == 0) {
            it->setCheckState(0, Qt::Checked);
        } else {
            if (ulst.contains(pwlst.at(i)->uId())) {
                it->setCheckState(0, Qt::Checked);
            } else {
                it->setCheckState(0, Qt::Unchecked);
            }
        }

        ui->treeWidget->addTopLevelItem(it);
    }

    ui->treeWidget->resizeColumnToContents(0);
}

QList<QUuid> ProjectSelectDialog::getChecked()
{
    QList<QUuid> out;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidget->topLevelItem(i);
        if (it->checkState(0) == Qt::Checked) {
            out << it->data(0, Qt::UserRole).toUuid();
        }
    }

    return out;
}

void ProjectSelectDialog::checkAll()
{
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidget->topLevelItem(i);
        it->setCheckState(0, Qt::Checked);
    }
}

void ProjectSelectDialog::checkCurrent()
{
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidget->topLevelItem(i);
        if (it->data(0, Qt::UserRole).toUuid() == currentUid) {
            // qDebug() << QString("ProjectSelectDialog::checkCurrent: %1 matches current ID %2").arg(it->data(0, Qt::UserRole).toString()).arg(currentUid);
            it->setCheckState(0, Qt::Checked);
        } else {
            // qDebug() << QString("ProjectSelectDialog::checkCurrent: %1 does not match current ID %2").arg(it->data(0, Qt::UserRole).toString()).arg(currentUid);
            it->setCheckState(0, Qt::Unchecked);
        }
    }
}

void ProjectSelectDialog::checkNone()
{
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidget->topLevelItem(i);
        it->setCheckState(0, Qt::Unchecked);
    }
}

void ProjectSelectDialog::showFollowActiveBox(bool b)
{
    if (b) ui->checkBoxFollowActiveRefinement->show();
}

bool ProjectSelectDialog::getFollowActiveRefinement()
{
    return ui->checkBoxFollowActiveRefinement->isChecked();
}

void ProjectSelectDialog::setFollowActiveRefinement(bool b)
{
    ui->checkBoxFollowActiveRefinement->setChecked(b);
}

/***************************************************************************
                          projecselecttreetitem.cpp  -  description
                             -------------------
    begin                : Thu Mai 11 18:00:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#include "projectselecttreeitem.h"
#include <QObject>

ProjectSelectTreeItem::ProjectSelectTreeItem(const QUuid &u, ProjectWidget *p) :
    QTreeWidgetItem(QStringList() << QString() << QString(), QTreeWidgetItem::UserType), _uid(u), _pw(p)
{
    setFlags(flags() & ~Qt::ItemIsDropEnabled);

    itemProjectType     = new QTreeWidgetItem(this, QStringList() << QObject::tr("Project Type") << QString());
    itemProjectFileName = new QTreeWidgetItem(this, QStringList() << QObject::tr("File Name") << QString());
    itemProjectFilePath = new QTreeWidgetItem(this, QStringList() << QObject::tr("File Path") << QString());
    itemProjectSampleId = new QTreeWidgetItem(this, QStringList() << QObject::tr("Sample ID") << QString());

    itemProjectType->setFlags(itemProjectType->flags()         & ~Qt::ItemIsDropEnabled);
    itemProjectFileName->setFlags(itemProjectFileName->flags() & ~Qt::ItemIsDropEnabled);
    itemProjectFilePath->setFlags(itemProjectFilePath->flags() & ~Qt::ItemIsDropEnabled);
    itemProjectSampleId->setFlags(itemProjectSampleId->flags() & ~Qt::ItemIsDropEnabled);

    _refStatus = global::RefinementStatus::IDLE;
    _refStatusText = QObject::tr("idle");
    _chi2 = -1.0;
    _curIteration = -1;
    _maxIterations = -1;
}

void ProjectSelectTreeItem::setProjectName(const QString &s)
{
    setText(0, s);
    setToolTip(0, s);
}

void ProjectSelectTreeItem::setProjectType(const QString &s)
{
    itemProjectType->setText(1, s);
    itemProjectType->setToolTip(1, s);
}

void ProjectSelectTreeItem::setProjectFileName(const QString &s)
{
    itemProjectFileName->setText(1, s);
    itemProjectFileName->setToolTip(1, s);
}

void ProjectSelectTreeItem::setProjectFilePath(const QString &s)
{
    itemProjectFilePath->setText(1, s);
    itemProjectFilePath->setToolTip(1, s);
}

void ProjectSelectTreeItem::setProjectSampleId(const QString &s)
{
    itemProjectSampleId->setText(1, s);
    itemProjectSampleId->setToolTip(1, s);
}

void ProjectSelectTreeItem::setStatus(const global::RefinementStatus &s)
{
    _refStatus = s;

    switch (_refStatus) {
    case global::RefinementStatus::IDLE:
        _refStatusText = QObject::tr("Idle");
        break;
    case global::RefinementStatus::RUNNING:
        _refStatusText = QObject::tr("Running...");
        break;
    case global::RefinementStatus::MATCHING:
        _refStatusText = QObject::tr("Matching...");
        break;
    case global::RefinementStatus::COMPLETED:
        _refStatusText = QObject::tr("Completed");
        break;
    case global::RefinementStatus::SCHEDULED:
        _refStatusText = QObject::tr("Scheduled");
        break;
    case global::RefinementStatus::ABORTED:
        _refStatusText = QObject::tr("Aborted");
        break;
    case global::RefinementStatus::FAILURE:
        _refStatusText = QObject::tr("Error");
        break;
    case global::RefinementStatus::CRASH:
        _refStatusText = QObject::tr("Crashed");
        break;
    case global::RefinementStatus::FITSCHEDULED:
        _refStatusText = QObject::tr("Fit Scheduled");
        break;
    case global::RefinementStatus::FITRUNNING:
        _refStatusText = QObject::tr("Curve fitting...");
        break;
    }

    updateStatusText();
}

void ProjectSelectTreeItem::setRefinementStats(int ci, int mi, double rwp, double rexp)
{
    if (qFuzzyIsNull(rexp)) {
        _chi2 = -1.0;
        return;
    }

    _curIteration = ci;
    _maxIterations = mi;
    _chi2 = qPow(rwp / rexp, 2.0);

    updateStatusText();
}

QString ProjectSelectTreeItem::getProjectName() const
{
    return text(0);
}

QString ProjectSelectTreeItem::getProjectType() const
{
    return itemProjectType->text(1);
}

QString ProjectSelectTreeItem::getProjectFileName() const
{
    return itemProjectFileName->text(1);
}

QString ProjectSelectTreeItem::getProjectFilePath() const
{
    return itemProjectFilePath->text(1);
}

QString ProjectSelectTreeItem::getProjectSampleId() const
{
    return itemProjectSampleId->text(1);
}

QColor ProjectSelectTreeItem::chiColor(double chival, bool light)
{
    double chiR = 5.0;
    double chiY = 2.0;
    double chiG = 1.0;
    double r = 255.0;
    double g = 255.0;

    if (chival < 0.5)       g =         255.0 *  chival         /  0.5;
    else if (chival < 1.0)  r = 255.0 - 255.0 * (chival - 0.5)  /  0.5;
    else if (chival < chiY) r =         255.0 * (chival - chiG) / (chiY - chiG);
    else if (chival < chiR) g = 255.0 - 255.0 * (chival - chiY) / (chiR - chiY);
    else                    g = 0.0;

    QColor c(int(r), int(g), 0);

    if (light) return c.lighter(175);
    return c;
}

void ProjectSelectTreeItem::updateStatusText()
{
    QString txtIter;
    if (_curIteration > 0) {
        txtIter = QString("%1").arg(_curIteration);

        if (_maxIterations > 0) {
            txtIter += QString(" of %1").arg(_maxIterations);
        }
    }

    QString txtChi2 = _chi2 < 0.0 ? QString() : QString("%1").arg(_chi2, 0, 'f', 2);

    setText(1, _refStatusText);

    /*
    if ((_refStatus == global::RefinementStatus::RUNNING) || (_refStatus == global::RefinementStatus::COMPLETED)) {
        setText(2, txtIter);
        setText(3, txtChi2);
        setBackground(3, chiColor(_chi2, false));
    } else {
        setText(2, QString());
        QPalette pal;
        setBackground(3, pal.base().color());
    }
    */

    if ((_refStatus == global::RefinementStatus::IDLE) || (_refStatus == global::RefinementStatus::SCHEDULED)) {
        setText(2, QString());
        QPalette pal;
        setBackground(3, pal.base().color());
    } else {
        setText(2, txtIter);
        setText(3, txtChi2);
        setBackground(3, chiColor(_chi2, false));
    }
}

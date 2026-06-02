/***************************************************************************
                          resultstreewidget.h  -  description
                             -------------------
    begin                : Thu June 06 21:00:00 CEST 2019
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

#include "../libXrdIO/structs.h"
#include "../libXrdIO/functions.h"
#include "bgmnresultstreewidget.h"
#include "../libXrdIO/parser/bgmnsavparser.h"
#include "../libXrdIO/parser/bgmnparparser.h"

#include <math.h> // for log10
#include <QtMath> // for other functions
#include <QStringList>
#include <QMenu>
#include <QHeaderView>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>

BgmnResultsTreeWidget::BgmnResultsTreeWidget(QWidget *parent) :
    ResultsTreeWidget(parent)
{
    QStringList h;
    h << tr("Parameter") << tr("Value") << tr("ESD");

    setColumnCount(h.count());
    setHeaderLabels(h);

    lparser = nullptr;

    itStats  = new QTreeWidgetItem(QStringList(tr("Statistics")));
    itGlobal = new QTreeWidgetItem(QStringList(tr("Global GOALs")));
    itLocal  = new QTreeWidgetItem(QStringList(tr("Local GOALs")));

    // data needed to save expanded/collapsed state in settings
    itStats->setData(0, Qt::UserRole, 0);
    itGlobal->setData(0, Qt::UserRole, 1);
    itLocal->setData(0, Qt::UserRole, 2);

    addTopLevelItem(itStats);
    addTopLevelItem(itGlobal);
    addTopLevelItem(itLocal);

    itStats->setFirstColumnSpanned(true);
    itGlobal->setFirstColumnSpanned(true);
    itLocal->setFirstColumnSpanned(true);

    itStatsRwp  = new QTreeWidgetItem(QStringList() << QString(tr("Rwp"))  << QString());
    itStatsRexp = new QTreeWidgetItem(QStringList() << QString(tr("Rexp")) << QString());
    itStatsChi2 = new QTreeWidgetItem(QStringList() << QString("%1%2").arg(global::chi).arg(global::superTwo) << QString());
    itStatsGooF = new QTreeWidgetItem(QStringList() << QString(tr("GoF")) << QString());
    itBkgrCoeff = new QTreeWidgetItem(QStringList() << QString(tr("Background Coefficients")) << QString() << QString());

    itStats->addChild(itStatsRwp);
    itStats->addChild(itStatsRexp);
    itStats->addChild(itStatsChi2);
    itStats->addChild(itStatsGooF);
    itStats->addChild(itBkgrCoeff);

    itStats->setExpanded(true);
    itGlobal->setExpanded(true);
    itLocal->setExpanded(true);

    actCopyGvalsGesds = addAction(tr("Copy global GOAL values and ESDs"), global::Functions::keyCopy(),      this, &BgmnResultsTreeWidget::copyGlobalValuesAndEsds);
    actCopyGvals      = addAction(tr("Copy global GOAL values"),          global::Functions::keyShiftCopy(), this, &BgmnResultsTreeWidget::copyGlobalValues);
    actCopyGesds      = addAction(tr("Copy global GOAL ESDs"),            global::Functions::keyAltCopy(),   this, &BgmnResultsTreeWidget::copyGlobalEsds);

    actCopyGvalsGesds->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    actCopyGvals->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    actCopyGesds->setShortcutContext(Qt::WidgetWithChildrenShortcut);

    connect(this, &QTreeWidget::itemExpanded,
            this, &BgmnResultsTreeWidget::slotItemExpanded);
    connect(this, &QTreeWidget::itemCollapsed,
            this, &BgmnResultsTreeWidget::slotItemCollapsed);
    connect(header(), &QHeaderView::sectionResized,
            this, &BgmnResultsTreeWidget::headerResized);
    connect(this, &QTreeWidget::customContextMenuRequested,
            this, &BgmnResultsTreeWidget::showContextMenu);
}

void BgmnResultsTreeWidget::initSettings()
{
    header()->restoreState(settings->value("bgmnProject/resultsTree/header", QByteArray()).toByteArray());

    // the preferences store the "show" value. Use inverse for "hide" state.
    bool hideRwp = !settings->value("bgmnProject/resultsTree/showRwp", true).toBool();
    bool hideRexp = !settings->value("bgmnProject/resultsTree/showRexp", true).toBool();
    bool hideChi2 = !settings->value("bgmnProject/resultsTree/showChi2", true).toBool();
    bool hideGooF = !settings->value("bgmnProject/resultsTree/showGof", true).toBool();
    bool hideBkgrCoeff = !settings->value("bgmnProject/resultsTree/showBkgrCoeff", true).toBool();

    int hasShown = 0;
    hasShown += hideRwp       ? 0 : 1;
    hasShown += hideRexp      ? 0 : 1;
    hasShown += hideChi2      ? 0 : 1;
    hasShown += hideGooF      ? 0 : 1;
    hasShown += hideBkgrCoeff ? 0 : 1;

    itStatsRwp->setHidden(hideRwp);
    itStatsRexp->setHidden(hideRexp);
    itStatsChi2->setHidden(hideChi2);
    itStatsGooF->setHidden(hideGooF);
    itBkgrCoeff->setHidden(hideBkgrCoeff);
    itStats->setHidden(!hasShown);
}

void BgmnResultsTreeWidget::updateGoals(const BgmnSavParser &sparser)
{
    QFileInfo fiCtrl(sparser.controlFile());

    parseSettings();
    setStats(fiCtrl.absolutePath() + "/" + sparser.outputFile());
    setGlobal();
    setLocal();
}

void BgmnResultsTreeWidget::parseSettings()
{
    QString goalsGlobal = settings->value("bgmnProject/reportedGlobalGoals", global::defaultBgmnGlobalGoals).toString();
    QString goalsLocal  = settings->value("bgmnProject/reportedLocalParameters", global::defaultBgmnLocalGoals).toString();

    // check again due to an incompatibility in Profex 5.4.0 with previous settings format (was a QStringList before)
    if (goalsGlobal.isEmpty()) goalsGlobal = global::defaultBgmnGlobalGoals;
    if (goalsLocal.isEmpty())  goalsLocal  = global::defaultBgmnLocalGoals;

    globalParams = goalsGlobal.split("\n");
    localParams  = goalsLocal.split("\n");

    static QRegularExpression rxUnit("\\^?UNIT\\$?");
    if (!localParams.contains(rxUnit)) localParams.append("^UNIT$"); // this one is important to always report
}

void BgmnResultsTreeWidget::setStats(const QString &parFile)
{
    if (!lparser) return;
    if (!lparser->hasData()) return;
    double rwp = lparser->getRwp();
    double rexp = lparser->getRexp();
    double chi2 = lparser->getChi2();
    double goof = qSqrt(chi2);

    itStatsRwp->setText(1, QString("%1").arg(rwp, 0, 'f', 2));
    itStatsRexp->setText(1, QString("%1").arg(rexp, 0, 'f', 2));
    itStatsChi2->setText(1, QString("%1").arg(chi2, 0, 'f', 4));
    itStatsGooF->setText(1, QString("%1").arg(goof, 0, 'f', 4));
    itBkgrCoeff->setText(1, QString("%1").arg(BgmnParParser::getBackgroundCoefficients(parFile).size()));

    itStats->setExpanded(settings->value("bgmnProject/resultsTree/statisticsExpanded", true).toBool());
}

void BgmnResultsTreeWidget::setGlobal()
{
    qDeleteAll(itGlobal->takeChildren());
    if (!lparser) return;
    if (!lparser->hasData()) return;

    QList<global::Result> gGoals = lparser->getGlobalGoals(globalParams);

    for (int i = 0; i < gGoals.size(); ++i) {
        double val = gGoals.at(i).value;
        double esd = gGoals.at(i).esd;
        int prec = gGoals.at(i).precision < 0 ? 4 : gGoals.at(i).precision;

        QString textValue(QString::number(val,'f', prec));
        QString textEsd(qFuzzyIsNull(esd) ? QString() : QString::number(gGoals.at(i).esd, 'f', prec));

        QTreeWidgetItem *itGlobalGoal = new QTreeWidgetItem();
        itGlobalGoal->setText(0, gGoals.at(i).name);

        if (!gGoals.at(i).error.isEmpty()) {
            itGlobalGoal->setText(1, gGoals.at(i).error);
            itGlobalGoal->setText(2, QString());
        } else {
            itGlobalGoal->setText(1, textValue);
            itGlobalGoal->setText(2, textEsd);
        }

        itGlobalGoal->setData(1, Qt::UserRole, gGoals.at(i).value);
        itGlobalGoal->setData(2, Qt::UserRole, gGoals.at(i).esd);

        if (settings->value("bgmnProject/warnShow", false).toBool()) {
            setQuantItemColor(itGlobalGoal);
        }

        itGlobal->addChild(itGlobalGoal);
    }

    itGlobal->setExpanded(settings->value("bgmnProject/resultsTree/globalGoalsExpanded", true).toBool());
}

void BgmnResultsTreeWidget::setLocal()
{
    qDeleteAll(itLocal->takeChildren());
    if (!lparser) return;
    if (!lparser->hasData()) return;

    QStringList phases(lparser->getPhaseNames());

    for (int p = 0; p < phases.size(); ++p) {
        QTreeWidgetItem *itPhase = new QTreeWidgetItem(QStringList(phases.at(p)));
        itPhase->setFirstColumnSpanned(true);
        itLocal->addChild(itPhase);

        QString sum(lparser->getSumFormula(phases.at(p)));
        QTreeWidgetItem *itForm = new QTreeWidgetItem(QStringList() << tr("Refined Composition") << sum);
        itPhase->addChild(itForm);

        QList<global::Result> lGoals = lparser->getLocalGoals(phases.at(p), localParams);

        for (int i = 0; i < lGoals.size(); ++i) {
            QTreeWidgetItem *itPar = parseLocalGoal(lGoals.at(i));
            itPhase->addChild(itPar);
        }

        itPhase->setExpanded(true);
    }

    itLocal->setExpanded(settings->value("bgmnProject/resultsTree/localGoalsExpanded", true).toBool());
}

/*
 * creates and returns a qtreewidgetitem with the text of a local goal.
 * If no data was found, it returns a nullptr.
 * If a goal with non-numerical result (e.g. "ERROR") is found, it sets
 * the text to the found string. Else it formats the numerical values correctly.
 */
QTreeWidgetItem * BgmnResultsTreeWidget::parseLocalGoal(global::Result data)
{
    static QRegularExpression rxSkipIfNull("^A$|^B$|^C$|^ALPHA$|^BETA$|^GAMMA$");
    QString textValue;
    QString textEsd;

    // check for goals with value -1.0, which is used for missing or erroneous goals
    if (data.value < 0.0) {
        if (!data.error.isEmpty()) {
            // if the result has an error string, show it
            textValue = data.error;
            textEsd = QString();
        } else if (rxSkipIfNull.match(data.name).hasMatch()) {
            // if it is one of the goals we don't show in case of error, skip it
            // (typically unit cell parameters missing due to high symmetry)
            return nullptr;
        } else {
            // fall back: show the error string stored in the result
            textValue = data.error;
            textEsd = QString();
        }
    } else {
        textValue = QString::number(data.value, 'f', data.precision);

        if (qFuzzyIsNull(data.esd)) {
            textEsd = QString();
        } else {
            textEsd = QString::number(data.esd, 'f', data.precision);
        }
    }

    QTreeWidgetItem *itParam = new QTreeWidgetItem();
    itParam->setText(0, data.name);
    itParam->setText(1, textValue);
    itParam->setText(2, textEsd);

    itParam->setData(1, Qt::UserRole, data.value);
    itParam->setData(2, Qt::UserRole, data.esd);

    return itParam;
}

void BgmnResultsTreeWidget::setQuantItemColor(QTreeWidgetItem *it)
{
    QColor colLod(settings->value("bgmnProject/warnLODColor", "#ff0000").toString());
    QColor colLoq(settings->value("bgmnProject/warnLOQColor", "#ff5500").toString());
    bool norm = settings->value("bgmnProject/quantGoals100percent", false).toBool();

    double minEsd = settings->value("bgmnProject/minESDvalue", 0.05).toDouble();
    if (!norm) minEsd *= 0.01;

    double val = it->data(1, Qt::UserRole).toDouble();
    double esd = qMax(it->data(2, Qt::UserRole).toDouble(), minEsd);

    bool warnLod = false;
    bool warnLoq = false;

    double relEsdLoq = settings->value("bgmnProject/warnLOQvalue", 0.25).toDouble();
    double relEsdLod = settings->value("bgmnProject/warnLODvalue", 0.50).toDouble();

    if (qFuzzyIsNull(val)) {
        warnLod = true;
    } else {
        double relEsd = esd / val;

        if (relEsd > relEsdLoq) {
            if (relEsd > relEsdLod) {
                warnLod = true;
            } else {
                warnLoq = true;
            }
        }
    }

    if (warnLod) {
        it->setForeground(1, QBrush(colLod));
        it->setForeground(2, QBrush(colLod));
        it->setToolTip(1, tr("< LOD"));
        it->setToolTip(2, tr("< LOD"));
    } else if (warnLoq) {
        it->setForeground(1, QBrush(colLoq));
        it->setForeground(2, QBrush(colLoq));
        it->setToolTip(1, tr("< LOQ"));
        it->setToolTip(2, tr("< LOQ"));
    }
}

void BgmnResultsTreeWidget::clearStats()
{
    if (itStatsRwp)  itStatsRwp->setText(1, QString());
    if (itStatsRexp) itStatsRexp->setText(1, QString());
    if (itStatsChi2) itStatsChi2->setText(1, QString());
    if (itStatsGooF) itStatsGooF->setText(1, QString());
}

void BgmnResultsTreeWidget::clearGlobal()
{
    if (itGlobal) qDeleteAll(itGlobal->takeChildren());
}

void BgmnResultsTreeWidget::clearLocal()
{
    if (itLocal) qDeleteAll(itLocal->takeChildren());
}

void BgmnResultsTreeWidget::resetStats()
{
    if (itStats) {
        for (int i = 0; i < itStats->childCount(); ++i) {
            itStats->child(i)->setText(1, QString());
        }
    }
}

void BgmnResultsTreeWidget::setRvalues(double _rwp, double _rexp)
{
    if (qFuzzyIsNull(_rexp)) return;
    double chi2 = qPow(_rwp / _rexp, 2.0);
    double goof = _rwp / _rexp;

    if (itStatsRwp)  itStatsRwp->setText(1, QString("%1").arg(_rwp, 0, 'f', 2));
    if (itStatsRexp) itStatsRexp->setText(1, QString("%1").arg(_rexp, 0, 'f', 2));
    if (itStatsChi2) itStatsChi2->setText(1, QString("%1").arg(chi2, 0, 'f', 2));
    if (itStatsGooF) itStatsGooF->setText(1, QString("%1").arg(goof, 0, 'f', 2));
}

void BgmnResultsTreeWidget::slotItemExpanded(QTreeWidgetItem *it)
{
    if (it->parent()) return;

    if (it->data(0, Qt::UserRole).toInt() == 0) settings->setValue("bgmnProject/resultsTree/statisticsExpanded", true);
    if (it->data(0, Qt::UserRole).toInt() == 1) settings->setValue("bgmnProject/resultsTree/globalGoalsExpanded", true);
    if (it->data(0, Qt::UserRole).toInt() == 2) settings->setValue("bgmnProject/resultsTree/localGoalsExpanded", true);
}

void BgmnResultsTreeWidget::slotItemCollapsed(QTreeWidgetItem *it)
{
    if (it->parent()) return;

    if (it->data(0, Qt::UserRole).toInt() == 0) settings->setValue("bgmnProject/resultsTree/statisticsExpanded", false);
    if (it->data(0, Qt::UserRole).toInt() == 1) settings->setValue("bgmnProject/resultsTree/globalGoalsExpanded", false);
    if (it->data(0, Qt::UserRole).toInt() == 2) settings->setValue("bgmnProject/resultsTree/localGoalsExpanded", false);
}

void BgmnResultsTreeWidget::headerResized()
{
    settings->setValue("bgmnProject/resultsTree/header", header()->saveState());
}

void BgmnResultsTreeWidget::showContextMenu(const QPoint &pos)
{
    QMenu contextMenu;
    contextMenu.addAction(actCopyGvalsGesds);
    contextMenu.addAction(actCopyGvals);
    contextMenu.addAction(actCopyGesds);

    const QPoint globalPos = viewport()->mapToGlobal(pos);
    contextMenu.exec(globalPos);
}

void BgmnResultsTreeWidget::copyGlobalValuesAndEsds()
{
    copyGlobalData(true, true);
}

void BgmnResultsTreeWidget::copyGlobalValues()
{
    copyGlobalData(true, false);
}

void BgmnResultsTreeWidget::copyGlobalEsds()
{
    copyGlobalData(false, true);
}

void BgmnResultsTreeWidget::copyGlobalData(bool v, bool e)
{
    QString s;
    QString sep = settings->value("config/clipboardFieldSeparator", ";").toString();

    for (int i = 0; i < itGlobal->childCount(); ++i) {
        QTreeWidgetItem *it = itGlobal->child(i);

        if (v && e) s += QString("%1%2%3\n").arg(it->text(1), sep, it->text(2));
        else if (v) s += QString("%1\n").arg(it->text(1));
        else if (e) s += QString("%2\n").arg(it->text(2));
    }

    QMimeData *mime = new QMimeData();
    mime->setText(s);
    qApp->clipboard()->setMimeData(mime);
}

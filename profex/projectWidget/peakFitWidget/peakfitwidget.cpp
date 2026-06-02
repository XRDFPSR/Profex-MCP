/***************************************************************************
                          peakfitwidget.cpp  -  description
                             -------------------
    begin                : Tue Nov 16 22:00:00 CEST 2019
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

#include "peakfitwidget.h"
#include "ui_peakfitwidget.h"
#include "../libXrdIO/curveFitting/curvehandler.h"
#include "../graphWidget/graphdatacontroller.h"
#include "../libXrdIO/curveFitting/linearcurve.h"
#include "../libXrdIO/curveFitting/gaussiancurve.h"
#include "../libXrdIO/curveFitting/lorentziancurve.h"
#include "../libXrdIO/curveFitting/pseudovoigtcurve.h"
#include "../libXrdIO/curveFitting/pearsoncurve.h"
#include "../libXrdIO/curveFitting/gaussiansplitcurve.h"
#include "../libXrdIO/curveFitting/lorentziansplitcurve.h"
#include "../libXrdIO/curveFitting/pseudovoigtsplitcurve.h"
#include "../libXrdIO/curveFitting/pearsonsplitcurve.h"
#include "../libXrdIO/curveFitting/quadraticcurve.h"
#include "../libXrdIO/curveFitting/cubiccurve.h"
#include "../libXrdIO/curveFitting/polynom4curve.h"

#include <QComboBox>
#include <QSpinBox>
#include <QShowEvent>
#include <QCloseEvent>
#include <QTableWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QToolButton>
#include <QList>
#include <QDebug>

PeakFitWidget::PeakFitWidget(GraphDataController *c, GraphWindow *v, QWidget *parent) :
    AbstractGraphView(c, parent),
    ui(new Ui::PeakFitWidget)
{
    graphView = v;
    ui->setupUi(this);

    vModes.insert(global::ViewUpdateMode::DISPLAY);

    wasUsed = false;
    _rangeCounter = 0;
    _curveCounter = 0;

    ui->labelActiveScan->setText(QString());

    ui->treeWidgetFunctions->setColumnCount(3);
    ui->treeWidgetFunctions->setHeaderLabels(QStringList() << tr("Parameter") << tr("Variable") << tr("Value"));

    ui->treeWidgetVariables->setColumnCount(4);
    ui->treeWidgetVariables->setHeaderLabels(QStringList() << tr("Variable") << tr("Value") << tr("Lower boundary") << tr("Upper boundary"));

    ui->buttonAddPeak->setEnabled(false);
    ui->toolButtonAutoAddFunction->setEnabled(false);
    ui->comboBoxPeakFunction->setEnabled(false);

    initSettings();

    connect(ui->toolButtonRemovePeak, SIGNAL(clicked()), this, SLOT(removeCurve()));
    connect(ui->toolButtonPinTemporary, SIGNAL(clicked()), this, SLOT(keepTemporary()));
    connect(ui->toolButtonFit, SIGNAL(clicked()), this, SLOT(fit()));
    connect(ui->toolButtonClearAll, SIGNAL(clicked()), this, SLOT(clearAll()));
    connect(ui->buttonAddPeak, SIGNAL(toggled(bool)), this, SLOT(toggleAddPeakMode(bool)));
    connect(ui->buttonAddRange, SIGNAL(toggled(bool)), this, SLOT(toggleSelectRangeMode(bool)));
    connect(ui->comboBoxPeakFunction, SIGNAL(currentIndexChanged(int)), this, SLOT(togglePeakFunction(int)));
    connect(ui->checkBoxStepSize, SIGNAL(toggled(bool)), ui->doubleSpinBoxStepSize, SLOT(setEnabled(bool)));
    connect(ui->treeWidgetFunctions, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(functionDoubleClicked(QTreeWidgetItem*,int)));
    connect(ui->treeWidgetVariables, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(variableDoubleClicked(QTreeWidgetItem*,int)));
    connect(ui->treeWidgetFunctions, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(activeCurveChanged(QTreeWidgetItem*,int)));
    connect(ui->toolButtonResetControl, SIGNAL(clicked()), this, SLOT(undoControls()));
    connect(ui->toolButtonApplyToAll, SIGNAL(clicked()), this, SIGNAL(applyToAll()));
    connect(ui->treeWidgetFunctions->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(functionsHeaderResized()));
    connect(ui->treeWidgetVariables->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(variablesHeaderResized()));
    connect(ui->checkBoxStepSize, SIGNAL(toggled(bool)), this, SLOT(stepSizeCustomToggled()));
    connect(ui->doubleSpinBoxStepSize, SIGNAL(valueChanged(double)), this, SLOT(stepSizeValueChanged()));
    connect(ui->spinBoxMaxIts, SIGNAL(valueChanged(int)), this, SLOT(itMaxChanged()));
    connect(ui->doubleSpinBoxEpsX, SIGNAL(valueChanged(double)), this, SLOT(epsxChanged()));
    connect(ui->doubleSpinBoxDiffStep, SIGNAL(valueChanged(double)), this, SLOT(diffStepChanged()));
    connect(ui->checkBoxItMax, SIGNAL(toggled(bool)), this, SLOT(controlsAutoToggled()));
    connect(ui->checkBoxConvergence, SIGNAL(toggled(bool)), this, SLOT(controlsAutoToggled()));
    connect(ui->checkBoxNumDiff, SIGNAL(toggled(bool)), this, SLOT(controlsAutoToggled()));
    connect(ui->buttonHelpDialog, SIGNAL(clicked()), this, SLOT(showHelpDialog()));
    connect(ui->toolButtonAutoAddFunction, SIGNAL(clicked()), this, SLOT(autoAddFunction()));

    if (graphView) {
        connect(graphView, SIGNAL(sigPeakPreviewPoints(QPointF,QPointF,PeakPreviewMode,QUuid,bool)),
                this,      SLOT(peakPreviewPoints(QPointF,QPointF,PeakPreviewMode,QUuid,bool)));
        connect(graphView, SIGNAL(sigRangePoints(QPointF,QPointF,QUuid)),
                this,      SLOT(rangePoints(QPointF,QPointF,QUuid)));
        connect(graphView, SIGNAL(sigCursorMessage(QString,QUuid)),
                this,      SLOT(cursorMessage(QString,QUuid)));
    }

    if (scanControl) {
        connect(scanControl, SIGNAL(dataUpdated()),
                this,        SLOT(updateDataRange()));
    }
}

PeakFitWidget::~PeakFitWidget()
{
    delete ui;
}

void PeakFitWidget::initSettings()
{
    ui->treeWidgetFunctions->header()->restoreState(settings->value("peakFitting/functionsHeader", QByteArray()).toByteArray());
    ui->treeWidgetVariables->header()->restoreState(settings->value("peakFitting/variablesHeader", QByteArray()).toByteArray());

    ui->comboBoxPeakFunction->addItem(LinearCurve::descriptionStatic(), int(PPMLIN));
    ui->comboBoxPeakFunction->addItem(GaussianCurve::descriptionStatic(), int(PPMPEAK));
    ui->comboBoxPeakFunction->addItem(LorentzianCurve::descriptionStatic(), int(PPMPEAK));
    ui->comboBoxPeakFunction->addItem(PseudoVoigtCurve::descriptionStatic(), int(PPMPEAK));
    ui->comboBoxPeakFunction->addItem(PearsonCurve::descriptionStatic(), int(PPMPEAK));

    ui->comboBoxPeakFunction->addItem(GaussianSplitCurve::descriptionStatic(), int(PPMPEAK));
    ui->comboBoxPeakFunction->addItem(LorentzianSplitCurve::descriptionStatic(), int(PPMPEAK));
    ui->comboBoxPeakFunction->addItem(PseudoVoigtSplitCurve::descriptionStatic(), int(PPMPEAK));
    ui->comboBoxPeakFunction->addItem(PearsonSplitCurve::descriptionStatic(), int(PPMPEAK));

    ui->comboBoxPeakFunction->addItem(QuadraticCurve::descriptionStatic(), int(PPMLIN));
    ui->comboBoxPeakFunction->addItem(CubicCurve::descriptionStatic(), int(PPMLIN));
    ui->comboBoxPeakFunction->addItem(Polynom4Curve::descriptionStatic(), int(PPMLIN));

    ui->checkBoxStepSize->setChecked(settings->value("peakFitting/stepSizeCustom", false).toBool());
    ui->doubleSpinBoxStepSize->setValue(settings->value("peakFitting/stepSizeValue", 0.001).toDouble());
    ui->doubleSpinBoxStepSize->setEnabled(ui->checkBoxStepSize->isChecked());

    ui->spinBoxMaxIts->setValue(settings->value("peakFitting/controls/maxIts", 200).toInt());
    ui->doubleSpinBoxEpsX->setValue(settings->value("peakFitting/controls/epsX", 0.000001).toDouble());
    ui->doubleSpinBoxDiffStep->setValue(settings->value("peakFitting/controls/diffStep", 0.0001).toDouble());

    ui->checkBoxItMax->setChecked(settings->value("peakFitting/controls/maxItsManual", true).toBool());
    ui->checkBoxConvergence->setChecked(settings->value("peakFitting/controls/epsXManual", false).toBool());
    ui->checkBoxNumDiff->setChecked(settings->value("peakFitting/controls/diffStepManual", false).toBool());

    ui->spinBoxMaxIts->setEnabled(ui->checkBoxItMax->isChecked());
    ui->doubleSpinBoxEpsX->setEnabled(ui->checkBoxConvergence->isChecked());
    ui->doubleSpinBoxDiffStep->setEnabled(ui->checkBoxNumDiff->isChecked());
}

void PeakFitWidget::updateView()
{
    if (!isShown) return;
    if (!scanControl) return;
    if (!scanControl->firstActiveScan()) return;

    ui->labelActiveScan->setText(scanControl->scanName(scanControl->firstActiveScan()));
}

bool PeakFitWidget::hasData() const
{
    return ui->treeWidgetFunctions->topLevelItemCount() > 0;
}

void PeakFitWidget::toggleSelectRangeMode(bool b)
{
    bool ignore = true;

    if (graphView) {
        if (b) ignore = graphView->rangeSelectModeActive();
        else   ignore = false;
    } else {
        ignore = true;
    }

    if (ignore) {
        bool oldState = ui->buttonAddRange->blockSignals(true);
        ui->buttonAddRange->setChecked(false);
        ui->buttonAddRange->blockSignals(oldState);
        return;
    }

    if (ui->buttonAddPeak->isChecked()) {
        uncheckAddFunctionButton();
    }

    graphView->setRangeSelectMode(b, uid);
}

void PeakFitWidget::toggleAddPeakMode(bool b)
{
    bool ignore = true;

    if (graphView) {
        if (b) ignore = (graphView->currentPeakPreviewMode() != PPMNONE);
        else   ignore = false;
    } else {
        ignore = true;
    }

    if (ignore) {
        bool oldState = ui->buttonAddPeak->blockSignals(true);
        ui->buttonAddPeak->setChecked(false);
        ui->buttonAddPeak->blockSignals(oldState);
        return;
    }

    if (ui->buttonAddRange->isChecked()) {
        uncheckSetRangeButton();
    }

    if (b) {
        graphView->setPeakPreviewMode(PeakPreviewMode(ui->comboBoxPeakFunction->currentData(Qt::UserRole).toInt()), uid);
    } else {
        graphView->setPeakPreviewMode(PPMNONE, uid);
    }
}

void PeakFitWidget::togglePeakFunction(int)
{
    if (!graphView) return;

    if (ui->buttonAddPeak->isChecked()) {
        graphView->setPeakPreviewMode(PeakPreviewMode(ui->comboBoxPeakFunction->currentData(Qt::UserRole).toInt()), uid);
    }
}

const Scan * PeakFitWidget::getActiveDataScan()
{
    QVector<Scan*> actScans = scanControl->activeScans();

    for (int i = 0; i < actScans.size(); ++i) {
        Scan::ScanTypes flags = actScans.at(i)->scanTypes();

        if (!flags.testFlag(Scan::TEMPORARY)) {
            return actScans.at(i);
        }
    }

    return scanControl->getScan(0);
}

bool PeakFitWidget::initSolver(PeakFitRange *rIt)
{
    if (!rIt) return false;

    const Scan *curScan = getActiveDataScan();

    if (!curScan) {
        qDebug() << QString("PeakFitWidget::initSolver(): No active scan found");
        return false;
    }

    int itsPerStep = ui->checkBoxItMax->isChecked() ? 0 : ui->spinBoxMaxIts->value();
    double epsX = ui->checkBoxConvergence->isChecked() ? 0.0 : ui->doubleSpinBoxEpsX->value();
    double diffStep = ui->checkBoxNumDiff->isChecked() ? 0.0001 : ui->doubleSpinBoxDiffStep->value();

    QStringList     var;
    QVector<double> val;
    QVector<double> loLim;
    QVector<double> upLim;

    readInitialValues(rIt, var, val, loLim, upLim);

    rIt->setControls(itsPerStep, epsX, diffStep);
    rIt->setScan(curScan);
    rIt->setVariableValues(var, val, loLim, upLim);

    return true;
}

void PeakFitWidget::fit()
{
    QThreadPool *pool = QThreadPool::globalInstance();
    startFit(pool);
}

void PeakFitWidget::startFit(QThreadPool *pool)
{
    m_fitTasks.clear();
    emit clearProtocol();
    wasUsed = true;

    int nRanges = ui->treeWidgetFunctions->topLevelItemCount();
    if (nRanges == 0) return;

    qApp->setOverrideCursor(Qt::WaitCursor);
    uncheckAddFunctionButton();
    setStartButtonToAbort(true);

    remainingTasks = nRanges; // Track remaining tasks
    completedTasks = 0; // Track completed tasks

    for (int n = 0; n < nRanges; ++n) {
        emit protocol(QString("Running curve fit for range %1 of %2...").arg(n + 1).arg(nRanges));

        PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(ui->treeWidgetFunctions->topLevelItem(n));
        if (!rIt) continue;

        if (!initSolver(rIt)) {
            emit protocol(QString("    Initializing solver for range %1 failed. Skipping.").arg(n + 1));
            --remainingTasks;
            continue;
        }

        // Create and start the FitTask in QThreadPool
        FitTask *task = new FitTask(rIt);
        connect(task, &FitTask::fitCompleted, this, &PeakFitWidget::onFitTaskCompleted);
        m_fitTasks << task;
        emit statusChanged(global::RefinementStatus::FITRUNNING);
        pool->start(task);
    }
}

// Callback when a FitTask finishes
void PeakFitWidget::onFitTaskCompleted(PeakFitRange *rIt, QMap<QString, QVariant> report, QVector<double> fitValues)
{
    FitTask *me = qobject_cast<FitTask*>(sender());
    m_fitTasks.removeAll(me);
    me->deleteLater();

    ++completedTasks;

    if (fitValues.isEmpty()) {
        clearTemporary();
    } else {
        writeFittedValues(rIt, fitValues);
        updateCurves();
    }

    int exitCode = report.value("ExitCode").toInt();

    if (exitCode == 5) {
        qApp->restoreOverrideCursor();
        if (QMessageBox::question(this, tr("Maximum iterations reached"),
                                  tr("The fit has not converged. Do you want to run it again?"))
            == QMessageBox::Yes) {
            fit();
            return;
        }
    }

    emit protocol(getProtocolOutput(rIt, report, fitValues));

    // Final cleanup when all tasks are completed
    if (completedTasks == remainingTasks) {
        qApp->restoreOverrideCursor();
        setStartButtonToAbort(false);
        emit fitCompleted(global::RefinementStatus::COMPLETED);
    }
}

void PeakFitWidget::abortFit()
{
    qApp->restoreOverrideCursor();
    emit protocol(">>> Aborting all curve fits...");

    // first, pull out any tasks still queued
    int cancelled = 0;

    for (FitTask *t : m_fitTasks) {
        if (QThreadPool::globalInstance()->tryTake(t)) {
            // we removed it before it started
            cancelled++;
            delete t;
            emit protocol("    Cancelled queued task.");
        } else {
            // already running → tell it to stop
            t->cancel();
        }
    }

    // adjust how many fits we still expect to finish
    remainingTasks -= cancelled;

    if (remainingTasks == completedTasks) {
        // nothing left
        emit fitCompleted(global::RefinementStatus::ABORTED);
    }
}

QString PeakFitWidget::getProtocolOutput(const PeakFitRange *rIt, const QMap<QString, QVariant> &report, const QVector<double> &fitValues)
{
    if (fitValues.isEmpty()) {
        return QString("\n%1: Convergence error (Exit code %2: %3)")
                    .arg(report.value("RangeName").toString())
                    .arg(report.value("ExitCode").toInt())
                    .arg(report.value("ExitString").toString());
    }

    const int varLength = 9;
    const int valLength = 14;
    const int decimals = 6;

    QString out;
    QStringList var = report.value("VariableNames").toStringList();
    QList<QVariant> stddev = report.value("StdDevs").toList();
    QSet<QString> usedVariables = rIt->getUsedVariableNames();

    out += QString("\n%1: %2\n").arg(report.value("RangeName").toString(),
                                     report.value("ExitString").toString());

    out += QString("R^2 = %1\n").arg(report.value("R2", 0.0).toDouble(), 0, 'f', decimals);

    // fitted variables table
    out += QString("\nFitted variables\n");

    QString fvHeader = QString("%1 %2 %3\n").arg("Variable", varLength).arg("Value", valLength).arg("ESD", valLength);
    QString fvSeparator = QString("%1\n").arg(QString(), fvHeader.length(), '-');

    out += fvSeparator + fvHeader + fvSeparator;

    for (int i = 0; i < qMin(var.size(), qMin(fitValues.size(), stddev.size())); ++i) {
        if (!usedVariables.contains(var.at(i))) continue;
        out += QString("%1 %2 %3\n")
                      .arg(var.at(i), varLength)
                      .arg(fitValues.at(i), valLength, 'f', decimals)
                      .arg(stddev.at(i).toDouble(), valLength, 'f', decimals);
    }

    out += fvSeparator;

    // correlation matrix
    out += QString("\nPearson correlation matrix\n");
    QVariantList corMatrix = report.value("CorrMatrix").toList();

    QStringList cmHeaderList(QString("%1 |").arg(QString(), varLength, ' '));

    for (int i = 0; i < var.size(); ++i) {
        if (!usedVariables.contains(var.at(i))) continue;
        cmHeaderList += QString("%1").arg(var.at(i), varLength);
    }

    QString cmHeader = cmHeaderList.join(" ") + "\n";
    QString cmSeparator = QString("%1\n").arg(QString(), cmHeader.length(), '-');

    out += cmSeparator + cmHeader + cmSeparator;

    for (int r = 0; r < qMin(var.size(), corMatrix.size()); ++r) {
        if (!usedVariables.contains(var.at(r))) continue;
        QVariantList line = corMatrix.at(r).toList();
        QStringList lineTxt(QString("%1 |").arg(var.at(r), varLength));

        for (int c = 0; c < qMin(var.size(), line.size()); ++c) {
            if (!usedVariables.contains(var.at(c))) continue;
            lineTxt.append(QString("%1").arg(line.at(c).toDouble(), varLength, 'f', decimals));
        }

        out += lineTxt.join(" ") + "\n";
    }

    out += cmSeparator;
    return out;
}

void PeakFitWidget::setStartButtonToAbort(bool b)
{
    if (b) {
        disconnect(ui->toolButtonFit, SIGNAL(clicked()), this, SLOT(fit()));
        connect(ui->toolButtonFit, SIGNAL(clicked()), this, SLOT(abortFit()));
        ui->toolButtonFit->setIcon(QIcon::fromTheme("profex-run-abort"));
    } else {
        disconnect(ui->toolButtonFit, SIGNAL(clicked()), this, SLOT(abortFit()));
        connect(ui->toolButtonFit, SIGNAL(clicked()), this, SLOT(fit()));
        ui->toolButtonFit->setIcon(QIcon::fromTheme("profex-run"));
    }
}

void PeakFitWidget::updateCurves()
{
    clearTemporary();

    for (int n = 0; n < ui->treeWidgetFunctions->topLevelItemCount(); ++n) {
        PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(ui->treeWidgetFunctions->topLevelItem(n));
        if (!rIt) continue;

        QMap<QString, global::CurveFitVariable> vars = getVariablesByName(rIt);
        rIt->updateCurveVariables(vars);

        if (!initSolver(rIt)) continue;

        QList<Scan> curveFits = rIt->getCalculatedCurves(ui->checkBoxStepSize->isChecked() ? ui->doubleSpinBoxStepSize->value() : -1.0);
        rIt->updateAreas();

        for (int i = 0; i < curveFits.size(); ++i) {
            scanControl->appendScan(curveFits.at(i), true);
        }
    }

    scanControl->updateViews(vModes);
}

QPointF PeakFitWidget::uncorrectedPoint(const QPointF &pt)
{
    double eps1 = scanControl->getAngularCorrEPS1();
    double eps2 = scanControl->getAngularCorrEPS2();
    double eps3 = scanControl->getAngularCorrEPS3();

    return QPointF(pt.x() + global::Functions::angularCorrection(pt.x(), eps1, eps2, eps3), pt.y());
}

void PeakFitWidget::rangePoints(QPointF pA, QPointF pB, QUuid u)
{
    if (u != uid) return;
    bool oldState = ui->buttonAddRange->blockSignals(true);
    ui->buttonAddRange->setChecked(false);
    ui->buttonAddRange->blockSignals(oldState);
    graphView->setRangeSelectMode(false, uid);

    ++_rangeCounter;
    QString rName = QString("Range %1").arg(_rangeCounter);

    PeakFitRange *rangeIt = new PeakFitRange(QUuid::createUuid());
    rangeIt->setName(rName);
    rangeIt->setRangeNumber(_rangeCounter);
    rangeIt->setLowerLimit(uncorrectedPoint(pA).x());
    rangeIt->setUpperLimit(uncorrectedPoint(pB).x());
    ui->treeWidgetFunctions->addTopLevelItem(rangeIt);
    rangeIt->setExpanded(true);
    ui->treeWidgetFunctions->setCurrentItem(rangeIt);

    PeakFitItem *rangeVarIt = new PeakFitItem(rangeIt->uid());
    rangeVarIt->setName(rName);
    ui->treeWidgetVariables->addTopLevelItem(rangeVarIt);
    rangeVarIt->setExpanded(true);

    ui->buttonAddPeak->setEnabled(true);
    ui->toolButtonAutoAddFunction->setEnabled(true);
    ui->comboBoxPeakFunction->setEnabled(true);
    ui->comboBoxPeakFunction->setCurrentIndex(0);
    setGraphHiglightRanges();
}

void PeakFitWidget::setGraphHiglightRanges()
{
    QList<global::HighlightRegion> regions;
    QColor colRegion(settings->value("graph/peakFitRangeColor", QString("#deecf4")).toString());

    for (int i = 0; i < ui->treeWidgetFunctions->topLevelItemCount(); ++i) {
        PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(ui->treeWidgetFunctions->topLevelItem(i));
        if (!rIt) continue;

        global::HighlightRegion region(rIt->name(),
                                       QPointF(rIt->lowerLimit(), 1.0),
                                       QPointF(rIt->upperLimit(), 1.0),
                                       false,
                                       colRegion,
                                       QPen(QColor()));
        regions.append(region);
    }

    graphView->setPeakFitRanges(regions);
}

void PeakFitWidget::peakPreviewPoints(QPointF pA, QPointF pB, PeakPreviewMode mode, QUuid u, bool applyAngCorrections)
{
    Q_UNUSED(mode);

    if (u != uid) return;
    if (qFuzzyCompare(pA.x(), pB.x())) return;

    QPointF pAc = applyAngCorrections ? uncorrectedPoint(pA) : pA;
    QPointF pBc = applyAngCorrections ? uncorrectedPoint(pB) : pB;

    double slope = (pBc.y() - pAc.y()) / (pBc.x() - pAc.x());
    double xoff = pAc.y() - pAc.x() * slope;

    double mu     = pAc.x();
    double sigma  = qAbs(pAc.x() - pBc.x()) * 0.5;
    double intens = qAbs(pAc.y() - pBc.y());

    if (ui->comboBoxPeakFunction->currentText() == LinearCurve::descriptionStatic())
        addCurve(std::make_shared<LinearCurve>(),           QList<double>() << xoff << slope);

    if (ui->comboBoxPeakFunction->currentText() == GaussianCurve::descriptionStatic())
        addCurve(std::make_shared<GaussianCurve>(),         QList<double>() << mu << sigma << intens);

    if (ui->comboBoxPeakFunction->currentText() == LorentzianCurve::descriptionStatic())
        addCurve(std::make_shared<LorentzianCurve>(),       QList<double>() << mu << sigma << intens);

    if (ui->comboBoxPeakFunction->currentText() == PseudoVoigtCurve::descriptionStatic())
        addCurve(std::make_shared<PseudoVoigtCurve>(),      QList<double>() << mu << sigma << intens << 0.5);

    if (ui->comboBoxPeakFunction->currentText() == PearsonCurve::descriptionStatic())
        addCurve(std::make_shared<PearsonCurve>(),          QList<double>() << mu << sigma << intens << 2.0);

    if (ui->comboBoxPeakFunction->currentText() == GaussianSplitCurve::descriptionStatic())
        addCurve(std::make_shared<GaussianSplitCurve>(),    QList<double>() << mu << sigma << sigma << intens);

    if (ui->comboBoxPeakFunction->currentText() == LorentzianSplitCurve::descriptionStatic())
        addCurve(std::make_shared<LorentzianSplitCurve>(),  QList<double>() << mu << sigma << sigma << intens);

    if (ui->comboBoxPeakFunction->currentText() == PseudoVoigtSplitCurve::descriptionStatic())
        addCurve(std::make_shared<PseudoVoigtSplitCurve>(), QList<double>() << mu << sigma << sigma << intens << 0.5 << 0.5);

    if (ui->comboBoxPeakFunction->currentText() == PearsonSplitCurve::descriptionStatic())
        addCurve(std::make_shared<PearsonSplitCurve>(),     QList<double>() << mu << sigma << sigma << intens << 2.0 << 2.0);

    if (ui->comboBoxPeakFunction->currentText() == QuadraticCurve::descriptionStatic())
        addCurve(std::make_shared<QuadraticCurve>(),        QList<double>() << xoff << slope << 0.0);

    if (ui->comboBoxPeakFunction->currentText() == CubicCurve::descriptionStatic())
        addCurve(std::make_shared<CubicCurve>(),            QList<double>() << xoff << slope << 0.0 << 0.0);

    if (ui->comboBoxPeakFunction->currentText() == Polynom4Curve::descriptionStatic())
        addCurve(std::make_shared<Polynom4Curve>(),         QList<double>() << xoff << slope << 0.0 << 0.0 << 0.0);

    updateCurves();
}

void PeakFitWidget::readInitialValues(PeakFitRange *rIt, QStringList &var, QVector<double> &val, QVector<double> &loLim, QVector<double> &upLim)
{
    QTreeWidgetItem *rvIt = getVariableRangeItem(rIt);
    if (!rvIt) return;

    QMap<QString, int>  idxMap;

    for (int i = 0; i < rvIt->childCount(); ++i) {
        PeakFitVariable *it = dynamic_cast<PeakFitVariable*>(rvIt->child(i));
        if (!it) continue;

        bool doFit = it->isChecked();
        double startVal = it->value();

        idxMap[it->variableName()] = i;
        var << it->variableName();
        val << startVal;

        if (doFit) {
            if (it->lowerLimitT() == "-inf") loLim << std::numeric_limits<double>::lowest();
            else                             loLim << it->lowerLimitT().toDouble();

            if (it->upperLimitT() == "+inf") upLim << std::numeric_limits<double>::max();
            else                             upLim << it->upperLimitT().toDouble();
        } else {
            loLim << startVal;
            upLim << startVal;
        }
    }

    for (int j = 0; j < rIt->curveCount(); ++j) {
        QStringList lVarNames = rIt->getVariableNames(j);
        QVector<int> cidx;

        for (int k = 0; k < lVarNames.size(); ++k) {
            cidx << idxMap.value(lVarNames.at(k));
        }

        auto cv = rIt->curve(j);
        if (cv) cv->setIndices(cidx);
        else    qDebug() << QString("PeakFitWidget::readInitialValues(): Curve object of curve %1 is invalid").arg(j);
    }
}

void PeakFitWidget::writeFittedValues(PeakFitRange *rIt, const QVector<double> &val)
{
    QTreeWidgetItem *rvIt = getVariableRangeItem(rIt);
    if (!rvIt) return;

    for (int i = 0; i < qMin(val.size(), rvIt->childCount()); ++i) {
        PeakFitVariable *it = dynamic_cast<PeakFitVariable*>(rvIt->child(i));
        if (it) it->setValue(val.at(i));
    }
}

void PeakFitWidget::removeCurve()
{
    QTreeWidgetItem *curIt = ui->treeWidgetFunctions->currentItem();
    if (!curIt) return;

    int depth = itemDepth(curIt);

    if (depth == 0) { // delete the entire range and the range's variable items
        PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(ui->treeWidgetFunctions->currentItem());

        int vIdx = ui->treeWidgetVariables->indexOfTopLevelItem(getVariableRangeItem(rIt));
        int fIdx = ui->treeWidgetFunctions->indexOfTopLevelItem(curIt);

        QTreeWidgetItem *vDelIt = ui->treeWidgetVariables->takeTopLevelItem(vIdx);
        QTreeWidgetItem *rDelIt = ui->treeWidgetFunctions->takeTopLevelItem(fIdx);

        if (vDelIt) delete vDelIt;
        if (rDelIt) delete rDelIt;

        if (ui->treeWidgetFunctions->topLevelItemCount() == 0) {
            ui->buttonAddPeak->setEnabled(false);
            ui->toolButtonAutoAddFunction->setEnabled(false);
            ui->comboBoxPeakFunction->setEnabled(false);
        }
    } else { // delete the current curve
        PeakFitCurve *cIt = depth > 1 ? dynamic_cast<PeakFitCurve*>(curIt->parent()) : dynamic_cast<PeakFitCurve*>(curIt);
        PeakFitRange *parIt = dynamic_cast<PeakFitRange*>(cIt->parent());

        if (cIt && parIt) {
            int cIdx = parIt->indexOfChild(cIt);
            QUuid cUid = cIt->uid();
            if (cIdx > 1) { // childs no 0 and 1 of ranges are the start and end values
                cIt = nullptr; // disable the pointer before deleting the item
                QTreeWidgetItem *cDelIt = parIt->takeChild(cIdx);
                if (cDelIt) delete cDelIt;
            }

            PeakFitItem *varIt = getVariableRangeItem(parIt);

            if (varIt) { // loop over the variable items and delete those with the same uid as the deletec curve
                for (int i = varIt->childCount() - 1; i >= 0; --i) {
                    PeakFitVariable *vIt = dynamic_cast<PeakFitVariable*>(varIt->child(i));
                    if (!vIt) continue;
                    if (vIt->uid() == cUid) {
                        vIt = nullptr; // disable the pointer before deleting the item
                        QTreeWidgetItem *vDelIt = varIt->takeChild(i);
                        if (vDelIt) delete vDelIt;
                    }
                }
            }
        }
    }

    updateCurves();
    setGraphHiglightRanges();
}

int PeakFitWidget::itemDepth(QTreeWidgetItem *it)
{
    if (!it) return -1;
    QTreeWidgetItem *parIt = it;
    int d = 0;

    while (parIt->parent()) {
        ++d;
        parIt = parIt->parent();
    }

    return d;
}

void PeakFitWidget::append()
{
    keepTemporary();
}

void PeakFitWidget::keepTemporary()
{
    scanControl->keepTemporary();
    scanControl->updateViews(vModes);
}

void PeakFitWidget::clearTemporary()
{
    scanControl->clearTemporary();
    scanControl->updateViews(vModes);
}

void PeakFitWidget::addCurve(std::shared_ptr<GenericCurve> curve, const QList<double> &values)
{
    const Scan *curScan = getActiveDataScan();
    PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(getCurrentRangeItem());

    if (!curScan || !rIt) {
        return;
    }

    QTreeWidgetItem *rvIt = getVariableRangeItem(rIt);

    if (!rvIt) {
        return;
    }

    double minang = rIt->lowerLimit();
    double maxang = rIt->upperLimit();
    double minss = curScan->stepSize();

    int m = 1;
    ++_curveCounter;

    curve->setDisplayName(QString(tr("#%1 %2")).arg(_curveCounter).arg(curve->description()));
    PeakFitCurve *curveIt = new PeakFitCurve(curve->uid());
    curveIt->setCurve(curve);
    curveIt->setCurveType(curve->type());
    curveIt->setName(curve->displayName());

    QList<double> defaultValues = curve->defaultValues();
    QStringList lowerLimits = curve->lowerLimits(minang, maxang, minss);
    QStringList upperLimits = curve->upperLimits(minang, maxang, minss);
    QStringList valNames = curve->parameterNames();

    for (int i = 0; i < curve->nParameters(); ++i) {
        QString varName = QString("$%1-%2-%3").arg(rIt->rangeNumber()).arg(_curveCounter).arg(m);
        double val = i < values.count() ? values.at(i) : defaultValues.at(i);

        PeakFitVariable *varIt = new PeakFitVariable(curve->uid());
        varIt->setName(varName);
        varIt->setValue(val, lowerLimits.at(i), upperLimits.at(i));
        varIt->setChecked(true);

        PeakFitParameter *paramIt = new PeakFitParameter(curve->uid());
        paramIt->setName(valNames.at(i));
        paramIt->setVariableName(varName);
        paramIt->setValue(val);

        curveIt->addChild(paramIt);
        rvIt->addChild(varIt);

        ++m;
    }

    if (curve->hasArea()) appendAreaItem(curveIt);

    rIt->addChild(curveIt);
    curveIt->setExpanded(true);
}

void PeakFitWidget::appendAreaItem(PeakFitCurve *curveIt)
{
    if (!curveIt) return;

    PeakFitParameter *aIt = new PeakFitParameter(curveIt->uid());
    aIt->setName(tr("Area"));
    aIt->setAreaItem(true);

    curveIt->addChild(aIt);
}

QTreeWidgetItem * PeakFitWidget::getCurrentRangeItem() const
{
    if (!ui->treeWidgetFunctions->topLevelItemCount()) return nullptr;

    QTreeWidgetItem *it = ui->treeWidgetFunctions->currentItem();
    if (!it) it = ui->treeWidgetFunctions->topLevelItem(ui->treeWidgetFunctions->topLevelItemCount() - 1);
    if (!it) return nullptr;

    while (it->parent()) {
        it = it->parent();
    }

    return it;
}

void PeakFitWidget::functionDoubleClicked(QTreeWidgetItem *it, int)
{
    int depth = itemDepth(it);
    if (depth < 1) return; // no actions for -1 (invalid pointer) or 0 (range item)

    if (depth == 1) editRangeBoundaryItem(it); // only to edit the range start and end items
    if (depth >= 2) editCurveParameterItem(it);
}

void PeakFitWidget::variableDoubleClicked(QTreeWidgetItem *it, int col)
{
    if (col == 0) return;

    QString txt = it->text(col);
    bool ok;
    txt = QInputDialog::getText(this, tr("Enter value"), tr("Value"), QLineEdit::Normal, txt, &ok);

    if (!ok) return;
    it->setText(col, txt);
    updateCurves();
}

void PeakFitWidget::editRangeBoundaryItem(QTreeWidgetItem *it)
{
    if (!scanControl) return;

    PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(it->parent());
    if (!rIt) return;

    int c = rIt->indexOfChild(it);
    if ((c < 0) || (c > 1)) return;

    double minScan = scanControl->firstActiveScan()->minAngle();
    double maxScan = scanControl->firstActiveScan()->maxAngle();
    double minRange = rIt->lowerLimit();
    double maxRange = rIt->upperLimit();

    bool ok = false;

    if (c == 0) {
        double val = QInputDialog::getDouble(this, tr("Range"), tr("Enter range start angle"), minRange, minScan, maxRange, 4, &ok);
        if (!ok) return;
        rIt->setLowerLimit(val);
    } else if (c == 1) {
        double val = QInputDialog::getDouble(this, tr("Range"), tr("Enter range end angle"), maxRange, minRange, maxScan, 4, &ok);
        if (!ok) return;
        rIt->setUpperLimit(val);
    }

    updateCurves();
    setGraphHiglightRanges();
}

void PeakFitWidget::editCurveParameterItem(QTreeWidgetItem *it)
{
    PeakFitParameter *pIt = dynamic_cast<PeakFitParameter*>(it);
    if (!pIt) return;

    PeakFitCurve *cIt = dynamic_cast<PeakFitCurve*>(pIt->parent());
    if (!cIt) return;

    PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(cIt->parent());
    if (!rIt) return;

    QString curVar = pIt->variableName();
    QStringList varList = getAllRangeVariableNames(rIt);

    bool ok = false;
    QString newVar = QInputDialog::getItem(this, tr("Function parameter"), tr("Select variable"), varList, varList.indexOf(curVar), false, &ok);
    if (!ok) return;
    pIt->setVariableName(newVar);

    updateCurves();
}

QStringList PeakFitWidget::getAllRangeVariableNames(PeakFitRange *rIt) const
{
    PeakFitItem *vIt = getVariableRangeItem(rIt);
    if (!vIt) return QStringList();

    QStringList vars;
    for (int i = 0; i < vIt->childCount(); ++i) {
        PeakFitVariable *varIt = dynamic_cast<PeakFitVariable*>(vIt->child(i));
        if (varIt) vars.append(varIt->variableName());
    }

    return vars;
}

void PeakFitWidget::clearAll()
{
    if (QMessageBox::question(this, tr("Clear all curves"), tr("Do you want to delete all curves and variables?")) == QMessageBox::No) return;
    clearAllRanges();
}

void PeakFitWidget::clearAllRanges()
{
    bool oldState = ui->treeWidgetFunctions->blockSignals(true);
    ui->treeWidgetFunctions->clear();
    ui->treeWidgetFunctions->blockSignals(oldState);
    graphView->setPeakFitRanges(QList<global::HighlightRegion>());

    oldState = ui->treeWidgetVariables->blockSignals(true);
    ui->treeWidgetVariables->clear();
    ui->treeWidgetVariables->blockSignals(oldState);

    _rangeCounter = 0;
    _curveCounter = 0;

    ui->buttonAddPeak->setEnabled(false);
    ui->toolButtonAutoAddFunction->setEnabled(false);
    ui->comboBoxPeakFunction->setEnabled(false);

    clearTemporary();
}

void PeakFitWidget::undoControls()
{
    ui->spinBoxMaxIts->setValue(0);
    ui->doubleSpinBoxEpsX->setValue(0.0);
    ui->doubleSpinBoxDiffStep->setValue(0.0001);
}

PeakFitItem * PeakFitWidget::getVariableRangeItem(PeakFitRange *rIt) const
{
    for (int i = 0; i < ui->treeWidgetVariables->topLevelItemCount(); ++i) {
        PeakFitItem *vIt = dynamic_cast<PeakFitItem*>(ui->treeWidgetVariables->topLevelItem(i));
        if (!vIt) continue;
        if (vIt->uid() == rIt->uid()) return vIt;
    }

    return nullptr;
}

QMap<QString, global::CurveFitVariable> PeakFitWidget::getVariablesByName(PeakFitRange *rIt)
{
    PeakFitItem *vrIt = getVariableRangeItem(rIt);
    QMap<QString, global::CurveFitVariable> m;
    if (!vrIt) return m;

    for (int i = 0; i < vrIt->childCount(); ++i) {
        PeakFitVariable *tlIt = dynamic_cast<PeakFitVariable*>(vrIt->child(i));

        global::CurveFitVariable cv;

        cv.name       = tlIt->variableName();
        cv.value      = tlIt->valueT();
        cv.lowerLimit = tlIt->lowerLimitT();
        cv.upperLimit = tlIt->upperLimitT();
        cv.checkState = tlIt->isCheckedT();

        m[cv.name] = cv;
    }

    return m;
}

void PeakFitWidget::getPreset(QDomDocument &doc)
{
    if (!ui->treeWidgetFunctions->topLevelItemCount()) return;

    QDomElement cfEl = doc.createElement("curveFit");
    cfEl.setAttribute("APIversion", "050100");
    doc.documentElement().appendChild(cfEl);

    QDomElement elCfStepSize = doc.createElement("curveStepSize");
    QDomElement elCfControls = doc.createElement("curveControls");

    elCfStepSize.setAttribute("StepSizeValue",  QString("%1").arg(ui->doubleSpinBoxStepSize->value()));
    elCfStepSize.setAttribute("StepSizeCustom", QString("%1").arg(ui->checkBoxStepSize->isChecked() ? "1" : "0"));

    elCfControls.setAttribute("MaxInt",   QString("%1").arg(ui->spinBoxMaxIts->value()));
    elCfControls.setAttribute("EpsX",     QString("%1").arg(ui->doubleSpinBoxEpsX->value()));
    elCfControls.setAttribute("DiffStep", QString("%1").arg(ui->doubleSpinBoxDiffStep->value()));
    elCfControls.setAttribute("MaxItManual",    ui->checkBoxItMax->isChecked()       ? "true" : "false");
    elCfControls.setAttribute("EpsXManual",     ui->checkBoxConvergence->isChecked() ? "true" : "false");
    elCfControls.setAttribute("DiffStepManual", ui->checkBoxNumDiff->isChecked()     ? "true" : "false");

    cfEl.appendChild(elCfStepSize);
    cfEl.appendChild(elCfControls);

    for (int r = 0; r < ui->treeWidgetFunctions->topLevelItemCount(); ++r) {
        PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(ui->treeWidgetFunctions->topLevelItem(r));
        if (!rIt) continue;

        QDomElement elRange = doc.createElement("curveRange");
        elRange.setAttribute("RangeStart",  rIt->lowerLimit());
        elRange.setAttribute("RangeEnd",    rIt->upperLimit());
        elRange.setAttribute("RangeNumber", rIt->rangeNumber());
        elRange.setAttribute("RangeName",   rIt->text(0));
        elRange.setAttribute("UID",         rIt->uid().toString());

        PeakFitItem *vrIt = getVariableRangeItem(rIt);

        if (vrIt) {
            for (int v = 0; v < vrIt->childCount(); ++v) {
                PeakFitVariable *vIt = dynamic_cast<PeakFitVariable*>(vrIt->child(v));
                if (!vIt) continue;

                QDomElement elVariable = doc.createElement("curveVariable");
                elVariable.setAttribute("Name",       vIt->variableName());
                elVariable.setAttribute("LowerLimit", vIt->lowerLimitT());
                elVariable.setAttribute("UpperLimit", vIt->upperLimitT());
                elVariable.setAttribute("CheckState", vIt->isCheckedT());
                elVariable.setAttribute("Value",      vIt->valueT());
                elVariable.setAttribute("UID",        vIt->uid().toString());
                elRange.appendChild(elVariable);
            }
        }

        for (int c = 2; c < rIt->childCount(); ++c) {
            PeakFitCurve *cIt = dynamic_cast<PeakFitCurve*>(rIt->child(c));
            if (!cIt) continue;

            QDomElement elCurve = doc.createElement("curveFunction");
            elCurve.setAttribute("Name", cIt->name());
            elCurve.setAttribute("Type", cIt->curveType());
            elCurve.setAttribute("Description", cIt->getCurve()->description());
            elCurve.setAttribute("UID", cIt->uid().toString());

            QDomElement elPnm = doc.createElement("ParameterNames");
            QDomElement elVar = doc.createElement("Variables");

            QDomText txPnm = doc.createTextNode(cIt->getParameterNames().join(";"));
            QDomText txVar = doc.createTextNode(cIt->getVariableNames().join(";"));

            elPnm.appendChild(txPnm);
            elVar.appendChild(txVar);

            elCurve.appendChild(elPnm);
            elCurve.appendChild(elVar);
            elRange.appendChild(elCurve);
        }

        cfEl.appendChild(elRange);
    }
}

void PeakFitWidget::applyPreset(const QDomElement &rootEl)
{
    clearAllRanges();

    int apiVersion = rootEl.attribute("APIversion", "0").toInt();

    if (apiVersion < 050100) {
        applyPresetControls050100(rootEl);
        applyPreset050000(rootEl);
    } else {
        applyPresetControls050100(rootEl);
        applyPreset050100(rootEl);
    }
}

void PeakFitWidget::applyPresetControls050100(const QDomElement &rootEl)
{
    QDomNodeList lStepSize = rootEl.elementsByTagName("curveStepSize");
    if (lStepSize.size()) {
        QDomElement el = lStepSize.at(0).toElement();

        bool osSBStepSize = ui->doubleSpinBoxStepSize->blockSignals(true);
        bool osCBStepSize = ui->checkBoxStepSize->blockSignals(true);

        ui->doubleSpinBoxStepSize->setValue(el.attribute("StepSizeValue", "0.001").toDouble());
        ui->checkBoxStepSize->setChecked(el.attribute("StepSizeCustom", "0") == "1" ? true : false);

        ui->doubleSpinBoxStepSize->blockSignals(osSBStepSize);
        ui->checkBoxStepSize->blockSignals(osCBStepSize);
    }

    QDomNodeList lControls = rootEl.elementsByTagName("curveControls");
    if (lControls.size()) {
        QDomElement el = lStepSize.at(0).toElement();

        bool osSBMaxIts   = ui->spinBoxMaxIts->blockSignals(true);
        bool osSBEpsX     = ui->doubleSpinBoxEpsX->blockSignals(true);
        bool osSBDiffSTep = ui->doubleSpinBoxDiffStep->blockSignals(true);
        bool osCBMaxIts   = ui->checkBoxItMax->blockSignals(true);
        bool osCBEpsX     = ui->checkBoxConvergence->blockSignals(true);
        bool osCBDiffSTep = ui->checkBoxNumDiff->blockSignals(true);

        ui->spinBoxMaxIts->setValue(el.attribute("MaxInt",           "200").toInt());
        ui->doubleSpinBoxEpsX->setValue(el.attribute("EpsX",         "1e-06").toDouble());
        ui->doubleSpinBoxDiffStep->setValue(el.attribute("DiffStep", "0.0001").toDouble());

        ui->checkBoxItMax->setChecked(el.attribute("MaxItManual",      "true") == QString("true"));
        ui->checkBoxConvergence->setChecked(el.attribute("EpsXManual", "true") == QString("true"));
        ui->checkBoxNumDiff->setChecked(el.attribute("DiffStepManual", "true") == QString("true"));

        ui->spinBoxMaxIts->setEnabled(ui->checkBoxItMax->isChecked());
        ui->doubleSpinBoxEpsX->setEnabled(ui->checkBoxConvergence->isChecked());
        ui->doubleSpinBoxDiffStep->setEnabled(ui->checkBoxNumDiff->isChecked());

        ui->spinBoxMaxIts->blockSignals(osSBMaxIts);
        ui->doubleSpinBoxEpsX->blockSignals(osSBEpsX);
        ui->doubleSpinBoxDiffStep->blockSignals(osSBDiffSTep);
        ui->checkBoxItMax->blockSignals(osCBMaxIts);
        ui->checkBoxConvergence->blockSignals(osCBEpsX);
        ui->checkBoxNumDiff->blockSignals(osCBDiffSTep);
    }
}

/*
 * preset api version <= 5.0
 */
void PeakFitWidget::applyPreset050000(const QDomElement &rootEl)
{
    // api < 5.1 supports only one range and does not store uids in xml
    QDomNodeList l = rootEl.elementsByTagName("curveRange");
    if (!l.size()) return;
    QDomElement elRange = l.at(0).toElement();

    _rangeCounter = 1;

    PeakFitRange *rangeIt = new PeakFitRange(QUuid::createUuid());
    rangeIt->setName("Range 1");
    rangeIt->setRangeNumber(_rangeCounter);
    rangeIt->setLowerLimit(elRange.attribute("RangeStart").toDouble());
    rangeIt->setUpperLimit(elRange.attribute("RangeEnd").toDouble());
    ui->treeWidgetFunctions->addTopLevelItem(rangeIt);
    rangeIt->setExpanded(true);

    PeakFitItem *rangeVarIt = new PeakFitItem(rangeIt->uid());
    rangeVarIt->setName(rangeIt->name());
    ui->treeWidgetVariables->addTopLevelItem(rangeVarIt);
    rangeVarIt->setExpanded(true);

    ui->buttonAddPeak->setEnabled(true);
    ui->toolButtonAutoAddFunction->setEnabled(true);
    ui->comboBoxPeakFunction->setEnabled(true);

    static QRegularExpression rxVarGroup("^(\\$\\d+)-\\d+$");
    QMap<QString, QString> varValues;
    QList<QUuid> varUids;
    QString lastVarGroup;

    QDomNodeList lVariables = rootEl.elementsByTagName("curveVariable");
    _curveCounter = -1;

    for (int v = 0; v < lVariables.size(); ++v) {
        QDomElement el = lVariables.at(v).toElement();

        QString vName = el.attribute("Name");
        _curveCounter = qMax(_curveCounter, getVariableCurveNumber050000(el));

        // Check if a uid for function $v was already created. If not, create it.
        QRegularExpressionMatch rm = rxVarGroup.match(vName);

        if (rm.hasMatch()) {
            if (rm.captured(1) != lastVarGroup) {
                varUids.append(QUuid::createUuid());
                lastVarGroup = rm.captured(1);
            }
        } else {
            qDebug() << QString("PeakFitWidget::applyPreset050000(): Illegal variable name detected: %1. Skipping.").arg(vName);
            continue;
        }

        PeakFitVariable *varIt = new PeakFitVariable(varUids.last());
        varIt->setName(vName);
        varIt->setValue(el.attribute("Value"), el.attribute("LowerLimit"), el.attribute("UpperLimit"));
        varIt->setChecked(el.attribute("CheckState", "1") == "1");
        varValues[vName] = el.attribute("Value");
        rangeVarIt->addChild(varIt);
    }

    QDomNodeList lFunctions = rootEl.elementsByTagName("curveFunction");

    for (int f = 0; f < lFunctions.size(); ++f) {
        QDomElement el = lFunctions.at(f).toElement();

        int cType      = el.attribute("Type", "0").toInt();
        QUuid cUid     = f < varUids.size() ? varUids.at(f) : QUuid::createUuid();

        std::shared_ptr<GenericCurve> curve = CurveHandler::createCurve(cType);
        if (!curve) continue;

        QString cName = QString("%1 %2").arg(f+1).arg(curve->description());
        curve->setUid(cUid);
        curve->setDisplayName(cName);

        PeakFitCurve *curveIt = new PeakFitCurve(cUid);
        curveIt->setCurveType(cType);
        curveIt->setName(cName);
        curveIt->setCurve(curve);

        QDomElement elParNames = lFunctions.at(f).firstChildElement("ParameterNames");
        QDomElement elVarNames = lFunctions.at(f).firstChildElement("Variables");

        if (elParNames.isNull() || elVarNames.isNull()) continue;

        QStringList parNames = elParNames.text().split(";");
        QStringList varNames = elVarNames.text().split(";");

        for (int i = 0; i < qMin(parNames.size(), varNames.size()); ++i) {
            if (parNames.at(i) == "Area") continue;

            PeakFitParameter *paramIt = new PeakFitParameter(cUid);
            paramIt->setName(parNames.at(i));
            paramIt->setVariableName(varNames.at(i));
            paramIt->setValue(varValues.value(varNames.at(i)));

            curveIt->addChild(paramIt);
        }

        if (curve->hasArea()) appendAreaItem(curveIt);
        rangeIt->addChild(curveIt);
        curveIt->setExpanded(true);
    }

    if (_curveCounter < 1) {
        _curveCounter = lFunctions.size();
    }

    updateCurves();
    setGraphHiglightRanges();
}

/*
 * preset api version >= 5.1
 */
void PeakFitWidget::applyPreset050100(const QDomElement &rootEl)
{
    // api 5.1 supports multiple ranges. uids are used to relate parameters and variables with curves
    QDomNodeList lRanges = rootEl.elementsByTagName("curveRange");
    for (int r = 0; r < lRanges.size(); ++r) {
        QDomElement el = lRanges.at(r).toElement();

        ++_rangeCounter;

        PeakFitRange *rangeIt = new PeakFitRange(QUuid::fromString(el.attribute("UID", QUuid::createUuid().toString())));
        rangeIt->setName(el.attribute("RangeName", QString("Range %1").arg(r+1)));
        rangeIt->setRangeNumber(el.attribute("RangeNumber", QString("%1").arg(_rangeCounter)).toInt());
        rangeIt->setLowerLimit(el.attribute("RangeStart").toDouble());
        rangeIt->setUpperLimit(el.attribute("RangeEnd").toDouble());
        ui->treeWidgetFunctions->addTopLevelItem(rangeIt);
        rangeIt->setExpanded(true);

        PeakFitItem *rangeVarIt = new PeakFitItem(rangeIt->uid());
        rangeVarIt->setName(rangeIt->name());
        ui->treeWidgetVariables->addTopLevelItem(rangeVarIt);
        rangeVarIt->setExpanded(true);

        ui->buttonAddPeak->setEnabled(true);
        ui->toolButtonAutoAddFunction->setEnabled(true);
        ui->comboBoxPeakFunction->setEnabled(true);

        QMap<QString, QString> varValues;

        QDomNodeList lVariables = el.elementsByTagName("curveVariable");

        for (int v = 0; v < lVariables.size(); ++v) {
            QDomElement el = lVariables.at(v).toElement();

            QString vName = el.attribute("Name");
            QUuid vUid  = QUuid::fromString(el.attribute("UID"));

            PeakFitVariable *varIt = new PeakFitVariable(vUid);
            varIt->setName(vName);
            varIt->setValue(el.attribute("Value"), el.attribute("LowerLimit"), el.attribute("UpperLimit"));
            varIt->setChecked(el.attribute("CheckState", "1") == "1");
            varValues[vName] = el.attribute("Value");
            rangeVarIt->addChild(varIt);
        }

        QDomNodeList lFunctions = el.elementsByTagName("curveFunction");
        _curveCounter = -1;

        for (int f = 0; f < lFunctions.size(); ++f) {
            QDomElement el = lFunctions.at(f).toElement();

            QString cName  = el.attribute("Name");
            QUuid cUid     = QUuid::fromString(el.attribute("UID"));
            int cType      = el.attribute("Type", "0").toInt();
            _curveCounter = qMax(_curveCounter, getCurveNumber(el));

            std::shared_ptr<GenericCurve> curve = CurveHandler::createCurve(cType);
            if (!curve) continue;
            curve->setUid(cUid);
            curve->setDisplayName(cName);

            PeakFitCurve *curveIt = new PeakFitCurve(cUid);
            curveIt->setCurveType(cType);
            curveIt->setName(cName);
            curveIt->setCurve(curve);

            QDomElement elParNames = lFunctions.at(f).firstChildElement("ParameterNames");
            QDomElement elVarNames = lFunctions.at(f).firstChildElement("Variables");

            if (elParNames.isNull() || elVarNames.isNull()) continue;

            QStringList parNames = elParNames.text().split(";");
            QStringList varNames = elVarNames.text().split(";");

            for (int i = 0; i < qMin(parNames.size(), varNames.size()); ++i) {
                PeakFitParameter *paramIt = new PeakFitParameter(cUid);
                paramIt->setName(parNames.at(i));
                paramIt->setVariableName(varNames.at(i));
                paramIt->setValue(varValues.value(varNames.at(i)));

                curveIt->addChild(paramIt);
            }

            if (curve->hasArea()) appendAreaItem(curveIt);
            rangeIt->addChild(curveIt);
            curveIt->setExpanded(true);
        }

        if (_curveCounter < 1) {
            _curveCounter = lFunctions.size();
        }
    }

    updateCurves();
    setGraphHiglightRanges();
}

QString PeakFitWidget::getReport(const QString &scanFile)
{
    const Scan *curScan = scanControl->firstActiveScan();

    if (!curScan) {
        qDebug() << QString("PeakFitWidget::getReport(): Error, no scan data found in project %1").arg(scanFile);
        return QString("Error: No scan data found in project %1.").arg(scanFile);
    }

    QStringList out;
    out << "File;" + scanFile;
    out << "Scan;" + scanControl->scanName(curScan);

    for (int r = 0; r < ui->treeWidgetFunctions->topLevelItemCount(); ++r) {
        PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(ui->treeWidgetFunctions->topLevelItem(r));
        if (!rIt) continue;

        out << QString("Range name;%1").arg(rIt->name());
        out << QString(";Range;%1;%2").arg(rIt->lowerLimit(), 0, 'f', 4).arg(rIt->upperLimit(), 0, 'f', 4);
        out << QString(";Step size;%1").arg(ui->checkBoxStepSize->isChecked() ? ui->doubleSpinBoxStepSize->value() : curScan->stepSize());
        out << QString();

        for (int c = 2; c < rIt->childCount(); ++c) {
            PeakFitCurve *cIt = dynamic_cast<PeakFitCurve*>(rIt->child(c));
            if (!cIt) continue;

            std::shared_ptr<GenericCurve> cur = cIt->getCurve();
            if (!cur) continue;

            out << ";" + cIt->name();
            out << ";;Equation;" + cur->equationText();
            out << ";;Parameters";

            for (int j = 0; j < cur->parameterText().size(); ++j) {
                out << ";;;" + cur->parameterText().at(j);
            }

            out << ";;Results";

            for (int p = 0; p < cIt->childCount(); ++p) {
                PeakFitParameter *pIt = dynamic_cast<PeakFitParameter*>(cIt->child(p));
                if (!pIt) continue;

                out << ";;;" + pIt->parameterName() + ";" + pIt->valueT();
            }

            out << QString();
        }
    }

    out << QString();

    QList<Scan> curveFits;

    for (int i = 0; i < ui->treeWidgetFunctions->topLevelItemCount(); ++i) {
        PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(ui->treeWidgetFunctions->topLevelItem(i));
        if (rIt) curveFits.append(rIt->getCalculatedCurves(ui->checkBoxStepSize->isChecked() ? ui->doubleSpinBoxStepSize->value() : -1.0));
    }

    QString scanHeader("Angle;");

    for (int i = 0; i < curveFits.size(); ++i) {
        scanHeader += curveFits.at(i).name() + ";";
    }

    out << scanHeader;

    for (int l = 0; l < curveFits.at(0).pDataAngle().size(); ++l) {
        QStringList line(QString("%1").arg(curveFits.at(0).pDataAngle().at(l), 0, 'f', 6));

        for (int i = 0; i < curveFits.size(); ++i) {
            line << QString("%1").arg(curveFits.at(i).pDataIntensity().at(l), 0, 'f', 6);
        }

        out << line.join(";");
    }

    return out.join("\n");
}

void PeakFitWidget::cursorMessage(QString s, QUuid u)
{
    if (u != uid) return;
    ui->labelCursorMessage->setText(s);
}

void PeakFitWidget::activeCurveChanged(QTreeWidgetItem *it, int)
{
    if (!it) return;

    PeakFitItem *iIt = dynamic_cast<PeakFitItem*>(it);

    if (iIt) scanControl->setActiveScans(QList<QUuid>() << iIt->uid());
    else     scanControl->clearActiveScans();

    scanControl->updateViews(vModes, this);
}

void PeakFitWidget::updateDataRange()
{
    if (scanControl) ui->buttonAddRange->setEnabled(scanControl->hasData());
}

void PeakFitWidget::uncheckSetRangeButton()
{
    bool oldState = ui->buttonAddRange->blockSignals(true);
    ui->buttonAddRange->setChecked(false);
    graphView->setRangeSelectMode(false, uid);
    ui->buttonAddRange->blockSignals(oldState);
}

void PeakFitWidget::uncheckAddFunctionButton()
{
    bool oldState = ui->buttonAddPeak->blockSignals(true);
    ui->buttonAddPeak->setChecked(false);
    graphView->setPeakPreviewMode(PPMNONE, uid);
    ui->buttonAddPeak->blockSignals(oldState);
}

void PeakFitWidget::functionsHeaderResized()
{
    settings->setValue("peakFitting/functionsHeader", ui->treeWidgetFunctions->header()->saveState());
}

void PeakFitWidget::variablesHeaderResized()
{
    settings->setValue("peakFitting/variablesHeader", ui->treeWidgetVariables->header()->saveState());
}

void PeakFitWidget::stepSizeCustomToggled()
{
    settings->setValue("peakFitting/stepSizeCustom", ui->checkBoxStepSize->isChecked());
}

void PeakFitWidget::stepSizeValueChanged()
{
    settings->setValue("peakFitting/setpSizeValue", ui->doubleSpinBoxStepSize->value());
}

void PeakFitWidget::itMaxChanged()
{
    settings->setValue("peakFitting/controls/maxIts", ui->spinBoxMaxIts->value());
}

void PeakFitWidget::epsxChanged()
{
    settings->setValue("peakFitting/controls/epsX", ui->doubleSpinBoxEpsX->value());
}

void PeakFitWidget::diffStepChanged()
{
    settings->setValue("peakFitting/controls/diffStep", ui->doubleSpinBoxDiffStep->value());
}

void PeakFitWidget::controlsAutoToggled()
{
    settings->setValue("peakFitting/controls/maxItsManual", ui->checkBoxItMax->isChecked());
    settings->setValue("peakFitting/controls/epsXManual", ui->checkBoxConvergence->isChecked());
    settings->setValue("peakFitting/controls/diffStepManual", ui->checkBoxNumDiff->isChecked());

    ui->spinBoxMaxIts->setEnabled(ui->checkBoxItMax->isChecked());
    ui->doubleSpinBoxEpsX->setEnabled(ui->checkBoxConvergence->isChecked());
    ui->doubleSpinBoxDiffStep->setEnabled(ui->checkBoxNumDiff->isChecked());
}

void PeakFitWidget::showHelpDialog()
{
    emit helpText("peakFitWidget");
}

int PeakFitWidget::getVariableCurveNumber050000(const QDomElement &el) const
{
    static QRegularExpression rx("^\\$(\\d+)-");
    QRegularExpressionMatch rm = rx.match(el.attribute("Name"));
    if (rm.hasMatch()) return rm.captured(1).toInt();
    return -1;
}

/*
 * extract the curve number from the name scheme "#n <name>"
 * if the attribute "Number" is not found
 * for compatibility with older preset formats
 */
int PeakFitWidget::getCurveNumber(const QDomElement &el) const
{
    if (el.hasAttribute("Number")) {
        return el.attribute("Number").toInt();
    }

    static QRegularExpression rx("^#(\\d+)\\s*");
    QRegularExpressionMatch rm = rx.match(el.attribute("Name"));

    if (rm.hasMatch()) return rm.captured(1).toInt();
    return -1;
}

void PeakFitWidget::autoAddFunction()
{
    const Scan *curScan = getActiveDataScan();
    PeakFitRange *rIt = dynamic_cast<PeakFitRange*>(getCurrentRangeItem());

    if (!curScan) {
        qDebug() << QString("PeakFitWidget::autoAddFunction(): No active scan found. Exiting.");
        return;
    }

    if (!rIt) {
        qDebug() << QString("PeakFitWidget::autoAddFunction(): No range selected. Exiting.");
        return;
    }

    double zoomAngMin;
    double zoomAngMax;
    double zoomIntMin;
    double zoomIntMax;

    graphView->getZoomRange(zoomAngMin, zoomAngMax, zoomIntMin, zoomIntMax);

    double rangeStart = qMax(rIt->lowerLimit(), zoomAngMin);
    double rangeEnd   = qMin(rIt->upperLimit(), zoomAngMax);

    if (!initSolver(rIt)) {
        qDebug() << QString("PeakFitWidget::autoAddFunction(): Initializing solver failed. Exiting.");
        return;
    }

    Scan dataRange = curScan->mid(rangeStart, rangeEnd);
    QList<Scan> curves = rIt->getCalculatedCurves(-1.0);

    // subtract curves from scan (curves also contains the sum curve, therefore skip the last one)
    for (int i = 0; i < curves.size() - 1; ++i) {
        int n = qMin(dataRange.size(), curves.at(i).size());

        for (int j = 0; j < n; ++j) {
            dataRange.pDataIntensity()[j] -= curves.at(i).pDataIntensity().at(j);
        }
    }

    PeakPreviewMode curPpm = static_cast<PeakPreviewMode>(ui->comboBoxPeakFunction->currentData(Qt::UserRole).toInt());

    if (curPpm == PPMPEAK) {
        autoAddPeak(dataRange);
    } else if (curPpm == PPMLIN) {
        autoAddLinear(dataRange);
    } else {
        qDebug() << QString("PeakFitWidget::autoAddFunction(): Mode not supported.");
    }
}

void PeakFitWidget::autoAddPeak(const Scan &scan)
{
    double maxVal = 0.0;
    double maxPos = 0.0;
    double hwhmL  = 0.0;
    double hwhmR  = 0.0;
    int maxIdx = -1;
    int hwhmLidx = -1;
    int hwhmRidx = -1;

    if (!ScanOps::maxPeak(scan, maxPos, maxVal, hwhmL, hwhmR, maxIdx, hwhmLidx, hwhmRidx)) {
        qDebug() << QString("PeakFitWidget::autoAddPeak(): No residual peak found. Exiting.");
        return;
    }

    double minLeft  = scan.minIntensity(maxPos - 3.0 * hwhmL, maxPos);
    double minRight = scan.minIntensity(maxPos, maxPos + 3.0 * hwhmR);

    QPointF ptTip(maxPos, maxVal);
    QPointF ptFoot(maxPos - hwhmL - hwhmR, 0.5 * (minLeft + minRight));
    peakPreviewPoints(ptTip, ptFoot, PPMPEAK, uid, false);
    qDebug() << QString("PeakFitWidget::autoAddPeak(): Residual peak found at %1 (%2, %3)").arg(maxPos).arg(hwhmL).arg(hwhmR);
}

void PeakFitWidget::autoAddLinear(const Scan &scan)
{
    double len = (scan.endAngle() - scan.startAngle()) / 5.0;
    int minLeft  = scan.indexOfMinIntensity(scan.startAngle(), scan.startAngle() + len);
    int minRight = scan.indexOfMinIntensity(scan.endAngle() - len, scan.endAngle());

    if (minLeft < 0 || minRight < 0) {
        qDebug() << QString("PeakFitWidget::autoAddLinear(): Could not locate minima. Exiting.");
        return;
    }

    double ptLeftX  = scan.pDataAngle().at(minLeft);
    double ptLeftY  = scan.pDataIntensity().at(minLeft);
    double ptRightX = scan.pDataAngle().at(minRight);
    double ptRightY = scan.pDataIntensity().at(minRight);

    QPointF ptLeft(ptLeftX,   ptLeftY);
    QPointF ptRight(ptRightX, ptRightY);
    peakPreviewPoints(ptLeft, ptRight, PPMLIN, uid, false);
    qDebug() << QString("PeakFitWidget::autoAddLinear(): Minima found at %1 %2 and %3, %4").arg(ptLeftX).arg(ptLeftY).arg(ptRightX).arg(ptRightY);
}

void PeakFitWidget::selectRegion(int n)
{
    if (n >= ui->treeWidgetFunctions->topLevelItemCount()) return;
    ui->treeWidgetFunctions->setCurrentItem(ui->treeWidgetFunctions->topLevelItem(n));
}

/* class FitTask */
FitTask::FitTask(PeakFitRange *range)
    : range(range)
{
    // we’ll delete ourselves exactly once via deleteLater() in the completion slot
    setAutoDelete(false);
}

void FitTask::run()
{
    QMap<QString, QVariant> report;
    QVector<double> fitValues = range->fit();
    report = range->report();
    emit fitCompleted(range, report, fitValues);
}


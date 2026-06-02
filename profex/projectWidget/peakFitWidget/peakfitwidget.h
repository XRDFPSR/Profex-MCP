/***************************************************************************
                          peakfitwidget.h  -  description
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

#ifndef PEAKFITWIDGET_H
#define PEAKFITWIDGET_H

#include <QWidget>
#include <QDomDocument>
#include <QVector>
#include "peakfitrange.h"
#include "peakfitcurve.h"
#include "peakfitparameter.h"
#include "peakfitvariable.h"
#include "projectWidget/graphWidget/abstractgraphview.h"
#include "projectWidget/graphWidget/graphwindow.h"
#include "../libXrdIO/scan.h"
#include "../libXrdIO/scanops.h"

namespace Ui {
class PeakFitWidget;
}

class FitTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit FitTask(PeakFitRange *range);
    void cancel() { range->abortFit(); }

    void run() override;

signals:
    void fitCompleted(PeakFitRange *range, QMap<QString, QVariant> report, QVector<double> fitValues);

private:
    PeakFitRange *range;
};

class PeakFitWidget : public AbstractGraphView
{
    Q_OBJECT

public:
    explicit PeakFitWidget(GraphDataController *c, GraphWindow *v, QWidget *parent = nullptr);
    ~PeakFitWidget();

    void clearTemporary();
    bool hasData() const;
    inline bool isRunning() {return m_fitTasks.size() > 0;}

    QMap<QString, global::CurveFitVariable> getVariablesByName(PeakFitRange *);

    void getPreset(QDomDocument &);
    void applyPreset(const QDomElement &);
    QString getReport(const QString &);
    void startFit(QThreadPool *pool);
    void selectRegion(int);

public slots:
    void updateView();
    void abortFit();

private:
    Ui::PeakFitWidget *ui;
    GraphWindow *graphView;
    bool wasUsed;
    QStringList headers;
    int _rangeCounter;
    int _curveCounter;
    int remainingTasks;
    int completedTasks;
    QList<FitTask*> m_fitTasks;

    void initSettings();
    void saveSettings();
    bool initSolver(PeakFitRange *);

    void addCurve(std::shared_ptr<GenericCurve> curve, const QList<double> &values);
    void clearAllRanges();

    void readInitialValues(PeakFitRange *, QStringList &, QVector<double> &, QVector<double> &, QVector<double> &);
    void writeFittedValues(PeakFitRange *, const QVector<double> &);

    void appendAreaItem(PeakFitCurve *);

    void editRangeBoundaryItem(QTreeWidgetItem *);
    void editCurveParameterItem(QTreeWidgetItem *);

    QPointF uncorrectedPoint(const QPointF &);

    const Scan * getActiveDataScan();
    QTreeWidgetItem * getCurrentRangeItem() const;
    int itemDepth(QTreeWidgetItem *);

    QStringList getAllRangeVariableNames(PeakFitRange *) const;

    PeakFitItem * getVariableRangeItem(PeakFitRange *) const;

    void applyPreset050000(const QDomElement &);
    void applyPreset050100(const QDomElement &);
    void applyPresetControls050100(const QDomElement &);

    int getVariableCurveNumber050000(const QDomElement &el) const;
    int getCurveNumber(const QDomElement &el) const;

    void autoAddPeak(const Scan &);
    void autoAddLinear(const Scan &);

    void setStartButtonToAbort(bool);
    QString getProtocolOutput(const PeakFitRange *rIt, const QMap<QString, QVariant> &, const QVector<double> &fitValues);

private slots:
    void fit();
    void append();
    void removeCurve();
    void keepTemporary();
    void toggleSelectRangeMode(bool);
    void toggleAddPeakMode(bool);
    void togglePeakFunction(int);
    void peakPreviewPoints(QPointF, QPointF, PeakPreviewMode, QUuid, bool);
    void rangePoints(QPointF, QPointF, QUuid);
    void functionDoubleClicked(QTreeWidgetItem *, int);
    void variableDoubleClicked(QTreeWidgetItem *, int);
    void updateCurves();
    void clearAll();
    void undoControls();
    void cursorMessage(QString, QUuid);
    void activeCurveChanged(QTreeWidgetItem*, int);
    void updateDataRange();
    void uncheckSetRangeButton();
    void uncheckAddFunctionButton();
    void functionsHeaderResized();
    void variablesHeaderResized();
    void stepSizeCustomToggled();
    void stepSizeValueChanged();
    void itMaxChanged();
    void epsxChanged();
    void diffStepChanged();
    void controlsAutoToggled();
    void showHelpDialog();
    void setGraphHiglightRanges();
    void autoAddFunction();
    void onFitTaskCompleted(PeakFitRange *rIt, QMap<QString, QVariant> report, QVector<double> fitValues);

signals:
    void updateScans();
    void protocol(QString);
    void clearProtocol();
    void applyToAll();
    void helpText(QString);
    void fitCompleted(global::RefinementStatus);
    void statusChanged(global::RefinementStatus);
};



#endif // PEAKFITWIDGET_H

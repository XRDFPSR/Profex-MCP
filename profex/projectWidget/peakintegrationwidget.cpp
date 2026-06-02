/***************************************************************************
                          peakintegrationwidget.cpp  -  description
                             -------------------
    begin                : Wed Oct 12 18:10:00 CEST 2017
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

#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QGridLayout>
#include <QGuiApplication>
#include <QMessageBox>
#include <QClipboard>
#include <QString>
#include <QFlags>
#include <QtGlobal>

#include "peakintegrationwidget.h"
#include "ui_peakintegrationwidget.h"
#include "graphWidget/graphdatacontroller.h"
#include "../libXrdIO/scanops.h"

PeakIntegrationWidget::PeakIntegrationWidget(GraphDataController *c, GraphWindow *v, QWidget *parent) :
    AbstractGraphView(c, parent),
    ui(new Ui::PeakIntegrationWidget)
{
    graphView = v;

    ui->setupUi(this);

    vModes.insert(global::ViewUpdateMode::DISPLAY);

    previousNumberOfScans = 0;
    mapper = new QSignalMapper(this);
    subtractBackground = false;
    unit = QString("%1%2%3").arg(global::degree).arg("2").arg(global::theta);
    defaultHeaders << QString("Range Name") << QString("Range Start [%1]").arg(unit) << QString("Range End [%1]").arg(unit);

    initSettings();
    initTable();

    updateView();
    displayRegions();

    connect(ui->table, SIGNAL(cellChanged(int,int)), this, SLOT(cellContentChanged(int,int)));
    connect(mapper,    SIGNAL(mappedInt(int)),       this, SLOT(rangeChanged(int)));
    connect(ui->table->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), this, SLOT(headerResized()));
    connect(ui->buttonBackground, SIGNAL(toggled(bool)), this, SLOT(bkgrButtonToggled()));

    if (graphView) {
        connect(graphView, SIGNAL(sigRangePoints(QPointF,QPointF,QUuid)), this, SLOT(rangePoints(QPointF,QPointF,QUuid)));
        connect(graphView, SIGNAL(sigCursorMessage(QString,QUuid)),       this, SLOT(cursorMessage(QString,QUuid)));
    }

    if (scanControl) {
        connect(scanControl, SIGNAL(dataUpdated()), this, SLOT(updateDataRange()));
    }
}

PeakIntegrationWidget::~PeakIntegrationWidget()
{
    if (mapper) delete mapper;
    if (ui) delete ui;
}

void PeakIntegrationWidget::initSettings()
{
    rangeMin = settings->value("peakIntegrationWidget/rangeMin", 0.0).toDouble();
    rangeMax = settings->value("peakIntegrationWidget/rangeMax", 0.0).toDouble();
    subtractBackground = settings->value("peakIntegrationWidget/subtractBg", true).toBool();
    ui->buttonBackground->setChecked(subtractBackground);
}

void PeakIntegrationWidget::initTable()
{
    ui->table->setRowCount(0);
    ui->table->setColumnCount(3);
    ui->table->setHorizontalHeaderLabels(defaultHeaders);

    if (graphView) {
        graphView->setIntegralRanges(QList<global::HighlightRegion>());
    }

    headers = defaultHeaders;

    if (!scanControl->count()) return;

    rangeMin = scanControl->getXmin();
    rangeMax = scanControl->getXmax();

    for (int i = 0; i < scanControl->count(); ++i) {
        headers.append(scanControl->scanName(scanControl->at(i)));
    }

    ui->table->setColumnCount(scanControl->count() + 3);
    ui->table->setHorizontalHeaderLabels(headers);
    ui->table->horizontalHeader()->restoreState(settings->value("peakIntegrationWidget/headerState", QByteArray()).toByteArray());
}

bool PeakIntegrationWidget::hasData() const
{
    return ui->table->rowCount() > 0;
}

/*
 * updates rows depending on 'row':
 * row < 0: updates all rows
 * row = i: updates row i
 */
void PeakIntegrationWidget::compute(int row)
{
    if (!ui->table->rowCount()) return;
    if (ui->table->columnCount() < 4) return;

    if (row < 0) { // row < 0 updates all rows
        for (int r = 0; r < ui->table->rowCount(); ++r) {
            computeRow(r);
        }
    } else { // row >= 0 updates row only
        computeRow(row);
    }
}

/*
 * updates row i
 */
void PeakIntegrationWidget::computeRow(int i)
{
    double _start = 0.0;
    double _end   = 0.0;

    if (!rangeBoundaries(i, _start, _end)) return;

    for (int c = 0; c < scanControl->count(); ++c) {
        const Scan *s = scanControl->at(c);
        double value = ScanOps::integrate(s, _start, _end, subtractBackground);

        if (c < ui->table->columnCount() - 3) {
            QTableWidgetItem *it = new QTableWidgetItem(QString("%1").arg(value, 0, 'f', 2));
            it->setFlags(it->flags() &  ~Qt::ItemIsEditable);
            ui->table->setItem(i, c + 3, it);
        }
    }
}

bool PeakIntegrationWidget::rangeBoundaries(int i, double &start, double &end)
{
    QDoubleSpinBox* sbStart = dynamic_cast<QDoubleSpinBox*>(ui->table->cellWidget(i, 1));
    QDoubleSpinBox* sbEnd   = dynamic_cast<QDoubleSpinBox*>(ui->table->cellWidget(i, 2));

    if (!sbStart || !sbEnd) return false;

    double _start = qMin(sbStart->value(), sbEnd->value());
    double _end   = qMax(sbStart->value(), sbEnd->value());

    start = _start + angularCorrection(_start);
    end   = _end   + angularCorrection(_end);
    return true;
}

/*
 * sends the regions to the current graphWidget to highlight the areas
 */
void PeakIntegrationWidget::displayRegions()
{
    if (!graphView) return;
    if (ui->table->columnCount() < 3) return;

    QColor rangeColor = QColor(settings->value("graph/integralRangeColor", QString("#e0e0d8")).toString());
    QPen centerLine(QColor(settings->value("graph/integralCenterLineColor", QString("#999999")).toString()));
    QList<global::HighlightRegion> regions;

    for (int i = 0; i < ui->table->rowCount(); ++i) {
        QDoubleSpinBox* sbStart = dynamic_cast<QDoubleSpinBox*>(ui->table->cellWidget(i, 1));
        QDoubleSpinBox* sbEnd   = dynamic_cast<QDoubleSpinBox*>(ui->table->cellWidget(i, 2));

        if (sbStart && sbEnd) {
            double _start = qMin(sbStart->value(), sbEnd->value());
            double _end   = qMax(sbStart->value(), sbEnd->value());

            global::HighlightRegion region(ui->table->item(i, 0)->text(),
                                           QPointF(_start, subtractBackground ? 1.0 : -1.0),
                                           QPointF(_end,   subtractBackground ? 1.0 : -1.0),
                                           true,
                                           rangeColor,
                                           centerLine);
            regions.append(region);
        }
    }

    graphView->setIntegralRanges(regions);
}

/*
 * removes the current range
 */
void PeakIntegrationWidget::removeRange()
{
    ui->toolButtonAddRange->setChecked(false);

    int i = ui->table->currentRow();
    ui->table->removeRow(i);
    displayRegions();
    if (graphView) graphView->forceUpdate();
}

/*
 * appends a new range. When appending several ranges in a row, call 'update' only
 * for the last one for performance reasons
 */
void PeakIntegrationWidget::appendRange(const QString &_name, double _start, double _end, bool _update)
{
    int r = ui->table->rowCount();
    ui->table->setRowCount(r + 1);

    QDoubleSpinBox *sbRangeStart = new QDoubleSpinBox(nullptr);
    QDoubleSpinBox *sbRangeEnd   = new QDoubleSpinBox(nullptr);

    sbRangeStart->setDecimals(4);
    sbRangeEnd->setDecimals(4);

    sbRangeStart->setMinimum(rangeMin);
    sbRangeStart->setMaximum(rangeMax);
    sbRangeStart->setValue(_start);
    sbRangeStart->setSingleStep(0.01);
    connect(sbRangeStart, SIGNAL(valueChanged(double)), mapper, SLOT(map()));
    mapper->setMapping(sbRangeStart, r);

    sbRangeEnd->setMinimum(rangeMin);
    sbRangeEnd->setMaximum(rangeMax);
    sbRangeEnd->setValue(_end);
    sbRangeEnd->setSingleStep(0.01);
    connect(sbRangeEnd, SIGNAL(valueChanged(double)), mapper, SLOT(map()));
    mapper->setMapping(sbRangeEnd, r);

    QString str = _name.isEmpty() ? QString("Range %1").arg(r) : _name;
    ui->table->setItem(r, 0, new QTableWidgetItem(str));
    ui->table->setCellWidget(r, 1, sbRangeStart);
    ui->table->setCellWidget(r, 2, sbRangeEnd);

    if (!qFuzzyCompare(_start, _end)) {
        compute(ui->table->rowCount() - 1);
    }

    if (_update) displayRegions();
}

/*
 * slot is called when a range start or end was changed
 */
void PeakIntegrationWidget::rangeChanged(int i)
{
    computeRow(i);
    displayRegions();
    if (graphView) graphView->forceUpdate();
}

/*
 * saves all integration data to a CSV file
 */
QString PeakIntegrationWidget::getDataCsv()
{
    QString output;

    for (int r = -1; r < ui->table->rowCount(); ++r) {
        QStringList line;

        for (int c = 0; c < ui->table->columnCount(); ++c) {
            if (r < 0) {
                if (c == 0) line << QString("File") << QString("Sample ID") << ui->table->horizontalHeaderItem(c)->text();
                else        line << ui->table->horizontalHeaderItem(c)->text();
            } else {
                QDoubleSpinBox* sb = static_cast<QDoubleSpinBox*>(ui->table->cellWidget(r, c));

                switch (c) {
                    case 0:
                        line << scanControl->fileName() << scanControl->getSampleId() << ui->table->item(r, c)->text();
                        break;

                    case 1:
                        if (sb) line << QString("%1").arg(sb->value(), 0, 'f', 4);
                        else    line << QString();
                        break;

                    case 2:
                        if (sb) line << QString("%1").arg(sb->value(), 0, 'f', 4);
                        else    line << QString();
                        break;

                    default:
                        line << ui->table->item(r, c)->text();
                }
            }
        }

        output += line.join(";") + "\n";
    }

    return output;
}

QList<global::PeakRange> PeakIntegrationWidget::getRangeData()
{
    QList<global::PeakRange> _ranges;
    if (ui->table->columnCount() < 3) return _ranges;

    for (int i = 0; i < ui->table->rowCount(); ++i) {
        QDoubleSpinBox* sbStart = static_cast<QDoubleSpinBox*>(ui->table->cellWidget(i, 1));
        QDoubleSpinBox* sbEnd   = static_cast<QDoubleSpinBox*>(ui->table->cellWidget(i, 2));

        if (sbStart && sbEnd) {
            global::PeakRange pr;
            pr.name  = ui->table->item(i, 0)->text();
            pr.start = sbStart->value();
            pr.end   = sbEnd->value();
            pr.background = ui->buttonBackground->isChecked() ? 1 : 0;
            _ranges.append(pr);
        }
    }

    return _ranges;
}

void PeakIntegrationWidget::getPreset(QDomDocument &doc)
{
    if (!ui->table->rowCount()) return;

    QDomElement prangeEl = doc.createElement("peakIntegrals");

    for (int i = 0; i < ui->table->rowCount(); ++i) {
        QDoubleSpinBox* sbStart = dynamic_cast<QDoubleSpinBox*>(ui->table->cellWidget(i, 1));
        QDoubleSpinBox* sbEnd   = dynamic_cast<QDoubleSpinBox*>(ui->table->cellWidget(i, 2));

        if (sbStart && sbEnd) {
            QDomElement el = doc.createElement("peakIntegral");
            el.setAttribute("Name",  ui->table->item(i, 0)->text());
            el.setAttribute("Start", sbStart->value());
            el.setAttribute("End",   sbEnd->value());
            el.setAttribute("Background", ui->buttonBackground->isChecked() ? "1" : "0");
            prangeEl.appendChild(el);
        }
    }

    doc.documentElement().appendChild(prangeEl);
}

void PeakIntegrationWidget::applyPreset(const QDomElement &element)
{
    initTable();

    QDomNodeList l = element.elementsByTagName("peakIntegral");

    for (int i = 0; i < l.size(); ++i) {
        QDomElement el = l.at(i).toElement();
        if (el.isNull()) continue;

        QString n = el.attribute("Name");
        double st = el.attribute("Start", "0.0").toDouble();
        double ed = el.attribute("End", "0.0").toDouble();
        subtractBackground = el.attribute("Background", "0") == "1";

        appendRange(n, st, ed, false);
    }

    bool oldState = ui->buttonBackground->blockSignals(true);
    ui->buttonBackground->setChecked(subtractBackground);
    ui->buttonBackground->blockSignals(oldState);

    compute(-1);
    displayRegions();
}

void PeakIntegrationWidget::setRangeData(const QList<global::PeakRange> &r, bool clear)
{
    if (clear) initTable();

    bool bg = false;

    for (int i = 0; i < r.size(); ++i) {
        appendRange(r.at(i).name, r.at(i).start, r.at(i).end, false);
        bg = r.at(i).background == 1 ? true : false;
    }

    bool oldState = ui->buttonBackground->blockSignals(true);
    ui->buttonBackground->setChecked(bg);
    ui->buttonBackground->blockSignals(oldState);
    subtractBackground = bg;

    compute(-1);
    displayRegions();
}

/*
 * call this function if the number of scans (i.e. number of columns in the table) changes.
 * It adjusts the horizontal size of the table.
 */
void PeakIntegrationWidget::updateView()
{
    if (!isShown) return;
    bool oldState = ui->table->blockSignals(true);

    if (previousNumberOfScans != scanControl->count()) {
        previousNumberOfScans = scanControl->count();
        // setRangeData() will call initTable() first, which adjusts
        // the number of table columns to the number of scans, and then
        // writes back the ranges.
        setRangeData(getRangeData());
    } else {
        compute(-1);
    }

    ui->table->blockSignals(oldState);
}

void PeakIntegrationWidget::addRange(bool b)
{
    bool ignore = true;

    if (graphView) {
        if (b) ignore = graphView->rangeSelectModeActive();
        else   ignore = false;
    }

    if (ignore) {
        bool oldState = ui->toolButtonAddRange->blockSignals(true);
        ui->toolButtonAddRange->setChecked(false);
        ui->toolButtonAddRange->blockSignals(oldState);
        return;
    }

    graphView->setRangeSelectMode(b, uid);
}

void PeakIntegrationWidget::applyToAll()
{
    ui->toolButtonAddRange->setChecked(false);
    emit sigApplyToAll();
}

void PeakIntegrationWidget::toggleBackground(bool b)
{
    ui->toolButtonAddRange->setChecked(false);
    subtractBackground = b;
    compute(-1);
    displayRegions();
    if (graphView) graphView->forceUpdate();
}

void PeakIntegrationWidget::cellContentChanged(int r, int c)
{
    if (c) return; // we only monitor the cell containing the range name (column 0)
    rangeChanged(r);
}

void PeakIntegrationWidget::setSampleDisplacements(double, double, double)
{
    compute(-1);
    displayRegions();
}

double PeakIntegrationWidget::angularCorrection(double ztheta)
{
    double eps1 = scanControl->getAngularCorrEPS1();
    double eps2 = scanControl->getAngularCorrEPS2();
    double eps3 = scanControl->getAngularCorrEPS3();

    return global::Functions::angularCorrection(ztheta, eps1, eps2, eps3);
}

void PeakIntegrationWidget::removeAll()
{
    ui->toolButtonAddRange->setChecked(false);

    if (QMessageBox::question(this, tr("Remove all ranges"),
                              tr("Do you really want to remove all integral ranges?"),
                              QMessageBox::Yes | QMessageBox::No)
            == QMessageBox::Yes) {
        initTable();
    }

    graphView->forceUpdate();
}

void PeakIntegrationWidget::paste()
{
    ui->toolButtonAddRange->setChecked(false);
    QClipboard *clipboard = QGuiApplication::clipboard();
    QStringList lines = clipboard->text().split("\n");
    QList<global::PeakRange> ranges;

    for (int i = 0; i < lines.size(); ++i) {
        QStringList records = lines.at(i).split(QRegularExpression("[;\\t]+"));
        if (records.size() < 3) continue;

        bool oks;
        bool oke;

        global::PeakRange range;
        range.name       = records.at(0);
        range.start      = records.at(1).toDouble(&oks);
        range.end        = records.at(2).toDouble(&oke);
        range.background = 0;

        if (!oks || !oke) continue;

        ranges.append(range);
    }

    setRangeData(ranges, false);
}

void PeakIntegrationWidget::centerRanges()
{
    ui->toolButtonAddRange->setChecked(false);
    if (!ui->table->rowCount()) return;

    const Scan *scan = scanControl->firstActiveScan();
    if (!scan) return;

    for (int i = 0; i < ui->table->rowCount(); ++i) {
        QTableWidgetItem *it = ui->table->item(i, 0);

        if (!it) continue;

        if (it->isSelected()) {
            centerRangeMaxIntens(i, scan);
        }
    }
}

void PeakIntegrationWidget::centerRangeMaxIntens(int n, const Scan *scan)
{
    if (!scan->size()) return;

    QDoubleSpinBox* sbStart = dynamic_cast<QDoubleSpinBox*>(ui->table->cellWidget(n, 1));
    QDoubleSpinBox* sbEnd   = dynamic_cast<QDoubleSpinBox*>(ui->table->cellWidget(n, 2));

    if (!sbStart || !sbEnd) return;

    // double start = qMin(sbStart->value(), sbEnd->value());
    // double end   = qMax(sbStart->value(), sbEnd->value());

    double start = -1.0;
    double end = -1.0;

    if (!rangeBoundaries(n, start, end)) return;

    int startIdx = scan->indexOfAngle(start, 1);
    int endIdx   = scan->indexOfAngle(end, 2);
    qDebug() << QString("Start: %1 (%2), End: %3 (%4)").arg(start).arg(startIdx).arg(end).arg(endIdx);

    int winSize = 0;
    int maxIdx = startIdx;
    double lastMovAvg = 0.0;

    for (int i = startIdx; i <= endIdx; ++i) {
        double mAvg = 0.0;

        for (int w = -winSize; w <= winSize; ++w) {
            int iw = i + w;
            if (iw < startIdx) iw = startIdx;
            if (iw > endIdx)   iw = endIdx;
            mAvg += scan->intensity(iw);
        }

        if (mAvg > lastMovAvg) {
            lastMovAvg = mAvg;
            maxIdx = i;
        }
    }

    if (maxIdx == startIdx) return;
    if (maxIdx == endIdx)   return;

    double ttCorr = angularCorrection(scan->angle(maxIdx));
    double ttOffset = scan->angle(maxIdx) - (start + end) / 2.0;
    start = ((start + ttOffset) < scan->startAngle() ? scan->startAngle() : start) + ttOffset - ttCorr;
    end   = ((end   + ttOffset) > scan->endAngle()   ? scan->endAngle()   : end)   + ttOffset - ttCorr;

    sbStart->setValue(start);
    sbEnd->setValue(end);

    computeRow(n);
}

/*
void PeakIntegrationWidget::centerRangeMaxIntens(int n, const Scan *scan)
{
    if (!scan->size()) return;
    double start = -1.0;
    double end = -1.0;

    if (!rangeBoundaries(n, start, end)) return;

    int startIdx = scan->indexOfAngle(start, 1);
    int endIdx   = scan->indexOfAngle(end, 2);
    qDebug() << QString("Start: %1 (%2), End: %3 (%4)").arg(start).arg(startIdx).arg(end).arg(endIdx);

    int winSize = 1;
    int maxIdx = startIdx;
    double lastMovAvg = 0.0;

    for (int i = startIdx; i <= endIdx; ++i) {
        double mAvg = 0.0;

        for (int w = -winSize; w <= winSize; ++w) {
            int iw = i + w;
            if (iw < startIdx) iw = startIdx;
            if (iw > endIdx)   iw = endIdx;
            mAvg += scan->intensity(iw);
        }

        if (mAvg > lastMovAvg) {
            lastMovAvg = mAvg;
            maxIdx = i;
        }
    }

    if (maxIdx == startIdx) return;
    if (maxIdx == endIdx)   return;

    double ttOffset = scan->angle(maxIdx) - (start + end) / 2.0;
    start = start + ttOffset < scan->startAngle() ? scan->startAngle() : start + ttOffset;
    end   = end   + ttOffset > scan->endAngle()   ? scan->endAngle()   : end   + ttOffset;

    QDoubleSpinBox* sbStart = dynamic_cast<QDoubleSpinBox*>(ui->table->cellWidget(n, 1));
    QDoubleSpinBox* sbEnd   = dynamic_cast<QDoubleSpinBox*>(ui->table->cellWidget(n, 2));

    if (sbStart) sbStart->setValue(start);
    if (sbEnd)   sbEnd->setValue(end);

    computeRow(n);
}
*/

void PeakIntegrationWidget::rangePoints(QPointF pa, QPointF pb, QUuid u)
{
    if (u != uid) return;

    appendRange(QString(), qMin(pa.x(), pb.x()), qMax(pa.x(), pb.x()), true);
    updateView();
}

void PeakIntegrationWidget::cursorMessage(QString , QUuid u)
{
    if (u != uid) return;
}

void PeakIntegrationWidget::updateDataRange()
{

}

void PeakIntegrationWidget::headerResized()
{
    settings->setValue("peakIntegrationWidget/headerState", ui->table->horizontalHeader()->saveState());
}

void PeakIntegrationWidget::bkgrButtonToggled()
{
    settings->setValue("peakIntegrationWidget/subtractBg", subtractBackground);
}

void PeakIntegrationWidget::showHelp()
{
    emit helpText("peakintegrationWidget");
}

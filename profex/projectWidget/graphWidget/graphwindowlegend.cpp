/***************************************************************************
                          graphwindowlegend.h  -  description
                             -------------------
    begin                : Mon Feb 01 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#include "graphwindowlegend.h"
#include "graphdatacontroller.h"
#include <QPainter>
#include <QGuiApplication>
#include <QMouseEvent>
#include "../../../libXrdIO/scan.h"
#include "../../../libXrdIO/functions.h"

GraphWindowLegend::GraphWindowLegend(GraphDataController *c, QWidget *parent)
    : AbstractGraphView(c, parent)
{
    setAccessibleName(tr("Graph legend"));

    vModes.insert(global::ViewUpdateMode::DISPLAY);

    standardColors.append(QColor(0, 0, 0, 255));
    standardColors.append(QColor(255, 0, 0, 255));
    standardColors.append(QColor(0, 0, 255, 255));
    standardColors.append(QColor(0, 0, 0, 255));

    bgColor = Qt::white;

    contextMenuLegend = new QMenu(this);
    actLegendShow  = new QAction(tr("Show legend"));
    actLegendFrame = new QAction(tr("Draw frame"));
    actLegendFill  = new QAction(tr("Fill background"));
    actSortBottomUp = new QAction(tr("Sort bottom-up"));

    connect(actLegendShow, SIGNAL(triggered(bool)), this, SLOT(toggleLegendVisibility(bool)));
    connect(actLegendFrame, SIGNAL(triggered(bool)), this, SLOT(toggleLegendFrame(bool)));
    connect(actLegendFill, SIGNAL(triggered(bool)), this, SLOT(toggleLegendFill(bool)));
    connect(actSortBottomUp, SIGNAL(triggered(bool)), this, SLOT(toggleSortBottomUp(bool)));

    actLegendShow->setCheckable(true);
    actLegendFrame->setCheckable(true);
    actLegendFill->setCheckable(true);
    actSortBottomUp->setCheckable(true);

    // contextMenuLegend->addAction(actLegendShow); // this one conflicts with the mainwindow
                                                    // action to toggle the legend visibility.
                                                    // therefore we disable it for now
    contextMenuLegend->addAction(actLegendFrame);
    contextMenuLegend->addAction(actLegendFill);
    contextMenuLegend->addAction(actSortBottomUp);
}

void GraphWindowLegend::initSettings()
{
    QColor bgC = QColor(settings->value("graph/backgroundColor", "").toString());
    bgColor = bgC.isValid() ? bgC : QGuiApplication::palette().color(QPalette::Base);
    lineWidth = settings->value("graph/lineWidth", 1).toInt();
    boundingBoxIsDrawn = settings->value("graph/drawLegendBoundingBox", false).toBool();
    backgroundIsFilled = settings->value("graph/fillLegendBox", true).toBool();
    isDrawn = settings->value("graph/drawLegend", true).toBool();
    drawPhaseLabels = (settings->value("graph/phaseVisibilityPattern", true).toBool()
                            || settings->value("graph/phaseVisibilityHkl", true).toBool());
    sortBottomUp = settings->value("graph/legendSortBottomUp", false).toBool();
}

void GraphWindowLegend::updateView()
{
    colors.clear();
    labels.clear();
    lineWidths.clear();

    const Scan *scan = nullptr;

    for (int i = 0; i < scanControl->count(); ++i) {
        scan = scanControl->at(i);

        if (!scan)              continue;
        if (!scan->isVisible()) continue;
        if (scan->scanTypes().testFlag(Scan::PHASE) && !drawPhaseLabels) continue;

        labels.append(scanControl->scanName(scan));

        if (scan->color().isValid()) {
            colors.append(bgColor.value() < 150 ? global::Functions::colorToDarkMode(scan->color()) : scan->color());
        } else {
            if (i < colors.size()) {
                colors.append(bgColor.value() < 150 ? global::Functions::colorToDarkMode(standardColors.at(i)) : standardColors.at(i));
            } else {
                colors.append(bgColor.value() < 150 ? Qt::white : Qt::black);
            }
        }

        lineWidths.append(scan->lineWidth() < lineWidth ? lineWidth : scan->lineWidth());
    }
}

void GraphWindowLegend::redraw(QPainter &p, const QFont &f, const QColor &bg, const QRect &plt)
{
    bRect = QRect();
    bgColor = bg;
    if (!isDrawn)      return;
    if (!p.isActive()) return;
    if (plt.isNull())  return;

    updateView();

    p.save();
    QFontMetrics fm(f);
    updateBoundingRect(fm, plt);
    drawBackground(p);
    drawContent(p, f, fm);
    p.restore();
}

void GraphWindowLegend::updateBoundingRect(const QFontMetrics &fm, const QRect &plt)
{
    lTextWidth = 0;

    for (int i = 0; i < labels.size(); ++i) {
        lTextWidth = qMax(fm.boundingRect(labels.at(i)).width(), lTextWidth);
    }

    lLineSpacing = fm.lineSpacing();
    lTopBoundingBox = plt.top() + fm.lineSpacing() + fm.ascent();
    lRightBoundingBox = lTextWidth + fm.boundingRect("MM").width();
    lLineLength = fm.boundingRect("MMMM").width();
    lHorizontalSpacing = fm.boundingRect("M").width();

    lLeftBoundingBox = plt.right() - lRightBoundingBox - 2*lHorizontalSpacing - lLineLength;
    lLeftText = plt.right() - lRightBoundingBox - lHorizontalSpacing;
    lTopText = lTopBoundingBox;

    bRect = QRect(lLeftBoundingBox - lHorizontalSpacing,
                  lTopBoundingBox - int(fm.height() / 2.0),
                  lTextWidth + 2 * lHorizontalSpacing + lLeftText - lLeftBoundingBox,
                  labels.size() * lLineSpacing + int(fm.descent()));
}

void GraphWindowLegend::drawBackground(QPainter &p)
{
    if (!backgroundIsFilled && !boundingBoxIsDrawn) return;

    QColor colBox(boundingBoxIsDrawn ? (bgColor.value() < 150 ? Qt::white : Qt::black) : bgColor);

    p.setPen(QPen(colBox));
    p.setBrush(backgroundIsFilled ? QBrush(bgColor) : QBrush());
    p.drawRect(bRect);
}

void GraphWindowLegend::drawContent(QPainter &p, const QFont &f, const QFontMetrics &fm)
{
    int v = lTopText;
    p.setFont(f);

    int lastIdx = qMin(labels.size(), qMin(colors.size(), lineWidths.size()));

    for (int i = 0; i < lastIdx; i++) {
        int idx = sortBottomUp ? lastIdx - i - 1 : i;
        QPen pen(colors.at(idx));
        pen.setWidth(lineWidths.at(idx));

        p.setPen(pen);
        p.drawLine(lLeftBoundingBox, v, lLeftBoundingBox + lLineLength, v);
        p.drawText(lLeftText, v + int(fm.ascent() / 2.0), labels.at(idx));
        v += lLineSpacing;
    }
}

void GraphWindowLegend::showContextMenu(QMouseEvent *e)
{
    actLegendShow->setChecked(isDrawn);
    actLegendFrame->setChecked(settings->value("graph/drawLegendBoundingBox", false).toBool());
    actLegendFill->setChecked(settings->value("graph/fillLegendBox", true).toBool());
    actSortBottomUp->setChecked(sortBottomUp);

    contextMenuLegend->exec(e->globalPosition().toPoint());
}

void GraphWindowLegend::toggleLegendVisibility(bool b)
{
    isDrawn = b;
    settings->setValue("graph/drawLegend", isDrawn);
    emit sigRequestRedraw();
}

void GraphWindowLegend::toggleLegendFrame(bool b)
{
    boundingBoxIsDrawn = b;
    settings->setValue("graph/drawLegendBoundingBox", boundingBoxIsDrawn);
    emit sigRequestRedraw();
}

void GraphWindowLegend::toggleLegendFill(bool b)
{
    backgroundIsFilled = b;
    settings->setValue("graph/fillLegendBox", backgroundIsFilled);
    emit sigRequestRedraw();
}

void GraphWindowLegend::toggleSortBottomUp(bool b)
{
    sortBottomUp = b;
    settings->setValue("graph/legendSortBottomUp", sortBottomUp);
    emit sigRequestRedraw();
}

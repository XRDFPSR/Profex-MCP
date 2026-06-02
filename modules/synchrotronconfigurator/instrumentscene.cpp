/***************************************************************************
                          instrumentscene.cpp  -  description
                             -------------------
    begin                : Tue Jul 16 18:00:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "instrumentscene.h"
#include "arrowlineitem.h"
#include "centeredsimpletextitem.h"
#include "../libXrdIO/structs.h"
#include <QDebug>

InstrumentScene::InstrumentScene(bool darkMode)
{
    connect(this, SIGNAL(selectionChanged()), this, SLOT(updateSelection()));

    radius = 1000;
    isDarkMode = darkMode;
    detectorTilt = 0.0;

    addOpticalElements();
    addBackgroundElements();
    addRays();

    detectorGroup = createDetectorGroup();
}

void InstrumentScene::addOpticalElements()
{
    QString icnPath = isDarkMode
                ? ":/synchrotronconfigurator/svg/dark-mode/"
                : ":/synchrotronconfigurator/svg/light-mode/";

    itemMap["SAMPLE"]     = new OpticsItem(icnPath + "sample-capillary-active.svg",
                                         icnPath + "sample-capillary-inactive.svg",
                                         "SAMPLE");
    itemMap["PRIMARYBEAM"] = new OpticsItem(icnPath + "collimator-active.svg",
                                         icnPath + "collimator-inactive.svg",
                                         "PRIMARYBEAM");
    itemMap["DETECTOR"]   = new OpticsItem(icnPath + "detector-active.svg",
                                         icnPath + "detector-inactive.svg",
                                         "DETECTOR");

    int sampleWidth = itemMap["SAMPLE"]->boundingRect().width();
    int collimatorHeight = itemMap["PRIMARYBEAM"]->boundingRect().height();
    int detectorWidth = itemMap["DETECTOR"]->boundingRect().width();
    int detectorHeight = itemMap["DETECTOR"]->boundingRect().height();

    itemMap["SAMPLE"]->setPos(-sampleWidth / 2, -sampleWidth / 2);
    itemMap["PRIMARYBEAM"]->setPos(-radius, -collimatorHeight / 2);
    itemMap["DETECTOR"]->setPos(radius - detectorWidth, collimatorHeight - detectorHeight);

    itemMap["SAMPLE"]->setToolTip(QStringLiteral("Sample holder"));
    itemMap["PRIMARYBEAM"]->setToolTip(QStringLiteral("Primary beam"));
    itemMap["DETECTOR"]->setToolTip(QStringLiteral("Detector"));

    QMapIterator<QString, QGraphicsItem*> it(itemMap);
    while (it.hasNext()) {
        it.next();
        OpticsItem *oitm = dynamic_cast<OpticsItem*>(it.value());
        if (oitm) oitm->setDarkMode(isDarkMode);
        addItem(it.value());
    }
}

void InstrumentScene::addBackgroundElements()
{
    QPen linePen(isDarkMode ? Qt::lightGray : Qt::darkGray, 2.0, Qt::DashLine, Qt::RoundCap);

    QPainterPath arcPath;
    arcPath.arcMoveTo(-0.25*radius, -0.25*radius, 0.5 * radius, 0.5 * radius, 0.0);
    arcPath.arcTo(-0.25*radius, -0.25*radius, 0.5 * radius, 0.5 * radius, 0.0, 35.0);

    QGraphicsPathItem *arcItem = new QGraphicsPathItem(arcPath);
    arcItem->setPen(linePen);
    addItem(arcItem);

    int collH = itemMap["PRIMARYBEAM"]->boundingRect().height();
    int sampW = itemMap["SAMPLE"]->boundingRect().height();
    int detH  = itemMap["DETECTOR"]->boundingRect().height();
    int detW  = itemMap["DETECTOR"]->boundingRect().width();

    double tickLen = double(collH);

    double beamTop  = collH / 8.0;
    double detFront = radius - 0.99 * detW;
    double detBack  = radius - 0.71 * detW;
    double detTop   = double(collH - detH);
    double detBottom = double(collH);

    itemMap["T_TICK_1"] = addLine(detFront,           detTop,        detFront, detTop - tickLen,    linePen);
    itemMap["T_TICK_2"] = addLine(detBack,            detTop,        detBack,  detTop - tickLen,    linePen);
    itemMap["C_TICK_1"] = addLine(0.0,                0.0,           0.0,      detBottom + tickLen, linePen);
    itemMap["C_TICK_2"] = addLine(detFront,           detBottom,     detFront, detBottom + tickLen, linePen);
    itemMap["P_TICK_1"] = addLine(detFront - tickLen, 0.30 * detTop, detBack,  0.30 * detTop,       linePen);
    itemMap["P_TICK_2"] = addLine(detFront - tickLen, 0.25 * detTop, detBack,  0.25 * detTop,       linePen);

    int arrowSize = 25;
    double cXpos = -1.5 * sampW;
    double cYpos = -beamTop;
    double dXpos = 0.5 * detFront;
    double dYpos = detBottom + 0.5 * tickLen;
    double tYpos = detTop - 0.5 * tickLen;
    double pXpos = detFront - 0.5 * tickLen;

    ArrowLineItem *arrowC1 = new ArrowLineItem(cXpos,              cYpos - tickLen,         cXpos,    cYpos,         arrowSize, linePen);
    ArrowLineItem *arrowC2 = new ArrowLineItem(cXpos,              beamTop + tickLen,       cXpos,    beamTop,       arrowSize, linePen);
    ArrowLineItem *arrowD1 = new ArrowLineItem(dXpos - tickLen,    dYpos,                   0.0,      dYpos,         arrowSize, linePen);
    ArrowLineItem *arrowD2 = new ArrowLineItem(dXpos + tickLen,    dYpos,                   detFront, dYpos,         arrowSize, linePen);
    ArrowLineItem *arrowT1 = new ArrowLineItem(detFront - tickLen, tYpos,                   detFront, tYpos,         arrowSize, linePen);
    ArrowLineItem *arrowT2 = new ArrowLineItem(detBack  + tickLen, tYpos,                   detBack,  tYpos,         arrowSize, linePen);
    ArrowLineItem *arrowP1 = new ArrowLineItem(pXpos,              0.30 * detTop - tickLen, pXpos,    0.30 * detTop, arrowSize, linePen);
    ArrowLineItem *arrowP2 = new ArrowLineItem(pXpos,              0.25 * detTop + tickLen, pXpos,    0.25 * detTop, arrowSize, linePen);

    itemMap["C_ARROW_1"] = arrowC1;
    itemMap["C_ARROW_2"] = arrowC2;
    itemMap["D_ARROW_1"] = arrowD1;
    itemMap["D_ARROW_2"] = arrowD2;
    itemMap["T_ARROW_1"] = arrowT1;
    itemMap["T_ARROW_2"] = arrowT2;
    itemMap["P_ARROW_1"] = arrowP1;
    itemMap["P_ARROW_2"] = arrowP2;

    addItem(arrowC1);
    addItem(arrowC2);
    addItem(arrowD1);
    addItem(arrowD2);
    addItem(arrowT1);
    addItem(arrowT2);
    addItem(arrowP1);
    addItem(arrowP2);

    CenteredSimpleTextItem *txtC  = new CenteredSimpleTextItem("c");
    CenteredSimpleTextItem *txtD  = new CenteredSimpleTextItem("D");
    CenteredSimpleTextItem *txtT  = new CenteredSimpleTextItem("t");
    CenteredSimpleTextItem *txtP  = new CenteredSimpleTextItem("p");
    CenteredSimpleTextItem *txtTt = new CenteredSimpleTextItem(QString("2%1").arg(global::theta));

    QBrush brush = isDarkMode ? QBrush(Qt::white) : QBrush(Qt::black);
    txtC->setBrush(brush);
    txtD->setBrush(brush);
    txtT->setBrush(brush);
    txtP->setBrush(brush);
    txtTt->setBrush(brush);

    txtC->setPos(QPointF(cXpos, cYpos - tickLen - 2.0 * txtC->boundingRect().height()));
    txtD->setPos(QPointF(dXpos, dYpos - 0.5 * txtD->boundingRect().height()));
    txtT->setPos(QPointF((detFront + detBack) * 0.5, detTop - tickLen - 2.0 * txtT->boundingRect().height()));
    txtP->setPos(QPointF(detFront - tickLen - 4.0 * txtP->boundingRect().width(), 0.275 * detTop));
    txtTt->setPos(QPointF(0.30 * radius, -0.10 * radius));

    itemMap["C_TEXT"] = txtC;
    itemMap["D_TEXT"] = txtD;
    itemMap["T_TEXT"] = txtT;
    itemMap["P_TEXT"] = txtP;
    itemMap["TT_TEXT"] = txtTt;

    addItem(txtC);
    addItem(txtD);
    addItem(txtT);
    addItem(txtP);
    addItem(txtTt);
}

void InstrumentScene::addRays()
{
    QPen rayPen(isDarkMode ? QColor(255, 128, 255) : QColor(255, 0, 0), 2.0, Qt::DashLine, Qt::RoundCap);

    int collH = itemMap["PRIMARYBEAM"]->boundingRect().height();
    int collW = itemMap["PRIMARYBEAM"]->boundingRect().width();
    int detH  = itemMap["DETECTOR"]->boundingRect().height();
    int detW  = itemMap["DETECTOR"]->boundingRect().width();

    int startX = collW - radius;
    int startY = collH / 8;
    int endX   = radius - 0.73 * detW;
    int endY   = -0.8 * detH;

    QGraphicsLineItem *primCapIt1 = new QGraphicsLineItem(startX, startY, endX, startY);
    QGraphicsLineItem *primCapIt2 = new QGraphicsLineItem(startX, -startY, endX, -startY);
    QGraphicsLineItem *secCapIt1 = new QGraphicsLineItem(0, startY, endX, endY);
    QGraphicsLineItem *secCapIt2 = new QGraphicsLineItem(0, -startY, endX, endY - 2*startY);

    primCapIt1->setPen(rayPen);
    primCapIt2->setPen(rayPen);
    secCapIt1->setPen(rayPen);
    secCapIt2->setPen(rayPen);

    raysPrim.append(primCapIt1);
    raysPrim.append(primCapIt2);
    raysSec.append(secCapIt1);
    raysSec.append(secCapIt2);

    for (int i = 0; i < raysPrim.size(); ++i) raysPrim.at(i)->setVisible(true);
    for (int i = 0; i < raysSec.size(); ++i)  raysSec.at(i)->setVisible(true);

    for (int i = 0; i < raysPrim.size(); ++i) addItem(raysPrim.at(i));
    for (int i = 0; i < raysSec.size(); ++i)  addItem(raysSec.at(i));
}

QGraphicsItemGroup * InstrumentScene::createDetectorGroup()
{
    QGraphicsItemGroup *group = new QGraphicsItemGroup();
    addItem(group);

    itemMap["DETECTOR"]->setParentItem(group);
    itemMap["T_TICK_1"]->setParentItem(group);
    itemMap["T_TICK_2"]->setParentItem(group);
    itemMap["T_ARROW_1"]->setParentItem(group);
    itemMap["T_ARROW_2"]->setParentItem(group);
    itemMap["T_TEXT"]->setParentItem(group);
    itemMap["P_TICK_1"]->setParentItem(group);
    itemMap["P_TICK_2"]->setParentItem(group);
    itemMap["P_ARROW_1"]->setParentItem(group);
    itemMap["P_ARROW_2"]->setParentItem(group);
    itemMap["P_TEXT"]->setParentItem(group);

    group->setHandlesChildEvents(false);
    group->setFlag(QGraphicsItem::ItemIsSelectable, false);
    group->setFlag(QGraphicsItem::ItemHasNoContents, true);
    group->setAcceptedMouseButtons(Qt::NoButton);
    group->setZValue(0);

    itemMap["DETECTOR"]->setZValue(1);

    return group;
}

/*
 * here we react to individual scene elements requesting changes.
 * For example, changing position or loading different svg files.
 */
void InstrumentScene::updateSceneElement(const QString &s, const QVariant &v)
{
    if (!itemMap.contains(s)) return;
    // nothing to do
    Q_UNUSED(v);
}

void InstrumentScene::itemActiveStatusChanged(const QString &s, bool b)
{
    if (!itemMap.contains(s)) return;
    OpticsItem *oitm = dynamic_cast<OpticsItem*>(itemMap.value(s));
    if (oitm) oitm->setInstalled(b);
}

void InstrumentScene::setItemSelected(const QString &s)
{
    QMapIterator<QString, QGraphicsItem*> it(itemMap);

    while (it.hasNext()) {
        it.next();
        OpticsItem *oitm = dynamic_cast<OpticsItem*>(it.value());
        if (oitm) oitm->setHighlighted(it.key() == s);
    }
}

void InstrumentScene::updateSelection()
{
    QList<QGraphicsItem *> sItms = selectedItems();
    QList<QGraphicsItem *> aItms = items();

    OpticsItem *itm = sItms.size() ? dynamic_cast<OpticsItem*>(sItms.first()) : nullptr;

    for (int i = 0; i < aItms.size(); ++i) {
        OpticsItem *aitm = dynamic_cast<OpticsItem*>(aItms.at(i));

        if (aitm) {
            if (itm) aitm->setHighlighted(aitm == itm);
            else     aitm->setHighlighted(false);
        }
    }

    emit itemSelected(itm ? itm->moduleName() : QString());
}

void InstrumentScene::setDetectorTilt(double d)
{
    detectorTilt = d;

    QPointF detectorBottomLeft = itemMap["DETECTOR"]->boundingRect().bottomLeft();
    QPointF groupPivot = detectorGroup->mapFromItem(itemMap["DETECTOR"], detectorBottomLeft);

    detectorGroup->setTransformOriginPoint(groupPivot);
    detectorGroup->setRotation(-detectorTilt);
}

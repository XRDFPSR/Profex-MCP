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
#include <QtMath>
#include <QDebug>

InstrumentScene::InstrumentScene(bool darkMode)
{
    connect(this, SIGNAL(selectionChanged()), this, SLOT(updateSelection()));

    radius = 1000.0;
    angle = 30.0;
    isDarkMode = darkMode;

    addBackgroundElements();
    addOpticalElements();
    addRays();
}

void InstrumentScene::addOpticalElements()
{
    QString icnPath = isDarkMode
                ? ":/opticalElements/svgFiles/dark-mode/"
                : ":/opticalElements/svgFiles/light-mode/";

    sampleRefl = new OpticsItem(icnPath + "sample-active.svg",
                                icnPath + "sample-inactive.svg",
                                "SAMPLE");

    sampleCapil = new OpticsItem(icnPath + "sample-capillary-active.svg",
                                icnPath + "sample-capillary-inactive.svg",
                                "SAMPLE");

    sampleTrans = new OpticsItem(icnPath + "sample-transmission-active.svg",
                                icnPath + "sample-transmission-inactive.svg",
                                "SAMPLE");

    sampleRefl->setDarkMode(isDarkMode);
    sampleCapil->setDarkMode(isDarkMode);
    sampleTrans->setDarkMode(isDarkMode);

    itemMap["FOCUS"] =   new OpticsItem(icnPath + "tube-active.svg",
                                        icnPath + "tube-inactive.svg",
                                        "FOCUS");
    itemMap["TSLIT"] =   new OpticsItem(icnPath + "slit-active.svg",
                                        icnPath + "slit-inactive.svg",
                                        "TSLIT");
    itemMap["HSLIT"] =   new OpticsItem(icnPath + "slit-active.svg",
                                        icnPath + "slit-inactive.svg",
                                        "HSLIT");
    itemMap["PCOLL"] =   new OpticsItem(icnPath + "soller-active.svg",
                                        icnPath + "soller-inactive.svg",
                                        "PCOLL");
    itemMap["VSLIT"] =   new OpticsItem(icnPath + "slit-active.svg",
                                        icnPath + "slit-inactive.svg",
                                        "VSLIT");
    itemMap["RSLIT"] =   new OpticsItem(icnPath + "pinhole-active.svg",
                                        icnPath + "pinhole-inactive.svg",
                                        "RSLIT");
    itemMap["AIRSCAT"] = new OpticsItem(icnPath + "airscat-active.svg",
                                        icnPath + "airscat-inactive.svg",
                                        "AIRSCAT");
    itemMap["SSLIT"] =   new OpticsItem(icnPath + "slit-active.svg",
                                        icnPath + "slit-inactive.svg",
                                        "SSLIT");
    itemMap["SCOLL"] =   new OpticsItem(icnPath + "soller-active.svg",
                                        icnPath + "soller-inactive.svg",
                                        "SCOLL");
    itemMap["DSLIT"] = new OpticsItem(icnPath + "slit-active.svg",
                                        icnPath + "slit-inactive.svg",
                                        "DSLIT");
    itemMap["MONOCHROMATOR"] = new OpticsItem(icnPath + "monochromator-active.svg",
                                              icnPath + "monochromator-inactive.svg",
                                              "MONOCHROMATOR");
    itemMap["DETECTOR"] = new OpticsItem(icnPath + "detector-active.svg",
                                         icnPath + "detector-inactive.svg",
                                         "DETECTOR");

    itemMap["FOCUS"]->setRotation(angle - 6.0);
    itemMap["TSLIT"]->setRotation(angle + 90.0);
    itemMap["HSLIT"]->setRotation(angle);
    itemMap["PCOLL"]->setRotation(angle);
    itemMap["VSLIT"]->setRotation(angle + 90.0);
    itemMap["RSLIT"]->setRotation(angle);
    itemMap["SSLIT"]->setRotation(-angle);
    itemMap["SCOLL"]->setRotation(-angle);
    itemMap["DSLIT"]->setRotation(-angle);
    itemMap["MONOCHROMATOR"]->setRotation(-2.0 * angle);
    itemMap["DETECTOR"]->setRotation(-3.0 * angle);

    sampleRefl->setPos(-sampleRefl->boundingRect().width() / 2.0, 0.0);
    sampleCapil->setPos(-sampleCapil->boundingRect().width() / 2.0, -sampleCapil->boundingRect().height() / 2.0);
    sampleTrans->setPos(-sampleTrans->boundingRect().width() / 2.0, -sampleTrans->boundingRect().height() / 2.0);

    itemMap["FOCUS"]->setPos(-0.938*radius, -0.580*radius);
    itemMap["TSLIT"]->setPos(-0.617*radius, -0.432*radius);
    itemMap["HSLIT"]->setPos(-0.603*radius, -0.423*radius);
    itemMap["PCOLL"]->setPos(-0.453*radius, -0.318*radius);
    itemMap["VSLIT"]->setPos(-0.183*radius, -0.181*radius);
    itemMap["RSLIT"]->setPos(-0.172*radius, -0.174*radius);
    itemMap["AIRSCAT"]->setPos(-0.024*radius, -0.174*radius);
    itemMap["SSLIT"]->setPos(0.107*radius, -0.137*radius);
    itemMap["SCOLL"]->setPos(0.361*radius, -0.264*radius);
    itemMap["DSLIT"]->setPos(0.778*radius, -0.524*radius);
    itemMap["MONOCHROMATOR"]->setPos(0.943*radius, -0.502*radius);
    itemMap["DETECTOR"]->setPos(0.957*radius, -0.731*radius);

    itemMap["FOCUS"]->setToolTip(QStringLiteral("X-ray tube"));
    itemMap["TSLIT"]->setToolTip(QStringLiteral("Axial beam slit"));
    itemMap["HSLIT"]->setToolTip(QStringLiteral("Divergence slit"));
    itemMap["PCOLL"]->setToolTip(QStringLiteral("Primary-beam collimator"));
    itemMap["VSLIT"]->setToolTip(QStringLiteral("Beam mask"));
    itemMap["RSLIT"]->setToolTip(QStringLiteral("Pinhole aperture"));
    itemMap["AIRSCAT"]->setToolTip(QStringLiteral("Beam knife"));
    itemMap["SSLIT"]->setToolTip(QStringLiteral("Anti-scatter slit"));
    itemMap["SCOLL"]->setToolTip(QStringLiteral("Secondary-beam collimator"));
    itemMap["DSLIT"]->setToolTip(QStringLiteral("Detector slit"));
    itemMap["MONOCHROMATOR"]->setToolTip(QStringLiteral("Monochromator"));
    itemMap["DETECTOR"]->setToolTip(QStringLiteral("Detector"));

    itemMap["DETECTOR"]->setZValue(99);

    QMapIterator<QString, OpticsItem*> it(itemMap);
    while (it.hasNext()) {
        it.next();
        it.value()->setDarkMode(isDarkMode);
        addItem(it.value());
    }

    sampleCapil->setVisible(false);
    sampleTrans->setVisible(false);

    itemMap["SAMPLE"] = sampleRefl;

    addItem(sampleRefl);
    addItem(sampleCapil);
    addItem(sampleTrans);
}

void InstrumentScene::addBackgroundElements()
{
    QPen linePen(isDarkMode ? Qt::lightGray : Qt::darkGray, 2.0, Qt::DashLine, Qt::RoundCap);

    QGraphicsEllipseItem *circle = addEllipse(-radius, -radius, 2.0 * radius, 2.0 * radius, linePen);
    circle->setStartAngle(0);
    circle->setSpanAngle(180*16);
    circle->setToolTip(QStringLiteral("Goniometer"));

    addLine(-1.15 * radius, 0.0, -radius, 0.0, linePen);
    addLine( 1.20 * radius, 0.0,  radius, 0.0, linePen);
}

void InstrumentScene::addRays()
{
    double beamSpread = 0.08 * radius;
    QPen rayPen(isDarkMode ? QColor(255, 128, 255) : QColor(255, 0, 0), 2.0, Qt::DashLine, Qt::RoundCap);

    double focX = qCos(qDegreesToRadians(angle)) * radius;
    double focY = -qAbs(qSin(qDegreesToRadians(angle)) * radius);
    double dx = beamSpread;
    double dy1 =  focY * dx / (focX + dx);
    double dy2 = -focY * dx / (focX - dx);

    QGraphicsLineItem *primReflIt1 = new QGraphicsLineItem(0.0, 0.0, -focX, focY);
    QGraphicsLineItem *primReflIt2 = new QGraphicsLineItem(-dx, 0.0, -focX, focY);
    QGraphicsLineItem *primReflIt3 = new QGraphicsLineItem( dx, 0.0, -focX, focY);
    QGraphicsLineItem *secReflIt1  = new QGraphicsLineItem(0.0, 0.0,  focX, focY);
    QGraphicsLineItem *secReflIt2  = new QGraphicsLineItem(-dx, 0.0,  focX, focY);
    QGraphicsLineItem *secReflIt3  = new QGraphicsLineItem( dx, 0.0,  focX, focY);

    primReflIt1->setPen(rayPen);
    primReflIt2->setPen(rayPen);
    primReflIt3->setPen(rayPen);
    secReflIt1->setPen(rayPen);
    secReflIt2->setPen(rayPen);
    secReflIt3->setPen(rayPen);

    raysPrimRefl.append(primReflIt1);
    raysPrimRefl.append(primReflIt2);
    raysPrimRefl.append(primReflIt3);
    raysSecRefl.append(secReflIt1);
    raysSecRefl.append(secReflIt2);
    raysSecRefl.append(secReflIt3);

    QGraphicsLineItem *primCapIt1 = new QGraphicsLineItem(0.0, 0.0, -focX, focY);
    QGraphicsLineItem *primCapIt2 = new QGraphicsLineItem(0.0, dy1, -focX, focY);
    QGraphicsLineItem *primCapIt3 = new QGraphicsLineItem(0.0, dy2, -focX, focY);
    QGraphicsLineItem *secCapIt1 = new QGraphicsLineItem(0.0, 0.0, focX, focY);
    QGraphicsLineItem *secCapIt2 = new QGraphicsLineItem(0.0, dy1, focX, focY);
    QGraphicsLineItem *secCapIt3 = new QGraphicsLineItem(0.0, dy2, focX, focY);

    primCapIt1->setPen(rayPen);
    primCapIt2->setPen(rayPen);
    primCapIt3->setPen(rayPen);
    secCapIt1->setPen(rayPen);
    secCapIt2->setPen(rayPen);
    secCapIt3->setPen(rayPen);

    raysPrimCapTrans.append(primCapIt1);
    raysPrimCapTrans.append(primCapIt2);
    raysPrimCapTrans.append(primCapIt3);
    raysSecCapTrans.append(secCapIt1);
    raysSecCapTrans.append(secCapIt2);
    raysSecCapTrans.append(secCapIt3);

    QGraphicsLineItem *focIt1 = new QGraphicsLineItem(focX, focY, 1.000 * radius, -0.577 * radius);
    QGraphicsLineItem *focIt2 = new QGraphicsLineItem(focX, focY, 1.007 * radius, -0.590 * radius);
    QGraphicsLineItem *focIt3 = new QGraphicsLineItem(focX, focY, 0.993 * radius, -0.566 * radius);
    QGraphicsLineItem *monIt1 = new QGraphicsLineItem(1.000 * radius, -0.577 * radius, 1.0 * radius, -0.734 * radius);
    QGraphicsLineItem *monIt2 = new QGraphicsLineItem(1.007 * radius, -0.590 * radius, 1.0 * radius, -0.734 * radius);
    QGraphicsLineItem *monIt3 = new QGraphicsLineItem(0.993 * radius, -0.566 * radius, 1.0 * radius, -0.734 * radius);

    focIt1->setPen(rayPen);
    focIt2->setPen(rayPen);
    focIt3->setPen(rayPen);
    monIt1->setPen(rayPen);
    monIt2->setPen(rayPen);
    monIt3->setPen(rayPen);

    raysFocus.append(focIt1);
    raysFocus.append(focIt2);
    raysFocus.append(focIt3);
    raysMono.append(monIt1);
    raysMono.append(monIt2);
    raysMono.append(monIt3);

    for (int i = 0; i < raysPrimRefl.size(); ++i) addItem(raysPrimRefl.at(i));
    for (int i = 0; i < raysSecRefl.size(); ++i)  addItem(raysSecRefl.at(i));

    for (int i = 0; i < raysPrimCapTrans.size(); ++i) raysPrimCapTrans.at(i)->setVisible(false);
    for (int i = 0; i < raysSecCapTrans.size(); ++i)  raysSecCapTrans.at(i)->setVisible(false);

    for (int i = 0; i < raysPrimCapTrans.size(); ++i) addItem(raysPrimCapTrans.at(i));
    for (int i = 0; i < raysSecCapTrans.size(); ++i)  addItem(raysSecCapTrans.at(i));

    for (int i = 0; i < raysFocus.size(); ++i) addItem(raysFocus.at(i));
    for (int i = 0; i < raysMono.size(); ++i)  addItem(raysMono.at(i));
}

/*
 * here we react to individual scene elements requesting changes.
 * For example, changing position or loading different svg files.
 */
void InstrumentScene::updateSceneElement(const QString &s, const QVariant &v)
{
    if (!itemMap.contains(s)) return;

    if (s == "SAMPLE") {
        placeSample(v.toString());
    }
}

void InstrumentScene::itemActiveStatusChanged(const QString &s, bool b)
{
    if (itemMap.contains(s)) itemMap.value(s)->setInstalled(b);

    if ((s == "MONOCHROMATOR") || (s == "DSLIT")) {
          placeDetector();
    }
}

void InstrumentScene::setItemSelected(const QString &s)
{
    QMapIterator<QString, OpticsItem*> it(itemMap);

    while (it.hasNext()) {
        it.next();
        it.value()->setHighlighted(it.key() == s);
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

void InstrumentScene::placeDetector()
{
    bool oldState = blockSignals(true);

    if (itemMap.value("MONOCHROMATOR")->isVisible()) {
        itemMap["DETECTOR"]->setPos(0.957*radius, -0.731*radius);
        itemMap["DETECTOR"]->setRotation(-3.0*angle);
        for (int i = 0; i < raysFocus.size(); ++i) raysFocus.at(i)->setVisible(true);
        for (int i = 0; i < raysMono.size(); ++i) raysMono.at(i)->setVisible(true);
    } else if (itemMap.value("DSLIT")->isVisible()) {
        itemMap["DETECTOR"]->setPos(0.934*radius, -0.595*radius);
        itemMap["DETECTOR"]->setRotation(-angle);
        for (int i = 0; i < raysFocus.size(); ++i) raysFocus.at(i)->setVisible(true);
        for (int i = 0; i < raysMono.size(); ++i) raysMono.at(i)->setVisible(false);
    } else {
        itemMap["DETECTOR"]->setPos(0.842*radius, -0.538*radius);
        itemMap["DETECTOR"]->setRotation(-angle);
        for (int i = 0; i < raysFocus.size(); ++i) raysFocus.at(i)->setVisible(false);
        for (int i = 0; i < raysMono.size(); ++i) raysMono.at(i)->setVisible(false);
    }

    blockSignals(oldState);
}

void InstrumentScene::placeSample(const QString &g)
{
    bool oldState = blockSignals(true);
    bool hl = itemMap["SAMPLE"]->isHighlighted();
    bool isRefl = (g == "REFLEXION") || (g == "");
    bool isCap  = (g == "CAPILLARY");
    bool isTrans = (g == "TRANSMISSION");

    if (isCap) {
        itemMap["SAMPLE"] =  sampleCapil;
    } else if (isTrans) {
        itemMap["SAMPLE"] =  sampleTrans;
    } else {
        itemMap["SAMPLE"] =  sampleRefl;
    }

    itemMap["SAMPLE"]->setHighlighted(hl);

    sampleRefl->setVisible(isRefl);
    sampleCapil->setVisible(isCap);
    sampleTrans->setVisible(isTrans);

    for (int i = 0; i < raysPrimRefl.size(); ++i) raysPrimRefl.at(i)->setVisible(isRefl);
    for (int i = 0; i < raysSecRefl.size(); ++i)  raysSecRefl.at(i)->setVisible(isRefl);
    for (int i = 0; i < raysPrimCapTrans.size(); ++i) raysPrimCapTrans.at(i)->setVisible(!isRefl);
    for (int i = 0; i < raysSecCapTrans.size(); ++i)  raysSecCapTrans.at(i)->setVisible(!isRefl);

    blockSignals(oldState);
}

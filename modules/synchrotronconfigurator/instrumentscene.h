/***************************************************************************
                          instrumentscene.h  -  description
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

#ifndef INSTRUMENTSCENE_H
#define INSTRUMENTSCENE_H

#include "opticsitem.h"
#include <QGraphicsScene>
#include <QMap>
#include <QVector>

class InstrumentScene : public QGraphicsScene
{
    Q_OBJECT
public:
    InstrumentScene(bool darkMode = false);

public slots:
    void itemActiveStatusChanged(const QString &, bool);
    void setItemSelected(const QString &);
    void updateSceneElement(const QString &, const QVariant &);
    void setDetectorTilt(double);

private:
    QMap<QString, QGraphicsItem *> itemMap;

    QVector<QGraphicsLineItem *> raysPrim;
    QVector<QGraphicsLineItem *> raysSec;
    QGraphicsItemGroup *detectorGroup;

    int radius;
    bool isDarkMode;
    double detectorTilt;

    void addBackgroundElements();
    void addOpticalElements();
    void addRays();
    QGraphicsItemGroup * createDetectorGroup();

private slots:
    void updateSelection();

signals:
    void itemSelected(QString);
};


#endif // INSTRUMENTSCENE_H

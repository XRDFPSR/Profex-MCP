/***************************************************************************
                          baselinedialog.h  -  description
                             -------------------
    begin                : Tue Jan 23 22:00:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#ifndef BASELINEDIALOG_H
#define BASELINEDIALOG_H

#include <QDialog>
#include <QVector>
#include <QTableWidgetItem>
#include "tools/abstracttooldialog.h"
#include "projectWidget/projectwidget.h"
#include "../libXrdIO/scan.h"

namespace Ui {
class BaseLineDialog;
}

class BaseLineDialog : public AbstractToolDialog
{
    Q_OBJECT

public:
    explicit BaseLineDialog(QWidget *parent = 0);
    ~BaseLineDialog();

    void clearTemporary();

private:
    Ui::BaseLineDialog *ui;
    QStringList headers;
    QList<QStringList> prevManualAnchorPoints;

    void preSetProject(ProjectWidget *);
    void postSetProject(ProjectWidget *);
    void clearGui();
    void initSettings();
    void saveSettings();
    void parseScans();
    void keepTemporary();
    void blockUpdateSignals(bool);
    void setAnchorTable(const QVector<QPointF> &);
    void interpolateAnchorPoints(Scan &, const QVector<global::AnchorPoint> &anch, int ipol);
    void clearAnchorPoints();
    QVector<global::AnchorPoint> tableToAnchors();

private slots:
    void updateView();
    void algoChanged(int);
    void computeCurve();
    void computeSnip();
    void computeGolot();
    void computeManual();
    void generateCurve();
    void generateGolot();
    void generateManual();
    void append();
    void resetSnip();
    void resetGolot();
    void resetManual();
    void addAnchor();
    void removeAnchor();
    void highlightAnchor(QTableWidgetItem *, QTableWidgetItem *);
    void addManualAnchor(double, double, double);
    void moveAnchor(int, double, double);
    void removeAnchorIndex(int);
    void optimizeManual();

signals:
    void updateScans();
};

#endif // BASELINEDIALOG_H

/***************************************************************************
                          peakintegrationwidget.h  -  description
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

#ifndef PEAKINTEGRATIONWIDGET_H
#define PEAKINTEGRATIONWIDGET_H

#include <QTableWidget>
#include <QToolButton>
#include <QDomDocument>
#include "../libXrdIO/settingsmanager.h"
#include "graphWidget/abstractgraphview.h"
#include "graphWidget/graphwindow.h"
#include "../libXrdIO/scan.h"
#include "../libXrdIO/structs.h"

namespace Ui {
class PeakIntegrationWidget;
}

class PeakIntegrationWidget : public AbstractGraphView
{
    Q_OBJECT
public:
    explicit PeakIntegrationWidget(GraphDataController *c, GraphWindow *v, QWidget *parent = nullptr);
    ~PeakIntegrationWidget();

    void getPreset(QDomDocument &);
    void applyPreset(const QDomElement &);
    void appendRange(const QString &, double, double, bool update = true);

    QString getDataCsv();
    void setRangeData(const QList<global::PeakRange> &, bool clear = true);
    QList<global::PeakRange> getRangeData();
    void setSampleDisplacements(double, double, double);

    bool hasData() const;

public slots:
    void updateView();

private:
    Ui::PeakIntegrationWidget *ui;

    GraphWindow *graphView;
    QSignalMapper *mapper;

    QStringList defaultHeaders;
    QStringList headers;
    QString unit;
    int previousNumberOfScans;
    double rangeMin;
    double rangeMax;
    bool subtractBackground;

    void initSettings();
    bool rangeBoundaries(int i, double &start, double &end);
    void computeRow(int);
    void displayRegions();
    double angularCorrection(double);
    void centerRangeMaxIntens(int, const Scan *);

private slots:
    void compute(int row = -1);
    void initTable();
    void addRange(bool b);
    void removeRange();
    void removeAll();
    void rangeChanged(int);
    void applyToAll();
    void toggleBackground(bool);
    void cellContentChanged(int, int);
    void paste();
    void centerRanges();
    void rangePoints(QPointF, QPointF, QUuid);
    void cursorMessage(QString, QUuid);
    void updateDataRange();
    void headerResized();
    void bkgrButtonToggled();
    void showHelp();

signals:
    void sigApplyToAll();
    void helpText(QString);
};

#endif // PEAKINTEGRATIONWIDGET_H

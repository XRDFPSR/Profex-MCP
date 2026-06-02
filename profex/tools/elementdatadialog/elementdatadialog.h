/***************************************************************************
                          elementdatadialog.h  -  description
                             -------------------
    begin                : Wed Feb 10 19:16:07 CET 2021
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

#ifndef ELEMENTDATADIALOG_H
#define ELEMENTDATADIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include "../../../libXrdIO/elementscatteringdatamanager.h"
#include "../../../libXrdIO/settingsmanager.h"
#include "../../3rdparty/qcustomplot/qcustomplot.h"

namespace Ui {
class ElementDataDialog;
}

enum DataType {F0, F1, F2, F1F2, MAC, LAC};

struct Parameter {
    QString label;
    QString unit;
    bool log;
    DataType type;

    Parameter(QString v, QString u, bool l, DataType d) : label(v), unit(u), log(l), type(d) {}
};

class ElementDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ElementDataDialog(QWidget *parent = nullptr);
    ~ElementDataDialog();

private:
    Ui::ElementDataDialog *ui;
    SettingsManager *settings;
    ElementScatteringDataManager scatData;
    int lineWidth;
    QList<ElementScatteringData> data;
    QList<QColor> colorTable;
    QSharedPointer<QCPAxisTicker> defaultTicker;
    QSharedPointer<QCPAxisTickerLog> logTicker;
    QList<Parameter> axLbl;
    bool xAxisWl;

    void initWavelength();
    void initSettings();
    void saveSettings();
    void initDisplay();
    void initColorTable(const QList<QVariant> &);
    QColor getColor(int);

    void updateDisplayGraph();
    void updateDisplayLists();
    void setupXAxis();
    void setupYAxis(int);
    void addDataGraph(const QString &e, int z, const QVector<double> &x, const QVector<double> &y, int colIdx, double &mi, double &ma);
    void addWlGraph(const QString &s, double e, double yMin, double yMax, int colIdx);
    void updateAbsWidget(const QStringList &, const QList<int> &, const QList<double> &, const QList<double> &, const QList<double> &, const QList<double> &);

private slots:
    void elementSelected();
    void dataSelected(int);
    void emissionLineChanged(int);
    void energyChanged(double);
    void wavelengthChanged(double);
    void saveMdr();
    void saveAno();
    void saveXy();
    void savePdf();
    void cursorCoordinates(QMouseEvent *);
    void displayWavelengthChanged();
    void tabToggled(int);
    void toggleCustomWl(bool);
    void resetPlotZoom(QMouseEvent*);
    void xunitChanged();
    void splitterXchanged(int,int);
    void splitterYchanged(int,int);
};

#endif // ELEMENTDATADIALOG_H

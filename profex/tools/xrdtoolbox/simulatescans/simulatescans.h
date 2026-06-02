/***************************************************************************
                          simulatescans.h  -  description
                             -------------------
    begin                : Sat Jun 24 11:00:00 CEST 2017
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

#ifndef SIMULATESCANS_H
#define SIMULATESCANS_H

#include <QDialog>
#include <QVector>
#include <QElapsedTimer>
#include <QSettings>
#include <QListWidgetItem>
#include <QTableWidgetItem>
#include <QFileInfo>
#include "../libXrdIO/settingsmanager.h"
#include "ui_simulatescans.h"

namespace Ui {
class SimulateScans;
}

struct PhaseContrib {
    QString file;
    double quantity;
};

class SimulateScans : public QDialog
{
    Q_OBJECT

public:
    explicit SimulateScans(QWidget *parent = 0);
    ~SimulateScans();

private:
    Ui::SimulateScans *ui;
    QElapsedTimer seedTime;
    SettingsManager *settings;
    QList<QStringList> defaultQuant;
    bool isRunning;
    bool abort;

    void initSettings();
    void saveSettings();

    void execSimulations();
    double quantValue(int, int);
    double macPhase(int);
    double macSample(int);
    QFileInfo fileInfo(int, int);
    PhaseContrib getPhaseContrib(int, int, int);
    QVector< QVector<double> > readFile(const QString &);
    void writeFile(const QString &, const QVector< QVector<double> > &);
    QVector< QVector<double> > applyNoise(const QVector< QVector<double> > &);
    void mergeFiles(QVector< QVector<double> > &, const QVector< QVector<double> > &, double);
    QString macToString();
    void updateRefScans(const QStringList &);
    QStringList getScanFileNames();
    void addTableRow(const QString &);
    void removeTableRow(const QString &);
    QString getDefaultCellText(int r, int c);
    void disableGUI(bool);
    bool checkMacs();

private slots:
    void close();
    void runSynthesis();
    void updateTotalInt();
    void addPhase();
    void removePhase();
    void addScan();
    void removeScan();
    void macChanged(double);
    void currentPhaseChanged(QListWidgetItem *, QListWidgetItem *);
    void currentScanChanged(QListWidgetItem *, QListWidgetItem *);
    void phaseQuantityChanged(QTableWidgetItem *);
    void logMessage(const QString &);
    void numberOfCompositionsChanged(int);

signals:
    void emitLogMessage(const QString &);
};

#endif // SIMULATESCANS_H

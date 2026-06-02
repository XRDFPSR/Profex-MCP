/***************************************************************************
                          mainwindow.h  -  description
                             -------------------
    begin                : Wed Dec 11 18:00:00 CEST 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "../libXrdIO/settingsmanager.h"
#include "modelmanagerchernyshov.h"
#include "instrumentscene.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool doShowMaximized();
    void loadFileList(const QStringList &) {}
    inline void setLogDest(const QString &d) {logDest = d;}
    inline void setWorkingDir(const QString &d) {_workingDir = d;}

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *);

private:
    Ui::MainWindow *ui;
    SettingsManager *settings;
    QString logDest;
    QLabel *labelStatus;
    QString _workingDir;
    QString _configFileName;
    ModelManagerChernyshov *modelManagerChernyshov;
    InstrumentScene *scene;
    double customLac;
    QMap<QString, QTreeWidgetItem *> configItemMap;

    void initSettings();
    void saveSettings();
    void initDetectors();
    void initConnections();
    void initTreeWidgetItems();
    void setPeakParametersChernyshov();
    void loadHelpText(const QString &);

private slots:
    void loadFile();
    void importPeakList();
    void saveAs();
    void save();
    void saveGeqFile();
    void helpAbout();
    void runProfileCalculation();
    void generateAllProfiles();
    void fitAllProfiles();
    void fitFundamentalParameters();
    void updateFwhmParametersChernyshov();
    void updateRangeParameters();
    void updateProfileParameters();
    void updateAbsorptionCoefficient();
    void customAbsorptionCoefficient();
    void preferences();
    void exportCurves();
    void raiseParametersPage(QTreeWidgetItem *, QTreeWidgetItem *);
    void instrumentItemSelected(QString);
    void fitComplete();
    void coordinatesFwhm(QMouseEvent *);
    void coordinatesProfile(QMouseEvent *);
    void toggleTransparencyCutoffEnabled(int);
    void detectorTiltChanged(double);
};

#endif // MAINWINDOW_H

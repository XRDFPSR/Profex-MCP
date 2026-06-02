/***************************************************************************
                          mainwindow.h  -  description
                             -------------------
    begin                : Mon Sep 22 09:00:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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
#include <QElapsedTimer>
#include <QFileInfo>
#include <QProgressDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include "threadrenderemap.h"
#include "emapwidget2d.h"
#include "emapscale2d.h"
#include "emapdatahandler.h"
#include "../libXrdIO/colorMaps/lutstructs.h"
#include "emapstructs.h"
#include "../libXrdIO/settingsmanager.h"
#include "emapexportcoordinatedialog.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setFileName(const QString &);
    bool doShowMaximized();
    void loadFileList(const QStringList &) {}
    inline void setLogDest(const QString &d) {logDest = d;}
    inline void setWorkingDir(const QString &d) {workingDir = d;}

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *);

private:
    Ui::MainWindow *ui;
    SettingsManager *settings;

    QLabel *statusCoordinates;
    QComboBox *comboBoxSynthesis;
    QComboBox *comboBoxProjection;
    QComboBox *comboBoxColorMap;
    QDoubleSpinBox *doubleSpinBoxRange;
    QCheckBox *checkBoxAutoRange;
    QString logDest;
    QString workingDir;
    QFileInfo hklFileName;
    QFileInfo cellFileName;
    QMap<QString, colorMaps::Lut> lutList;
    float dmax, dmin, dmaxStored;
    EDataMap vmap;
    QList<Atom> atoms;
    int activeThreads;
    int nextZLevel;
    int nCpus;
    int bufferMode;
    int levelsX;
    int levelsY;
    int levelsZ;
    QElapsedTimer startTime;
    bool abort;
    QProgressDialog *pdlg;
    UnitCell cell;
    QMatrix4x4 mf2c; // fractional -> cartesian
    projection_t projection;
    synthesis_t synthesis;
    EMapDataHandler dataHandler;
    EMapExportCoordinateDialog *exportCoordinateDialog;

    void initSettings();
    void saveSettings();
    void initGuiElements();
    void initConnections();
    void exportRawMapVolume(int mode, const QString &fname, int format); // mode 0 = fractional, mode 1 = cartesian
    void exportRawMapLevels(int mode, const QString &fname, int format, projection_t prj, int);

    void dragEnterEvent(QDragEnterEvent *);
    void dropEvent(QDropEvent *);
    void mousePressEvent(QMouseEvent *);
    void startFourierSynth();

private slots:
    void loadFile();
    void saveCurrent();
    void saveAll();

    void calculateMap();
    void currentLevelChanged(int);
    void fourierSyntThreadCompleted();
    void renderImages();

    void coordinates(float,float,float);
    void setGuiState(bool);

    void newDmax(float);
    void newDmin(float);
    void dmaxChanged(double);
    void abortCalculation();
    void setProjection(int);
    void lutChanged(QString);
    void preferences();
    void helpAbout();
    void autoRangeToggled(bool);

    void saveRawMap();
};

#endif // MAINWINDOW_H

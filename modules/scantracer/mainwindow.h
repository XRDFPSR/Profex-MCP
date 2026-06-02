/***************************************************************************
                          mainwindow.h  -  description
                             -------------------
    begin                : Wed May 12 19:00:00 CEST 2021
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsItem>
#include <QTransform>
#include <QLabel>

#include "scantracerscene.h"
#include "scantracerimagetracer.h"
#include "scantracercalibinfowidget.h"

#include "../libXrdIO/settingsmanager.h"

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

    ScanTracerScene *_scene;
    ScanTracerImageTracer *_tracer;
    QLabel *labelStatus;

    QList<QPointF> _calibPointsPx;
    QList<QPointF> _calibPointsCalib;
    QTransform _pxToUnity;
    QTransform _unityToCoordinates;
    QPolygonF _traceCoord;
    QString _workingDir;
    QColor _scanColor;
    QColor _backgroundColor;

    void initSettings();
    void saveSettings();
    void blockSpinBoxSignals(bool);
    void updateCalibPointTR();
    QTransform getPxToUnity();
    QTransform getUnityToCoordinates();
    void addCalibPoint(const QPointF &);
    void loadImageFromDisk(const QString &);
    void setImageLimits();
    void clearCalibData();

    void resetGui();
    void resetCalibPage();
    void resetBaseLinePage();
    void resetTracePage();
    void resetSavePage();

private slots:
    void loadFile();
    void pasteFile();
    void saveAs();
    void setCalibMode(bool);
    void setBaseLineMode(bool);
    void trace();
    void mouseClickPosition(QPointF);
    void mouseCoordinates(QPointF);
    void updateMatrix();
    void calibBLChanged();
    void calibBRChanged();
    void calibTLChanged();
    void calibCoordinatesChanged();
    void scanColorDialog();
    void backgroundColorDialog();
    void pickScanColor(bool);
    void pickBackgroundColor(bool);
    void nextPage();
    void scanColorPicked(QColor);
    void backgroundColorPicked(QColor);
    void colorSensitivity(int);
    void lineCenter(int);
    void helpAbout();
};

#endif // MAINWINDOW_H

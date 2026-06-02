/***************************************************************************
                          mainwindow.cpp  -  description
                             -------------------
    begin                : Thu Apr 14 18:00:00 CEST 2011
    copyright            : (C) 2011 by Nicola Doebelin
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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGuiApplication>
#include <QMainWindow>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QMimeData>
#include <QClipboard>
#include <QColorDialog>

#include "helpaboutdialog.h"
#include "../libXrdIO/bgmnfileio.h"

/* calibration points:
 *
 * TL (2) ------------- TR (calculated)
 *   |                    |
 *   |                    |
 *   |                    |
 *   |                    |
 * BL (0) ------------- BR (1)
 */

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    settings = SettingsManager::getInstance();
    settings->initialize();

    QString iconTheme = settings->value("Gui/iconTheme", QString()).toString();

    if (iconTheme.isEmpty()) {
        if (settings->isDarkMode()) iconTheme = QString("profex-dark");
        else                        iconTheme = QString("profex-light-colored");
    }

    QIcon::setThemeName(iconTheme);

    ui->setupUi(this);
    setWindowTitle(QString("Profex Scan Tracer"));

    labelStatus = new QLabel(statusBar());
    labelStatus->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    statusBar()->addPermanentWidget(labelStatus, 0);

    _workingDir = QString();
    _tracer = new ScanTracerImageTracer();
    _scene = new ScanTracerScene();
    _tracer->setScene(_scene);
    ui->graphicsViewGraph->setScene(_scene);
    connect(_scene, SIGNAL(sigMouseClickPosition(QPointF)), this, SLOT(mouseClickPosition(QPointF)));
    connect(_scene, SIGNAL(sigMouseCoordinates(QPointF)), this, SLOT(mouseCoordinates(QPointF)));

    _pxToUnity          = QTransform(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
    _unityToCoordinates = QTransform(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);

    ui->splitter->setSizes(QList<int>() << 10 << 90);
    labelStatus->clear();
    ui->labelColorScan->clear();
    ui->labelColorBackground->clear();

    ui->lineEditBLX->setValidator(new QDoubleValidator(-0.01, 180.0, 4));
    ui->lineEditBRX->setValidator(new QDoubleValidator(-0.01, 180.0, 4));
    ui->lineEditTLX->setValidator(new QDoubleValidator(-0.01, 180.0, 4));

    ui->lineEditBLY->setValidator(new QDoubleValidator(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(), 2));
    ui->lineEditBRY->setValidator(new QDoubleValidator(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(), 2));
    ui->lineEditTLY->setValidator(new QDoubleValidator(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(), 2));

    connect(ui->lineEditBLX, SIGNAL(textEdited(QString)), this, SLOT(calibCoordinatesChanged()));
    connect(ui->lineEditBLY, SIGNAL(textEdited(QString)), this, SLOT(calibCoordinatesChanged()));
    connect(ui->lineEditBRX, SIGNAL(textEdited(QString)), this, SLOT(calibCoordinatesChanged()));
    connect(ui->lineEditBRY, SIGNAL(textEdited(QString)), this, SLOT(calibCoordinatesChanged()));
    connect(ui->lineEditTLX, SIGNAL(textEdited(QString)), this, SLOT(calibCoordinatesChanged()));
    connect(ui->lineEditTLY, SIGNAL(textEdited(QString)), this, SLOT(calibCoordinatesChanged()));
    connect(_scene, SIGNAL(sigScanColorPicked(QColor)), this, SLOT(scanColorPicked(QColor)));
    connect(_scene, SIGNAL(sigBackgroundColorPicked(QColor)), this, SLOT(backgroundColorPicked(QColor)));

    initSettings();

    resetCalibPage();
    resetBaseLinePage();
    resetTracePage();
    resetSavePage();
    resetGui();
}

MainWindow::~MainWindow()
{
    if (_tracer) delete _tracer;
    delete ui;
    settings->destroy();
}

bool MainWindow::doShowMaximized()
{
    return settings->value("Gui/maximized", false).toBool();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    saveSettings();
    settings->sync();
    e->accept();
}

void MainWindow::initSettings()
{
    bool dm = settings->isDarkMode();
    restoreGeometry(settings->value("Gui/geometry", QByteArray()).toByteArray());
    ui->splitter->restoreState(settings->value("Gui/splitter", QByteArray()).toByteArray());
    ui->doubleSpinBoxStepSize->setValue(settings->value("Trace/stepSize", 0.02).toDouble());
    if (_workingDir.isEmpty()) _workingDir = settings->value("Gui/workingDir", QDir::homePath()).toString();
    if (dm) _scene->setBackgroundBrush(QBrush(QGuiApplication::palette().color(QPalette::Base)));
    _scanColor = QColor(settings->value("Trace/scanColor", "#000000").toString());
    _backgroundColor = QColor(settings->value("Trace/backgroundColor", "#FFFFFF").toString());
    int colSens = settings->value("Trace/colorSensitivity", 4).toInt();
    int lineCentMode = settings->value("Trace/lineCenterMode", 1).toInt();

    ui->labelColorScan->setStyleSheet(QString("background-color:%1").arg(_scanColor.name()));
    ui->labelColorBackground->setStyleSheet(QString("background-color:%1").arg(_backgroundColor.name()));

    ui->spinBoxColorSensitivity->setValue(colSens);
    ui->comboBoxLineCenter->setCurrentIndex(lineCentMode);

    ui->checkBoxAllowNegative->setChecked(settings->value("Trace/allowNegativeIntensities", false).toBool());

    if (_tracer) {
        _tracer->setScanColor(_scanColor);
        _tracer->setBackgroundColor(_backgroundColor);
        _tracer->setColorSensitivity(colSens);
        _tracer->setLineCenterMode(lineCentMode);
    }
}

void MainWindow::saveSettings()
{
    settings->setValue("Gui/geometry", saveGeometry());
    settings->setValue("Gui/splitter", ui->splitter->saveState());
    settings->setValue("Trace/stepSize", ui->doubleSpinBoxStepSize->value());
    settings->setValue("Trace/workingDir", _workingDir);
    settings->setValue("Trace/scanColor", _scanColor.name());
    settings->setValue("Trace/backgroundColor", _backgroundColor.name());
    settings->setValue("Trace/colorSensitivity", ui->spinBoxColorSensitivity->value());
    settings->setValue("Trace/lineCenterMode", ui->comboBoxLineCenter->currentIndex());
    settings->setValue("Trace/allowNegativeIntensities", ui->checkBoxAllowNegative->isChecked());
}

void MainWindow::loadFile()
{
    QString px = QFileDialog::getOpenFileName(this,
                                              tr("Open XRD scan image"),
                                              _workingDir,
                                              tr("Image files (*.jpg *.JPG *.png *.PNG *.bmp *.BMP"));

    if (px.isEmpty()) return;
    QFileInfo fi(px);
    _workingDir = fi.absolutePath();
    loadImageFromDisk(px);
}

void MainWindow::pasteFile()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasImage()) {
        _scene->setPixmap(qvariant_cast<QPixmap>(mimeData->imageData()));
        setImageLimits();
    }
}

void MainWindow::saveAs()
{
    QString f = QFileDialog::getSaveFileName(this, tr("Save scan to file"), QDir::homePath(), "ASCII free format (*.xy *.XY)");
    if (f.isEmpty()) return;

    int si = ui->spinBoxSubtractIntensity->value();
    bool neg = ui->checkBoxAllowNegative->isChecked();
    QString out;

    for (int i = 0; i < _traceCoord.size(); ++i) {
        double dx = _traceCoord.at(i).x();
        double dy = _traceCoord.at(i).y() - si;
        if ((dy < 0.0) && !neg) dy = 0.0;

        out += QString("%1 %2\n").arg(dx, 0, 'f', 6).arg(dy, 0, 'f', 4);
    }

    BgmnFileIO::writeTextFile(f, out);
}


void MainWindow::loadImageFromDisk(const QString &px)
{
    if (!px.isEmpty()) {
        _scene->loadImage(px);
        setImageLimits();
    }
}


void MainWindow::setImageLimits()
{
    resetCalibPage();
    resetBaseLinePage();
    resetTracePage();
    resetSavePage();

    ui->graphicsViewGraph->fitScene();

    int w = _scene->imageWidth();
    int h = _scene->imageHeight();

    ui->spinBoxBLX->setMaximum(w);
    ui->spinBoxBRX->setMaximum(w);
    ui->spinBoxTLX->setMaximum(w);

    ui->spinBoxBLY->setMaximum(h);
    ui->spinBoxBRY->setMaximum(h);
    ui->spinBoxTLY->setMaximum(h);

    ui->toolBox->setItemEnabled(0, true);
    ui->toolButtonStartCalibMode->setEnabled(true);
    ui->calibInfoWidget->setDisplayMode(0);
}

void MainWindow::setCalibMode(bool b)
{
    _scene->setCalibMode(b);

    if (b) {
        clearCalibData();
        resetBaseLinePage();
        resetTracePage();
        resetSavePage();

        ui->toolButtonStartCalibMode->setToolTip(tr("End axis calibration mode"));
        _calibPointsPx.clear();
        ui->calibInfoWidget->setDisplayMode(1);
    } else {
        ui->toolButtonStartCalibMode->setToolTip(tr("Start axis calibration mode"));
    }
}

void MainWindow::setBaseLineMode(bool b)
{
    _scene->setBaseLineMode(b);

    if (b) {
        resetTracePage();
        resetSavePage();

        ui->baseLineInfoWidget->setDisplayMode(1);
        ui->toolButtonBaseLineMode->setText(tr("End"));
    } else {
        ui->toolButtonBaseLineMode->setText(tr("Start"));

        if (_scene->baseLinePolygon().size() > 1) {
            ui->baseLineInfoWidget->setDisplayMode(2);
            ui->toolBox->setItemEnabled(2, true);
            ui->toolButtonBaseLineNext->setEnabled(true);
        } else {
            ui->baseLineInfoWidget->setDisplayMode(0);
        }
    }
}

void MainWindow::mouseClickPosition(QPointF p)
{
    if (_scene->calibMode()) {
        addCalibPoint(p);
    } else if (_scene->baseLineMode()) {
        double calXmax = 0.0;

        for (int i = 0; i < _calibPointsPx.size(); ++i) {
            calXmax = qMax(calXmax, _calibPointsPx.at(i).x());
        }

        if (p.x() > calXmax) {
            bool oldState = ui->toolButtonBaseLineMode->blockSignals(true);
            setBaseLineMode(false);
            ui->toolButtonBaseLineMode->setChecked(false);
            ui->toolButtonBaseLineMode->blockSignals(oldState);
        }
    }
}

void MainWindow::addCalibPoint(const QPointF &p)
{
    _calibPointsPx.append(p);

    blockSpinBoxSignals(true);

    if (_calibPointsPx.size() == 1) {
        ui->spinBoxBLX->setValue(int(p.x()));
        ui->spinBoxBLY->setValue(int(p.y()));
        ui->spinBoxBLX->setEnabled(true);
        ui->spinBoxBLY->setEnabled(true);
        ui->calibInfoWidget->setDisplayMode(2);
    } else if (_calibPointsPx.size() == 2) {
        ui->spinBoxBRX->setValue(int(p.x()));
        ui->spinBoxBRY->setValue(int(p.y()));
        ui->spinBoxBRX->setEnabled(true);
        ui->spinBoxBRY->setEnabled(true);
        ui->calibInfoWidget->setDisplayMode(3);
    } else if (_calibPointsPx.size() == 3) {
        ui->spinBoxTLX->setValue(int(p.x()));
        ui->spinBoxTLY->setValue(int(p.y()));
        ui->spinBoxTLX->setEnabled(true);
        ui->spinBoxTLY->setEnabled(true);
        ui->calibInfoWidget->setDisplayMode(4);

        ui->lineEditBLX->setEnabled(true);
        ui->lineEditBLY->setEnabled(true);
        ui->lineEditBRX->setEnabled(true);
        ui->lineEditBRY->setEnabled(true);
        ui->lineEditTLX->setEnabled(true);
        ui->lineEditTLY->setEnabled(true);

        // append TR point
        _calibPointsPx.append(QPoint());
        updateCalibPointTR();
    }

    blockSpinBoxSignals(false);

    if (_calibPointsPx.size() > 3) {
        ui->toolButtonStartCalibMode->setChecked(false);
        //setCalibMode(false);
    }
}

void MainWindow::blockSpinBoxSignals(bool b)
{
    ui->spinBoxBLX->blockSignals(b);
    ui->spinBoxBLY->blockSignals(b);
    ui->spinBoxBRX->blockSignals(b);
    ui->spinBoxBRY->blockSignals(b);
    ui->spinBoxTLX->blockSignals(b);
    ui->spinBoxTLY->blockSignals(b);
}

void MainWindow::mouseCoordinates(QPointF p)
{
    QPointF pu = _unityToCoordinates.map(_pxToUnity.map(p));
    QString s = QString("%1 %2").arg(pu.x(), 0, 'f', 2).arg(pu.y(), 0, 'f', 1);
    labelStatus->setText(s);
}

void MainWindow::updateMatrix()
{
    bool ok;
    int err = 0;

    double dblx = ui->lineEditBLX->text().toDouble(&ok);
    if (!ok) ++err;
    double dbly = ui->lineEditBLY->text().toDouble(&ok);
    if (!ok) ++err;
    double dbrx = ui->lineEditBRX->text().toDouble(&ok);
    if (!ok) ++err;
    double dbry = ui->lineEditBRY->text().toDouble(&ok);
    if (!ok) ++err;
    double dtlx = ui->lineEditTLX->text().toDouble(&ok);
    if (!ok) ++err;
    double dtly = ui->lineEditTLY->text().toDouble(&ok);
    if (!ok) ++err;

    if (err) return;

    // construct top right point
    double dtrx = dbrx + dtlx - dblx;
    double dtry = dbry + dtly - dbly;

    _calibPointsCalib.clear();
    _calibPointsCalib.append(QPointF(dblx, dbly));
    _calibPointsCalib.append(QPointF(dbrx, dbry));
    _calibPointsCalib.append(QPointF(dtlx, dtly));
    _calibPointsCalib.append(QPointF(dtrx, dtry));

    // projects pixel coordinates to unity coordinates (0.0 - 1.0)
    _pxToUnity = getPxToUnity();

    // projects unity coordinates to plot coordinates
    _unityToCoordinates = getUnityToCoordinates();

    ui->toolBox->setItemEnabled(1, true);
    ui->toolButtonCalibNext->setEnabled(true);
    ui->calibInfoWidget->setDisplayMode(5);
}

QTransform MainWindow::getPxToUnity()
{
    // based on:
    // https://upload.wikimedia.org/wikipedia/commons/thumb/a/ab/Perspective_transformation_matrix_2D.svg/1024px-Perspective_transformation_matrix_2D.svg.png

    if (_calibPointsPx.size() < 4) return QTransform(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);

    // use shorter variables for the equations
    QPointF bl = _calibPointsPx.at(0);
    QPointF br = _calibPointsPx.at(1);
    QPointF tl = _calibPointsPx.at(2);
    QPointF tr = _calibPointsPx.at(3);

    double _a = br.x() - bl.x();
    double _b = tr.x() - br.x();
    double _c = bl.x();
    double _d = tr.y() - tl.y();
    double _e = tl.y() - br.y();
    double _f = bl.y();

    QTransform t(_a, _d, 0.0, _b, _e, 0.0, _c, _f, 1.0);
    return t.inverted();
}

QTransform MainWindow::getUnityToCoordinates()
{
    // based on:
    // https://upload.wikimedia.org/wikipedia/commons/thumb/a/ab/Perspective_transformation_matrix_2D.svg/1024px-Perspective_transformation_matrix_2D.svg.png

    if (_calibPointsCalib.size() < 4) return QTransform(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);

    // use shorter variables for the equations
    QPointF bl = _calibPointsCalib.at(0);
    QPointF br = _calibPointsCalib.at(1);
    QPointF tl = _calibPointsCalib.at(2);
    QPointF tr = _calibPointsCalib.at(3);

    double _a = br.x() - bl.x();
    double _b = tr.x() - br.x();
    double _c = bl.x();
    double _d = tr.y() - tl.y();
    double _e = tl.y() - br.y();
    double _f = bl.y();

    return QTransform(_a, _d, 0.0, _b, _e, 0.0, _c, _f, 1.0);
}

void MainWindow::calibBLChanged()
{
    if (_calibPointsPx.size() > 0) {
        _calibPointsPx[0].setX(ui->spinBoxBLX->value());
        _calibPointsPx[0].setY(ui->spinBoxBLY->value());
        _scene->setBL(QPoint(int(_calibPointsPx.at(0).x()), int(_calibPointsPx.at(0).y())));
        updateCalibPointTR();
        updateMatrix();
    }
}

void MainWindow::calibBRChanged()
{
    if (_calibPointsPx.size() > 1) {
        _calibPointsPx[1].setX(ui->spinBoxBRX->value());
        _calibPointsPx[1].setY(ui->spinBoxBRY->value());
        _scene->setBR(QPoint(int(_calibPointsPx.at(1).x()), int(_calibPointsPx.at(1).y())));
        updateCalibPointTR();
        updateMatrix();
    }
}

void MainWindow::calibTLChanged()
{
    if (_calibPointsPx.size() > 2) {
        _calibPointsPx[2].setX(ui->spinBoxTLX->value());
        _calibPointsPx[2].setY(ui->spinBoxTLY->value());
        _scene->setTL(QPoint(int(_calibPointsPx.at(2).x()), int(_calibPointsPx.at(2).y())));
        updateCalibPointTR();
        updateMatrix();
    }
}

void MainWindow::calibCoordinatesChanged()
{
    int err = 0;
    err += ui->lineEditBLX->hasAcceptableInput() ? 0 : 1;
    err += ui->lineEditBLY->hasAcceptableInput() ? 0 : 1;
    err += ui->lineEditBRX->hasAcceptableInput() ? 0 : 1;
    err += ui->lineEditBRY->hasAcceptableInput() ? 0 : 1;
    err += ui->lineEditTLX->hasAcceptableInput() ? 0 : 1;
    err += ui->lineEditTLY->hasAcceptableInput() ? 0 : 1;

    if (err == 0) updateMatrix();
}

void MainWindow::updateCalibPointTR()
{
    if (_calibPointsPx.size() > 3) {
        double trX = _calibPointsPx.at(1).x() + _calibPointsPx.at(2).x() - _calibPointsPx.at(0).x();
        double trY = _calibPointsPx.at(1).y() + _calibPointsPx.at(2).y() - _calibPointsPx.at(0).y();
        _calibPointsPx[3].setX(int(trX));
        _calibPointsPx[3].setY(int(trY));
    }
}

void MainWindow::trace()
{
    _traceCoord.clear();

    if (_pxToUnity.isIdentity()) qDebug() << QString("ScanTracerDialog::trace(): Warning: Pixel-to-Unity matrix is identity, continuing anyway");
    if (_unityToCoordinates.isIdentity()) qDebug() << QString("ScanTracerDialog::trace(): Warning: Unity-to-Coordinates matrix is identity, continuing anyway");

    if (!_scene->hasPixmap()) {
        qDebug() << QString("ScanTracerDialog::trace(): Scene has no pixmap. Exiting.");
        return;
    }

    if (!_scene->hasBaseLine()) {
        qDebug() << QString("ScanTracerDialog::trace(): Scene has no baseline. Exiting.");
        return;
    }

    _tracer->setPixmap(_scene->pixmap());
    _tracer->setBaseLine(_scene->baseLinePolygon());
    _tracer->setNPoints(int(0.5 + (_calibPointsCalib.at(1).x() - _calibPointsCalib.at(0).x()) / ui->doubleSpinBoxStepSize->value()));
    _tracer->setCalibrationPoints(_calibPointsPx.at(0), _calibPointsPx.at(1), _calibPointsPx.at(2));
    QPolygonF tracePx = _tracer->startTrace();

    for (int i = 0; i < tracePx.size(); ++i) {
        _traceCoord.append(_unityToCoordinates.map(_pxToUnity.map(tracePx.at(i))));
    }

    ui->toolBox->setItemEnabled(3, true);
    ui->toolButtonTraceNext->setEnabled(true);
}

void MainWindow::clearCalibData()
{
    ui->spinBoxBLX->setEnabled(false);
    ui->spinBoxBLY->setEnabled(false);
    ui->spinBoxBRX->setEnabled(false);
    ui->spinBoxBRY->setEnabled(false);
    ui->spinBoxTLX->setEnabled(false);
    ui->spinBoxTLY->setEnabled(false);

    ui->lineEditBLX->setEnabled(false);
    ui->lineEditBLY->setEnabled(false);
    ui->lineEditBRX->setEnabled(false);
    ui->lineEditBRY->setEnabled(false);
    ui->lineEditTLX->setEnabled(false);
    ui->lineEditTLY->setEnabled(false);

    ui->spinBoxBLX->setValue(0);
    ui->spinBoxBLY->setValue(0);
    ui->spinBoxBRX->setValue(0);
    ui->spinBoxBRY->setValue(0);
    ui->spinBoxTLX->setValue(0);
    ui->spinBoxTLY->setValue(0);

    ui->lineEditBLX->clear();
    ui->lineEditBLY->clear();
    ui->lineEditBRX->clear();
    ui->lineEditBRY->clear();
    ui->lineEditTLX->clear();
    ui->lineEditTLY->clear();
}

void MainWindow::resetGui()
{
    if (ui->toolBox->count()) ui->toolBox->setCurrentIndex(0);

    for (int i = 0; i < ui->toolBox->count(); ++i) {
        ui->toolBox->setItemEnabled(i, i == 0);
    }

    ui->calibInfoWidget->setDisplayMode(-1);
    ui->toolButtonStartCalibMode->setEnabled(false);
    ui->toolButtonCalibNext->setEnabled(false);
    ui->toolButtonBaseLineNext->setEnabled(false);
    ui->toolButtonTraceNext->setEnabled(false);
}

void MainWindow::resetCalibPage()
{
    if (ui->toolBox->count() > 0) ui->toolBox->widget(0)->setEnabled(true);
    clearCalibData();
}

void MainWindow::resetBaseLinePage()
{
    if (ui->toolBox->count() > 1) ui->toolBox->widget(1)->setEnabled(true);
    _scene->clearBaseLine();
}

void MainWindow::resetTracePage()
{
    if (ui->toolBox->count() > 2) ui->toolBox->widget(2)->setEnabled(true);
    _traceCoord.clear();
    _scene->clearTrace();
}

void MainWindow::resetSavePage()
{
    if (ui->toolBox->count() > 3) ui->toolBox->widget(3)->setEnabled(true);
}

void MainWindow::nextPage()
{
    int i = ui->toolBox->currentIndex();
    if (ui->toolButtonStartCalibMode->isChecked()) ui->toolButtonStartCalibMode->setChecked(false);
    if (ui->toolButtonBaseLineMode->isChecked()) ui->toolButtonBaseLineMode->setChecked(false);
    if (ui->toolBox->count() > i + 1) ui->toolBox->setCurrentIndex(i + 1);
}

void MainWindow::scanColorDialog()
{
    if (!_tracer) return;
    _scanColor = QColorDialog::getColor(_scanColor, this, tr("Scan color"));
    _tracer->setScanColor(_scanColor);
    ui->labelColorScan->setStyleSheet(QString("background-color:%1").arg(_scanColor.name()));
}

void MainWindow::backgroundColorDialog()
{
    if (!_tracer) return;
    _backgroundColor = QColorDialog::getColor(_backgroundColor, this, tr("Background color"));
    _tracer->setBackgroundColor(_backgroundColor);
    ui->labelColorBackground->setStyleSheet(QString("background-color:%1").arg(_backgroundColor.name()));
}

void MainWindow::pickScanColor(bool b)
{
    if (!_scene) return;
    _scene->setPickScanColorMode(b);
}

void MainWindow::pickBackgroundColor(bool b)
{
    if (!_scene) return;
    _scene->setPickBackgroundColorMode(b);
}

void MainWindow::scanColorPicked(QColor c)
{
    _scanColor = c;
    ui->toolButtonPickScanColor->setChecked(false);

    if (!_tracer) return;

    _tracer->setScanColor(_scanColor);
    ui->labelColorScan->setStyleSheet(QString("background-color:%1").arg(_scanColor.name()));
}

void MainWindow::backgroundColorPicked(QColor c)
{
    _backgroundColor = c;
    ui->toolButtonPickBackgroundColor->setChecked(false);

    if (!_tracer) return;

    _tracer->setBackgroundColor(_backgroundColor);
    ui->labelColorBackground->setStyleSheet(QString("background-color:%1").arg(_backgroundColor.name()));
}

void MainWindow::colorSensitivity(int i)
{
    if (_tracer) _tracer->setColorSensitivity(i);
}

void MainWindow::lineCenter(int i)
{
    if (_tracer) _tracer->setLineCenterMode(i);
}

void MainWindow::helpAbout()
{
    HelpAboutDialog *hdlg = new HelpAboutDialog(this);
    hdlg->setVersion(QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD));
    hdlg->setLogDestination(logDest);
    hdlg->exec();
    delete hdlg;
}

/* EOF */

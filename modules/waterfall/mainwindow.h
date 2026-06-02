/***************************************************************************
                          mainwindow.h  -  description
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>

#include "../../libXrdIO/settingsmanager.h"
#include "../../libXrdIO/import/importhandler.h"
#include "../../libXrdIO/colorMaps/lutgenerator.h"
#include "../../libXrdIO/scan.h"
#include "waterfallplotwidget.h"
#include "directorymonitorsetupdialog.h"
#include <QDialog>
#include <QHash>
#include <QFileSystemWatcher>
#include <QTreeWidgetItem>
#include <QStandardItemModel>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void loadFileList(const QStringList &);
    inline void setLogDest(const QString &d) {logDest = d;}
    bool doShowMaximized();
    inline void setWorkingDir(const QString &d) {_workingDir = d;}

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *);
    void dragEnterEvent(QDragEnterEvent *);
    void dropEvent(QDropEvent *);

protected slots:
    void sortFiles(int);

private:
    Ui::MainWindow *ui;

    SettingsManager *settings;
    DirectoryMonitorSetupDialog *monDirDlg;
    QLabel *statusCoordinates;
    QLabel *statusMonitorDir;
    QLabel *statusMonitorStatus;
    QString logDest;
    QMenu *twContextMenu;
    QString _workingDir;
    QString curFilter;
    QMap<QString, colorMaps::Lut> lutList;
    QFileSystemWatcher fsWatcher;
    QHash<QUuid, Scan> _scanHeap;
    QVector<QUuid> _uidList;
    QHash<QUuid, QString> _fileNames;
    QString _monitorDir;
    int _monitorFileType;
    QStringList _monitorFilter;
    QString _labelFilterRegexp;
    QString _labelFilterPrefix;
    QString _labelFilterSuffix;

    void initSettings();
    void saveSettings();
    void appendFileScans(const QString &file, const QString &filter = QString());
    void updateYRangeSpinboxes();
    void removeNonexistentFiles();
    QString getLabel(const QString &);

private slots:
    void showContextMenu(const QPoint &);
    void addFile();
    void removeSelected();
    void removeUnchecked();
    void clearScans();
    void updateScene();
    void selectDir();
    void monitorDir(bool);
    void updateLabels();
    void saveAs();
    void lutChanged();
    void colorTempChanged();
    void xRangeChanged();
    void yRangeChanged();
    void mouseCoordinates(QString);
    void resetX();
    void resetY();
    void dirContentChanged(const QString &);
    void itemsSorted();
    void updateCheckedItemList();
    void itemSelectionChanged();
    void scanHighlightingToggled(bool);
    void treeWidgetCheckAll();
    void treeWidgetUncheckAll();
    void treeWidgetCheckSelected();
    void treeWidgetUncheckSelected();
    void updateXRangeSpinboxes();
    void gammaChanged(int);
    void resetGamma();
    void setLabelFilter();
    void helpAbout();
    void clearSelection();
    void toggleDrawScanNames(bool);
};

#endif // MAINWINDOW_H

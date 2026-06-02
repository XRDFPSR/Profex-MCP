/***************************************************************************
                          strucimportdialog.h  -  description
                             -------------------
    begin                : Fri Mar 13 14:02:00 CEST 2015
    copyright            : (C) 2015 by Nicola Doebelin
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

#ifndef STRUCIMPORTDIALOG_H
#define STRUCIMPORTDIALOG_H

#include <QMainWindow>
#include "strucimportitemdata.h"
#include "3rdparty/qcustomplot/qcustomplot.h"
#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/coddbmanager.h"
#include "../libXrdIO/crystal/crystalstructure.h"
#include "../libXrdIO/parser/bgmnsgdatparser.h"
#include "codcifretrievedialog.h"
#include "coddownloadmanager.h"
#include "codlocalcifmanager.h"
#include "wavelengthcombobox.h"

namespace Ui {
    class StrucImportDialog;
}

struct BondDist {
    QString elementA;
    QString elementB;
    double structureDist;
    double referenceDist;

    BondDist(QString a, QString b, double s, double r) : elementA(a), elementB(b), structureDist(s), referenceDist(r) {}
};

class StrucImportDialog : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit StrucImportDialog(QWidget *parent = 0);
    ~StrucImportDialog();

    void setFile(const QString &);
    void setFile(const QStringList &);
    void showAndAdd();
    void initSettings();

private:
    Ui::StrucImportDialog *ui;
    SettingsManager *settings;
    CodCifRetrieveDialog *codSearchDialog;
    CodDbManager *codManager;
    QCustomPlot *hklPlot;
    QPlainTextEdit *textEditMessages;
    BgmnSgDatParser sgParser;
    QString msgFadeColor;
    QString workingDir;
    QString selectedFilter;
    QMap<QUuid, CrystalStructure> crystalStructureMap;
    bool strIndexRequired;
    bool processWasCancelled;
    QMap<QUuid, StrucImportItemData*> listData;
    QMap<QListWidgetItem*, QUuid> listItems;
    CodDownloadManager *codOnlineDownloader;
    CodLocalCifManager *codLocalDownloader;
    bool verifyAllMode;
    QList<QUuid> verifyList;
    QProgressBar *progressBar;
    QLabel *labelNumberOfFiles;
    WaveLengthComboBox *comboBoxWavelength;
    int nVerifiedFiles;

    void closeEvent(QCloseEvent *);
    void initConnections();
    void saveSettings();
    void exportHklCsv(const QString &, const StrucImportItemData *);
    void exportHklPdf(const QString &, const StrucImportItemData *);
    void exportHklPng(const QString &, const StrucImportItemData *);

    void setFileNames(const QStringList &);
    void plotHklLines(const StrucImportItemData *);
    void clearData();
    QVector<double> hklToTwoTheta(const QVector<Hkl> &, double);
    QVector<double> hklToIntensity(const QVector<Hkl> &);
    void saveFile(const StrucImportItemData *it, const QString &fileName, int format);
    void setVerifiedOKStatus(const QUuid &, bool);
    void updateNumberOfFiles();
    void setMessage(const StrucImportItemData *);
    void displayItemData(const StrucImportItemData *);
    void setGuiState(bool);
    void toggleActionsEnabled();

    StrucImportItemData * currentItemData();
    StrucImportItemData * itemData(int);
    StrucImportItemData * itemData(QListWidgetItem *);
    QUuid currentItemUid() const;

private slots:
    void selectionChanged(QListWidgetItem*, QListWidgetItem*);
    void addFiles();
    void searchCodDb();
    void enterCodCodes();
    void convertStructure();
    void removeFile();
    void closeAll();
    void closeVerified();
    void saveAs();
    void saveAll();
    void verify();
    void verifyAll();
    void cancelProcess();
    void exportGraphs();

    void zoomHklPlot(const QCPRange &);
    void resetHklPlotZoom(QMouseEvent *);
    void resetHklPlotZoom();
    void redrawHklPlot();
    void exportMessages();

    void loadCodCifs(QList<CifFile> &, QStringList &);
    void verifyNext();
    void verifyComplete(QUuid);

signals:
    void emitMessage(QString);
    void emitRunIndexing();
};

#endif // STRUCIMPORTDIALOG_H

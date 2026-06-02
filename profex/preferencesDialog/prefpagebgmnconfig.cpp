/***************************************************************************
                          prefpagebgmnconfig.cpp  -  description
                             -------------------
    begin                : Wed May 10 09:00:00 CEST 2017
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

#include "prefpagebgmnconfig.h"
#include "ui_prefpagebgmnconfig.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QRegularExpression>

PrefPageBgmnConfig::PrefPageBgmnConfig(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageBgmnConfig)
{
    ui->setupUi(this);
}

PrefPageBgmnConfig::~PrefPageBgmnConfig()
{
    delete ui;
}

void PrefPageBgmnConfig::initUi()
{
    connect(ui->radioButtonInstCustom, SIGNAL(toggled(bool)), this, SLOT(toggleCustomBgmnEnabled(bool)));
    connect(ui->pushButtonBgmnExec, SIGNAL(clicked(bool)), this, SLOT(selectBgmnExec()));
    connect(ui->pushButtonGeomet,   SIGNAL(clicked(bool)), this, SLOT(selectGeometExec()));
    connect(ui->pushButtonMakeGeq,  SIGNAL(clicked(bool)), this, SLOT(selectMakeGeqExec()));
    connect(ui->pushButtonTeil,     SIGNAL(clicked(bool)), this, SLOT(selectTeilExec()));
    connect(ui->pushButtonEflech,   SIGNAL(clicked(bool)), this, SLOT(selectEflechExec()));
    connect(ui->toolButtonResetNotConvertFormats, SIGNAL(clicked(bool)), this, SLOT(resetXYFormats()));
    initSettings();
}

void PrefPageBgmnConfig::initSettings()
{
#ifdef Q_OS_WIN
    int bgmnInstallation = settings->value("bgmnProject/bgmnInstallation", 0).toInt();;
#else
    #ifdef Q_OS_OSX
        int bgmnInstallation = settings->value("bgmnProject/bgmnInstallation", 0).toInt();;
    #else
        int bgmnInstallation = settings->value("bgmnProject/bgmnInstallation", 1).toInt();;
        ui->radioButtonInstBundled->setEnabled(false);
    #endif
#endif

    ui->radioButtonInstBundled->setChecked(bgmnInstallation == 0);
    ui->radioButtonInstCustom->setChecked(bgmnInstallation == 1);
    ui->groupBgmnCustom->setEnabled(ui->radioButtonInstCustom->isChecked());

    ui->lineEditBgmnExec->setText(settings->value("bgmnProject/bgmnExec", "").toString());
    ui->lineEditMakeGeqExec->setText(settings->value("bgmnProject/makegeqExec", "").toString());
    ui->lineEditGeometExec->setText(settings->value("bgmnProject/geometExec", "").toString());
    ui->lineEditTeilExec->setText(settings->value("bgmnProject/teilExec", "").toString());
    ui->lineEditEflechExec->setText(settings->value("bgmnProject/eflechExec", "").toString());

    // ui->checkBoxConvertToXy->setChecked(settings->value("bgmnProject/convertToXy", true).toBool());
    ui->lineEditScanFilesNotConverted->setText(settings->value("bgmnProject/skipConvertToXy", "val xye").toStringList().join(" "));
    ui->checkBoxWlLamFile->setChecked(settings->value("bgmnProject/readLamFile", false).toBool());
    ui->checkBoxCreateReport->setChecked(settings->value("bgmnProject/createReport", false).toBool());

    ui->checkBoxManageGoals->setChecked(settings->value("bgmnProject/manageGoals", true).toBool());
    ui->checkBoxQuantGoals100->setChecked(settings->value("bgmnProject/quantGoals100percent", false).toBool());

    ui->checkBoxOpenAddedStrFiles->setChecked(settings->value("bgmnProject/openAddedStrFiles", false).toBool());

    workingDir = ui->lineEditBgmnExec->text().isEmpty() ? QDir::homePath() : ui->lineEditBgmnExec->text();
}

void PrefPageBgmnConfig::saveSettings()
{
    int bgmnInstallation = ui->radioButtonInstBundled->isChecked() ? 0 : 1;

    settings->setValue("bgmnProject/bgmnInstallation", bgmnInstallation);

    if (bgmnInstallation == 1) {
        settings->setValue("bgmnProject/bgmnExec", ui->lineEditBgmnExec->text());
        settings->setValue("bgmnProject/makegeqExec", ui->lineEditMakeGeqExec->text());
        settings->setValue("bgmnProject/geometExec", ui->lineEditGeometExec->text());
        settings->setValue("bgmnProject/teilExec", ui->lineEditTeilExec->text());
        settings->setValue("bgmnProject/eflechExec", ui->lineEditEflechExec->text());
    }

    static QRegularExpression rxSep("[\\s;,]+");
    // settings->setValue("bgmnProject/convertToXy", ui->checkBoxConvertToXy->isChecked());
    settings->setValue("bgmnProject/skipConvertToXy", ui->lineEditScanFilesNotConverted->text().toLower().split(rxSep));
    settings->setValue("bgmnProject/readLamFile", ui->checkBoxWlLamFile->isChecked());
    settings->setValue("bgmnProject/createReport", ui->checkBoxCreateReport->isChecked());

    settings->setValue("bgmnProject/manageGoals", ui->checkBoxManageGoals->isChecked());
    settings->setValue("bgmnProject/quantGoals100percent", ui->checkBoxQuantGoals100->isChecked());

    settings->setValue("bgmnProject/openAddedStrFiles", ui->checkBoxOpenAddedStrFiles->isChecked());
}

// here we use the autoguess function, which tries to find all other exes aswell. For the other exes, we will
// not use autoguess, because if it fails, there should still be a manual way of searching the exes.
void PrefPageBgmnConfig::selectBgmnExec()
{
    QString exe = getExec(tr("Select BGMN executable"));
    autoGuessBgmnExe(exe);
}

void PrefPageBgmnConfig::selectMakeGeqExec()
{
    QString str = getExec(tr("Select MakeGEQ executable"));
    if (!str.isEmpty()) {
        ui->lineEditMakeGeqExec->setText(str);
    }
}

void PrefPageBgmnConfig::selectGeometExec()
{
    QString str = getExec(tr("Select GEOMET executable"));
    if (!str.isEmpty()) {
        ui->lineEditGeometExec->setText(str);
    }
}

void PrefPageBgmnConfig::selectTeilExec()
{
    QString str = getExec(tr("Select TEIL executable"));
    if (!str.isEmpty()) {
        ui->lineEditTeilExec->setText(str);
    }
}

void PrefPageBgmnConfig::selectEflechExec()
{
    QString str = getExec(tr("Select EFLECH executable"));
    if (!str.isEmpty()) {
        ui->lineEditEflechExec->setText(str);
    }
}

/*
 * after receiving the location of an executable file (e.g. C:\BGMNwin\BGMN.EXE), try
 * to guess all other executables. But do it in a robust way, consider formats like:
 * C:\<path>\BGMN.EXE
 * C:\<path>\bgmn.exe
 * /opt/bgmnwin/bgmn
 * /opt/bgmnwin/BGMN
 * etc.
 */
void PrefPageBgmnConfig::autoGuessBgmnExe(const QString &s)
{
    QFileInfo fi(s);

    if (!fi.exists()) {
        return;
    }

    QMap<QString, QString> fnames;
    fnames.insert("bgmn", "bgmn");
    fnames.insert("makegeq", "makegeq");
    fnames.insert("geomet", "geomet");
    fnames.insert("teil", "teil");
    fnames.insert("eflech", "eflech");

    bool hasExt = false;
    bool isUpper = false;

    QString dir = fi.absolutePath();

    // check if the file name has the extension "exe"
    if (fi.suffix().toLower() == "exe") {
        hasExt = true;
    }

    // check if the file name is in upper case
    if (fi.fileName().toUpper() == fi.fileName()) {
        isUpper = true;
    }

    QMap<QString, QString>::Iterator it = fnames.begin();

    while (it != fnames.end()) {
        // append the extension if necessary
        if (hasExt) {
            it.value() = QString("%1.exe").arg(it.value());
        }

        // change the case
        if (isUpper) {
            it.value() = it.value().toUpper();
        } else {
            it.value() = it.value().toLower();
        }

        // prepend the path
        it.value() = QString("%1/%2").arg(dir).arg(it.value());

        // check if the file exists, else erase the name. We'll test for empty names before filling
        // in the lineedits
        QFileInfo finfo(it.value());

        if (!finfo.exists()) {
            it.value() = "";
        }

        it++;
    }

    if (!fnames.value("bgmn").isEmpty()) {
        ui->lineEditBgmnExec->setText(fnames.value("bgmn"));
    }

    if (!fnames.value("makegeq").isEmpty()) {
        ui->lineEditMakeGeqExec->setText(fnames.value("makegeq"));
    }

    if (!fnames.value("geomet").isEmpty()) {
        ui->lineEditGeometExec->setText(fnames.value("geomet"));
    }

    if (!fnames.value("teil").isEmpty()) {
        ui->lineEditTeilExec->setText(fnames.value("teil"));
    }

    if (!fnames.value("eflech").isEmpty()) {
        ui->lineEditEflechExec->setText(fnames.value("eflech"));
    }
}

QString PrefPageBgmnConfig::getExec(const QString &s)
{
    QString str = QFileDialog::getOpenFileName(this, s, workingDir);
    QFileInfo fi(str);
    if (fi.exists()) {
        workingDir = fi.absolutePath();
        return str;
    }

    return QString();
}

void PrefPageBgmnConfig::toggleCustomBgmnEnabled(bool b)
{
    ui->groupBgmnCustom->setEnabled(b);
}

void PrefPageBgmnConfig::resetXYFormats()
{
    ui->lineEditScanFilesNotConverted->setText("val xye");
}
